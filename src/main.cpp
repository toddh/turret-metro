#include <Arduino.h>
#include <i2c.h>
#include <Wire.h>

#include <simstep.hpp>
#include <axissm.hpp>

#include <globals.h>

// #define DHCSR (*(volatile uint32_t*)0xE000EDF0)
// bool is_debugger_attached() {
//     return (DHCSR & (1 << 0)) != 0; // C_DEBUGEN is bit 0
// }

#ifdef USE_RTT_STREAM
#include <RTTStream.h>
RTTStream _rttstream;
#define console _rttstream
#else
#define console Serial
#endif

const int PAN_DIR_PIN = 2;
const int PAN_STEP_PIN = 3;
const int PAN_EN_PIN = 4;
const int PAN_HALL_PIN = 5;

const int TILT_DIR_PIN = 8;
const int TILT_STEP_PIN = 9;
const int TILT_EN_PIN = 10;
const int TILT_HALL_PIN = 11;

I2CCtrl i2c;

int counter = 0;

HWStep panStep;
HWStep tiltStep;

Context panContext;
Context tiltContext;

FSM::Instance panMachine{panContext};
FSM::Instance tiltMachine{tiltContext};

static I2cRxStruct rxData;

void setup()
{
  #ifndef USE_RTT_STREAM
  Serial.begin(115200);
  #endif

  console.println("Setting up");

  panStep.init(PAN_DIR_PIN, PAN_STEP_PIN, PAN_EN_PIN, PAN_HALL_PIN, 2200, -2200, 2, 1, 4000, 400);
  panContext.stepper = &panStep;

  tiltStep.init(TILT_DIR_PIN, TILT_STEP_PIN, TILT_EN_PIN, TILT_HALL_PIN, 1200, -1200, 50, 1, 3000, 400);
  tiltContext.stepper = &tiltStep;

  counter = 0;

  i2c.begin();

  console.println("Setup done");
}

void loop()
{
  // counter++;

  // if (counter >= 5000000){

  //   console.println(".");
  //   counter = 0;
  // }

  if (i2c.checkForMessage(rxData))
  {
    // console.printf("rxData command: 0x%02X, axis: 0x%02X, ignoreLimits: %s value: %d\n",
    //          rxData.command, rxData.axis, rxData.ignoreLimits ? "true" : "false", rxData.value);

    switch (rxData.command)
    {

      case CMD_STOP:
        panMachine.react(StopMoving{});
        tiltMachine.react(StopMoving{});
        break;

      case CMD_HOME:
        switch (rxData.axis)
        {
          case PAN:
            console.printf("PAN_HOME\n");
            panMachine.react(StartHoming{});
            break;

          case TILT:
            console.printf("TILT_HOME\n");
            tiltMachine.react(StartHoming{});
            break;
        }
        break;

    case CMD_JOG:
      switch (rxData.axis)
      {
        case PAN:
          console.printf("PAN_JOG\n");
          panMachine.react(MoveRel{static_cast<bool>(rxData.ignoreLimits), rxData.value});
          break;

        case TILT:
          console.printf("TILT_JOG\n");
          tiltMachine.react(MoveRel{static_cast<bool>(rxData.ignoreLimits), rxData.value});
          break;
      }
      break;

    case CMD_GOTO:
      switch (rxData.axis)
      {
        case PAN:
          console.printf("PAN_GOTO\n");
          panMachine.react(MoveAbs{static_cast<bool>(rxData.ignoreLimits), rxData.value});
          break;

        case TILT:
          console.printf("TILT_GOTO\n");
          tiltMachine.react(MoveAbs{static_cast<bool>(rxData.ignoreLimits), rxData.value});
          break;
      }
      break;
    }
  }

  panMachine.update();
  tiltMachine.update();
  i2c.updateToSend(panStep.status(), tiltStep.status());
}
