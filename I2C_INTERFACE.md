# Turret Metro I2C Interface

## Overview

The Turret Metro acts as an **I2C slave** at address `0x55`. The host (Raspberry Pi) is the master. Communication is **polling-based** — the host sends commands by writing to the slave, and retrieves status by reading from the slave. There is no interrupt or async notification line.

The main loop calls `i2c.updateToSend(panStep.status(), tiltStep.status())` every iteration, so the status buffer reflects real-time axis state whenever the host reads it.

---

## Commands Received (Host → Turret)

The host sends a 5-byte packed struct over I2C:

```
#pragma pack(push, 1)
struct I2cRxStruct {
  Command  command;       // uint8_t  (byte 0)
  Axis     axis;          // uint8_t  (byte 1)
  uint8_t  ignoreLimits;  // byte 2 — 0=respect limits, 1=ignore
  int16_t  value;         // bytes 3-4 — signed 16-bit, little-endian
};
#pragma pack(pop)
```

**Packet layout (5 bytes, no checksum on RX):**

| Byte | Field | Notes |
|------|-------|-------|
| 0 | `command` | See command table below |
| 1 | `axis` | `0x00` = PAN, `0x01` = TILT |
| 2 | `ignoreLimits` | `0` = enforce soft limits, `1` = bypass |
| 3–4 | `value` | Signed 16-bit step count or position |

> **Note:** The firmware does not validate a checksum on received packets. The 5 raw bytes are copied directly into `I2cRxStruct`.

### Command Codes

| Command | Code | `axis` used? | `value` used? | Description |
|---------|------|:------------:|:-------------:|-------------|
| `CMD_STOP` | `0x00` | No | No | Immediately stops motion on **both** axes |
| `CMD_INIT` | `0x01` | No | No | Resets both axis FSMs (clears error state) and re-initializes stepper hardware |
| `CMD_HOME` | `0x02` | Yes | No | Starts homing sequence on the specified axis |
| `CMD_JOG` | `0x03` | Yes | Yes | Moves specified axis by `value` steps relative to current position |
| `CMD_GOTO` | `0x04` | Yes | Yes | Moves specified axis to absolute position `value` |
| `CMD_SWEEP` | `0x05` | — | — | Defined but **not implemented** |
| `CMD_HOLD` | `0x06` | Yes | Yes | Keeps motor energized after motion completes. `value` 1 = hold (maintain torque), 0 = release (de-energize when idle). Per-axis. Reset to off by `CMD_INIT`. |

### Axis Codes

| Axis | Code |
|------|------|
| `PAN` | `0x00` |
| `TILT` | `0x01` |

---

## Status Response (Turret → Host)

When the host performs an I2C read, the Turret returns an 11-byte packet: 10 bytes of status data followed by a 1-byte checksum.

```
#pragma pack(push, 1)
struct AxisStatus {
  bool    error;     // 1 byte
  bool    homed;     // 1 byte
  bool    moving;    // 1 byte
  int16_t position;  // 2 bytes, little-endian
};                   // 5 bytes per axis

struct I2cTxStruct {
  AxisStatus pan;    // bytes 0–4
  AxisStatus tilt;   // bytes 5–9
};
#pragma pack(pop)
// + 1 byte checksum appended = 11 bytes total
```

**Packet layout (11 bytes):**

| Byte | Field | Notes |
|------|-------|-------|
| 0 | `pan.error` | `1` = pan axis is in error state |
| 1 | `pan.homed` | `1` = pan has completed homing |
| 2 | `pan.moving` | `1` = pan stepper has steps remaining |
| 3–4 | `pan.position` | Pan current position, signed 16-bit |
| 5 | `tilt.error` | `1` = tilt axis is in error state |
| 6 | `tilt.homed` | `1` = tilt has completed homing |
| 7 | `tilt.moving` | `1` = tilt stepper has steps remaining |
| 8–9 | `tilt.position` | Tilt current position, signed 16-bit |
| 10 | checksum | 8-bit modular sum of bytes 0–9 |

### Status Field Semantics

| Field | Meaning |
|-------|---------|
| `error` | Axis entered an error state (hard limit hit, homing failed). Cleared only by `CMD_INIT`. While set, all movement commands are ignored. |
| `homed` | Axis has successfully completed a homing sequence. Position is relative to the magnet center (position 0). |
| `moving` | Stepper has not yet reached its target (`AccelStepper::distanceToGo() != 0`). |
| `position` | Current step position as a signed 16-bit integer. After homing, 0 is the magnet center. |

### Checksum Algorithm

```c
uint8_t checksum = 0;
for each byte in I2cTxStruct:
    checksum = (checksum + byte) & 0xFF;
// checksum appended as byte 10
```

---

## Move Completion

There is **no interrupt or push notification**. The host detects move completion by **polling** the `moving` flag:

- `moving == 1` — axis is in motion
- `moving == 0` — axis has reached its target

The status buffer is updated every main loop iteration, so latency between actual completion and the next read is one loop cycle.

**Typical polling sequence:**

```
host: write CMD_GOTO (axis=PAN, value=500)
host: read status  →  pan.moving=1, pan.position=120
host: read status  →  pan.moving=1, pan.position=380
host: read status  →  pan.moving=0, pan.position=500  ← move complete
```

---

## Error Handling

If an axis encounters a fault (emergency limit exceeded, homing failure), it transitions to the `Error` FSM state:
- `error` flag is set to `1` in the status response
- All subsequent movement commands for that axis are silently ignored
- **Recovery:** send `CMD_INIT`, which resets the FSM to `Idle` and clears the error flag on **both** axes

---

## Communication Flow

```
Host (Raspberry Pi)               Turret Metro (I2C slave 0x55)
        |                                    |
        |--- I2C Write (5 bytes) ----------->|
        |    [cmd][axis][ignoreLimits][value] |
        |                             receiveEvent() stores packet
        |                             main loop dispatches command
        |                                    |
        |--- I2C Read (11 bytes) <-----------|
        |    [pan: err/homed/mov/pos]        |
        |    [tilt: err/homed/mov/pos]       |
        |    [checksum]              requestEvent() sends txData
        |                                    |
        |    (repeat reads to poll status)   |
```
