#include <Arduino.h>
#include <hardwareSerial.h>
#include <Wire.h>
#include <globals.h>

#include "i2c.h"

// Using this approach https://forum.arduino.cc/t/use-i2c-for-communication-between-arduinos/653958/2

static I2cRxStruct rxData;
static I2cTxStruct txData;

bool newRxData = false;

void hexDump(const void *data, size_t size)
{
  const unsigned char *bytes = static_cast<const unsigned char *>(data);

  console.printf("Hex Dump (%d bytes): ", size);

  for (size_t i = 0; i < size; ++i)
  {
    console.printf("%02x ", bytes[i]);
    if ((i + 1) % 16 == 0)
      console.printf("\n");
  }
  console.printf("\n");
}

void receiveEvent(int numBytesReceived)
{
  // console.printf("receiveEvent numBytesReceived: %d\n", numBytesReceived);
  if (newRxData == false)
  {
    // copy the data to rxData
    Wire.readBytes((byte *)&rxData, numBytesReceived);
    newRxData = true;
  }
  else
  {
    // dump the data
    while (Wire.available() > 0)
    {
      byte c = Wire.read();
    }
  }
}

uint8_t calculateChecksum(const uint8_t* data, size_t length)
{
  uint8_t checksum = 0;
  for (size_t i = 0; i < length; ++i)
  {
    checksum = (checksum + data[i]) & 0xFF;
  }
  return checksum;
}

void create_message(uint8_t* bytes_to_send, size_t max_length, uint8_t* data, size_t length)
{
  memcpy(bytes_to_send, data, length);

  uint8_t checksum = calculateChecksum(data, length);
  console.printf("Checksum: %02x\n", checksum);
  console.printf("Max length: %d\n", max_length);
  bytes_to_send[length] = checksum;
}

void requestEvent()
{
  txData.pan.error = false;
  txData.pan.homed = true;
  txData.pan.moving = false;
  txData.pan.position = 0x3333;

  txData.tilt.error = true;
  txData.tilt.homed = false;
  txData.tilt.moving = true;
  txData.tilt.position = 0x4444;

  uint8_t bytes_to_send[sizeof(txData) + 1]; // +1 for checksum

  create_message(bytes_to_send, sizeof(bytes_to_send), (uint8_t*)&txData, sizeof(txData));

  hexDump(bytes_to_send, sizeof(bytes_to_send));
  Wire.write(bytes_to_send, sizeof(bytes_to_send));
}

void I2CCtrl::begin()
{
  Wire.begin(0x55);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

bool I2CCtrl::checkForMessage(I2cRxStruct &msg)
{
  if (newRxData)
  {
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

void I2CCtrl::updateToSend(StepStatus panStatus, StepStatus tiltStatus)
{
  txData.pan.error = panStatus.error;
  txData.pan.homed = panStatus.homed;
  txData.pan.moving = panStatus.moving;
  txData.pan.position = panStatus.position;

  txData.tilt.error = tiltStatus.error;
  txData.tilt.homed = tiltStatus.homed;
  txData.tilt.moving = tiltStatus.moving;
  txData.tilt.position = tiltStatus.position;
}
