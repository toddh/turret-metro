# Turret Metro - UML Diagram

## Class Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                          STEPPER MOTOR CONTROL HIERARCHY                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────┐
│          BaseStep (Abstract)         │
├─────────────────────────────────────┤
│ Protected:                           │
│  - maximumPosition: int = 5000      │
│  - minimumPosition: int = -5000     │
├─────────────────────────────────────┤
│ Methods:                            │
│  + init(pins, params): void         │
│  + moveAbsTo(pos, ignore): void     │
│  + moveRel(delta): void             │
│  + run(): void                      │
│  + moving(): bool                   │
│  + currentPosition(): int           │
│  + homeMagnet(): bool               │
│  + homePosition(pos): int           │
│  + isPastMax(): bool                │
│  + isPastMin(): bool                │
│  + reInit(): void                   │
│  + status(): StepStatus             │
└──────────────────────┬──────────────┘
                       │
         ┌─────────────┴─────────────┐
         │                           │
         ▼                           ▼
┌──────────────────────┐    ┌──────────────────────┐
│      HWStep          │    │     SimStep          │
│  (Hardware Impl)     │    │  (Simulation)        │
├──────────────────────┤    ├──────────────────────┤
│ Private:             │    │ Private:             │
│ - stepper: Accel...  │    │ - position: int      │
│ - _hallPin: int      │    │ - simMagnet: bool    │
│ - _enPin: int        │    │ - targetPos: int     │
│ - _stepPin: int      │    │ - moveDir: int       │
│ - _dirPin: int       │    │ - magnetStart: int   │
│ - _speed: float      │    │ - magnetEnd: int     │
│ - _accel: float      │    │ - lastStepTime: ms   │
│ - _maxPosition: int  │    │                      │
│ - _minPosition: int  │    ├──────────────────────┤
│ - _homeStep: int     │    │ Methods:             │
│ - _leadingEdgeBump   │    │ + init(...): void    │
│ - _isError: bool     │    │ + run(): void        │
│ - _isHomed: bool     │    │ + (all virtual)      │
├──────────────────────┤    └──────────────────────┘
│ Methods:             │
│ + init(...): void    │
│ + run(): void        │
│ + doHomeStep(): void │
│ + doLeadingEdgeBump()│
│ + reInit(): void     │
│ + (overrides all)    │
└──────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                         DATA STRUCTURES                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────┐
│    StepStatus            │
├──────────────────────────┤
│ - error: uint8_t         │
│ - homed: uint8_t         │
│ - moving: uint8_t        │
│ - position: int16_t      │
└──────────────────────────┘

┌──────────────────────────┐
│    AxisStatus            │
├──────────────────────────┤
│ - error: bool            │
│ - homed: bool            │
│ - moving: bool           │
│ - position: int16_t      │
└──────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                      I2C COMMUNICATION PROTOCOL                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────┐
│     Command (enum)       │
├──────────────────────────┤
│ CMD_STOP      = 0x00     │
│ CMD_INIT      = 0x01     │
│ CMD_HOME      = 0x02     │
│ CMD_JOG       = 0x03     │
│ CMD_GOTO      = 0x04     │
│ CMD_SWEEP     = 0x05     │
└──────────────────────────┘

┌──────────────────────────┐
│     Axis (enum)          │
├──────────────────────────┤
│ PAN           = 0x00     │
│ TILT          = 0x01     │
└──────────────────────────┘

┌────────────────────────────────────┐
│    I2cRxStruct                     │
│  (Raspberry Pi → Arduino)          │
├────────────────────────────────────┤
│ - command: Command                 │
│ - axis: Axis                       │
│ - ignoreLimits: uint8_t            │
│ - value: int16_t                   │
└────────────────────────────────────┘

┌────────────────────────────────────┐
│    I2cTxStruct                     │
│  (Arduino → Raspberry Pi)          │
├────────────────────────────────────┤
│ - pan: AxisStatus                  │
│ - tilt: AxisStatus                 │
└────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    FINITE STATE MACHINE (FFSM2)                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

FSM Context for Each Axis:

┌──────────────────────────────────────┐
│        Context (FSM Context)         │
├──────────────────────────────────────┤
│ - stepper: HWStep*                   │
│ - edgeLocationA: int                 │
│ - edgeLocationB: int                 │
│ - backedUp: bool                     │
│ - maxHomeSearchBeforeReverse: int    │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│    Payload (FSM Payload)             │
├──────────────────────────────────────┤
│ - step: int                          │
│ - desiredMagnet: bool                │
└──────────────────────────────────────┘

FSM Event Structures:

┌──────────────────────────────────────┐
│    MoveRel                           │
├──────────────────────────────────────┤
│ + ignoreLimits: bool                 │
│ + delta: int                         │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│    MoveAbs                           │
├──────────────────────────────────────┤
│ + ignoreLimits: bool                 │
│ + position: int                      │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│    StartHoming                       │
├──────────────────────────────────────┤
│ (Signal event - no data)             │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│    StopMoving                        │
├──────────────────────────────────────┤
│ (Signal event - no data)             │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│    Reset                             │
├──────────────────────────────────────┤
│ (Signal event - no data)             │
└──────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    FSM STATE HIERARCHY                                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

                    ┌──────────────────┐
                    │       On         │ (Root)
                    │   (Initial)      │
                    └────────┬─────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
    ┌─────────┐         ┌────────┐         ┌──────────┐
    │  Idle   │         │ Moving │         │  Error   │
    │ (Final) │         │        │         │          │
    └────┬────┘         └────────┘         └──────────┘
         │
         │ (Homing sequence)
         │
    ┌────▼─────────────────────────────┐
    │       HomeInit                    │
    │  - Reset edges                    │
    │  - Check magnet status            │
    └────┬─────────────────────────────┘
         │
    ┌────▼──────────────────────────────┐
    │    HomeBackupFirst                │
    │  - Back away from magnet          │
    └────┬──────────────────────────────┘
         │
    ┌────▼──────────────────────────────┐
    │  HomeFindLeadingEdge              │
    │  - Find magnet leading edge       │
    │  - Record edge location A         │
    └────┬──────────────────────────────┘
         │
    ┌────▼──────────────────────────────┐
    │  HomeFindTrailingEdge             │
    │  - Find magnet trailing edge      │
    │  - Record edge location B         │
    └────┬──────────────────────────────┘
         │
    ┌────▼──────────────────────────────┐
    │   SetHomeLocation                 │
    │  - Move to center (A+B)/2         │
    │  - Set home reference = 0         │
    └────┬──────────────────────────────┘
         │
         ▼
    ┌──────────┐
    │  Idle    │
    └──────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    MAIN COMPONENT INTERACTIONS                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                                                         │
│              TURRET METRO SYSTEM (main.cpp)            │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  PAN CONTROL                                     │  │
│  │  ┌─────────────┐         ┌──────────────────┐   │  │
│  │  │ panStep     │◄────────│ panMachine (FSM) │   │  │
│  │  │ (HWStep)    │         │                  │   │  │
│  │  └─────────────┘         └──────────────────┘   │  │
│  │       │                                         │  │
│  │       ├─► AccelStepper                          │  │
│  │       │   - DIR: GPIO 2                         │  │
│  │       │   - STEP: GPIO 3                        │  │
│  │       │   - EN: GPIO 4                          │  │
│  │       │   - HALL: GPIO 5                        │  │
│  │       │   - Speed: 4000 steps/sec               │  │
│  │       │   - Accel: 400 steps/sec²               │  │
│  │       │   - Range: +2200 to -2200 steps        │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  TILT CONTROL                                    │  │
│  │  ┌─────────────┐         ┌──────────────────┐   │  │
│  │  │ tiltStep    │◄────────│ tiltMachine (FSM)│   │  │
│  │  │ (HWStep)    │         │                  │   │  │
│  │  └─────────────┘         └──────────────────┘   │  │
│  │       │                                         │  │
│  │       ├─► AccelStepper                          │  │
│  │       │   - DIR: GPIO 8                         │  │
│  │       │   - STEP: GPIO 9                        │  │
│  │       │   - EN: GPIO 10                         │  │
│  │       │   - HALL: GPIO 11                       │  │
│  │       │   - Speed: 3000 steps/sec               │  │
│  │       │   - Accel: 400 steps/sec²               │  │
│  │       │   - Range: +800 to -600 steps          │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  I2C COMMUNICATION (Slave Addr: 0x55)           │  │
│  │                                                  │  │
│  │  Master: Raspberry Pi                           │  │
│  │  ├─ receiveEvent()   ◄─ I2cRxStruct            │  │
│  │  ├─ requestEvent()   ─► I2cTxStruct            │  │
│  │  └─ checkForMessage()                           │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  ┌──────────────────────────────────────────────────┐  │
│  │  DEBUG OUTPUT (RTT Stream or Serial)             │  │
│  │  via globals.h console configuration            │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
└─────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│                    CONTROL FLOW: MAIN LOOP                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

loop():
  │
  ├─→ checkForMessage()
  │   └─ Retrieve I2C command from receive buffer
  │
  ├─→ Route command to appropriate FSM:
  │   ├─ CMD_STOP   → panMachine.react(StopMoving{})
  │   ├─ CMD_INIT   → panMachine.react(Reset{})
  │   ├─ CMD_HOME   → panMachine.react(StartHoming{})
  │   ├─ CMD_JOG    → panMachine.react(MoveRel{...})
  │   └─ CMD_GOTO   → panMachine.react(MoveAbs{...})
  │   (Same for TILT axis)
  │
  ├─→ FSM Update Cycle:
  │   │
  │   ├─ panMachine.update()
  │   │   ├─ Current state checks conditions
  │   │   ├─ Calls panStep.run() (AccelStepper update)
  │   │   ├─ Calls panStep.moving() (check completion)
  │   │   ├─ Calls panStep.homeMagnet() (Hall sensor)
  │   │   └─ Transitions to next state if needed
  │   │
  │   └─ tiltMachine.update()
  │       └─ (Same as pan)
  │
  ├─→ updateToSend()
  │   ├─ panStatus = panStep.status()
  │   ├─ tiltStatus = tiltStep.status()
  │   └─ Package into I2cTxStruct
  │
  └─→ (Repeat)
