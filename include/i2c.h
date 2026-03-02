#ifndef _i2ch_
#define _i2ch_

#include <Arduino.h>
#include <hwstep.hpp>

enum Command : uint8_t { 
  CMD_STOP = 0x00,
  CMD_INIT,
  CMD_HOME,
  CMD_JOG,
  CMD_GOTO,
  CMD_SWEEP,
  CMD_HOLD,
};

enum Axis : uint8_t {
  PAN = 0x00,
  TILT,
};

// Message from Raspberry Pi to Arduino
#pragma pack(push, 1)
struct I2cRxStruct {
  Command command;
  Axis axis;
  uint8_t ignoreLimits; // 1-byte bool
  int16_t value;
};

struct AxisStatus {
  bool error;
  bool homed;
  bool moving;
  int16_t position;
};

// Responsed from Arduino to Raspberry Pi
struct I2cTxStruct {
  AxisStatus pan;
  AxisStatus tilt;
};
#pragma pack(pop)

class I2CCtrl {
public:
  void begin();
  byte receiveByte(); // Returns 0x00 if there is none
  bool checkForMessage(I2cRxStruct &msg);  // Must be non-blocking
  void updateToSend(StepStatus panStatus, StepStatus tiltStatus); // Update the txData to send
  // Returns true if a message was received
};

#endif
