#include <Arduino.h>
#include <hardwareSerial.h>
#include <Wire.h>
#include <globals.h>

#include "i2c.h"

// Using this approach https://forum.arduino.cc/t/use-i2c-for-communication-between-arduinos/653958/2

static I2cRxStruct rxData;
static I2cTxStruct txData;

bool newRxData = false;

void hexDump(const void* data, size_t size) {
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        console.printf("%02x ", bytes[i]);
        if ((i + 1) % 16 == 0) console.printf("\n");
    }
    console.printf("\n");
}

void receiveEvent(int numBytesReceived)
{
    if (newRxData == false) {
        // copy the data to rxData
        Wire.readBytes( (byte*) &rxData, numBytesReceived);
        newRxData = true;
    }
    else {
            // dump the data
        while(Wire.available() > 0) {
            byte c = Wire.read();
        }
    }
  }

  void requestEvent()
  {   
    // hexDump((byte*)&txData, sizeof(txData));
    Wire.write((byte*)&txData, sizeof(txData));
  }


void I2CCtrl::begin() {
  Wire.begin(0x55);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

bool I2CCtrl::checkForMessage(I2cRxStruct &msg) {
  if (newRxData) {
    // for (size_t i = 0; i < sizeof(rxData); ++i) {
    //   console.printf("%02x", ((uint8_t*)&rxData)[i], HEX);
    //   console.print(" ");
    // }
    // console.println();

    // console.printf("rxData.command: %u\n", rxData.command);
    // console.printf("rxData.axis: %u\n", rxData.axis);
    // console.printf("rxData.ignoreLimits: %u\n", rxData.ignoreLimits);
    // console.printf("rxData.value: %d\n", rxData.value);
    // msg = rxData;
    memcpy(&msg, &rxData, sizeof(rxData));

    newRxData = false;
    return true;
  }

  return false;
}

void I2CCtrl::updateToSend(StepStatus panStatus, StepStatus tiltStatus) {
  txData.error = false;

  txData.pan.homed = panStatus.homed;
  txData.pan.moving = panStatus.moving;
  txData.pan.position = panStatus.position;

  txData.tilt.homed = tiltStatus.homed;
  txData.tilt.moving = tiltStatus.moving;
  txData.tilt.position = tiltStatus.position;
}
