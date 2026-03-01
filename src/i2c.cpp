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

  LOG_I2C_PRINTF("Hex Dump (%d bytes): ", size);

  for (size_t i = 0; i < size; ++i)
  {
    LOG_I2C_PRINTF("%02x ", bytes[i]);
    if ((i + 1) % 16 == 0)
      LOG_I2C_PRINTF("\n");
  }
  LOG_I2C_PRINTF("\n");
}

void receiveEvent(int numBytesReceived)
{
  LOG_I2C_PRINTF("receiveEvent numBytesReceived: %d\n", numBytesReceived);
  if (numBytesReceived != sizeof(I2cRxStruct))
  {
    LOG_I2C_PRINTF("receiveEvent: discarding %d bytes (expected %d)\n", numBytesReceived, sizeof(I2cRxStruct));
    while (Wire.available()) Wire.read();
    return;
  }
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
  LOG_I2C_PRINTF("Checksum: %02x\n", checksum);
  LOG_I2C_PRINTF("Max length: %d\n", max_length);
  bytes_to_send[length] = checksum;
}

void requestEvent()
{
  // txData.pan.error = false;
  // txData.pan.homed = true;
  // txData.pan.moving = false;
  // txData.pan.position = 0x3333;

  // txData.tilt.error = true;
  // txData.tilt.homed = false;
  // txData.tilt.moving = true;
  // txData.tilt.position = 0x4444;

  uint8_t bytes_to_send[sizeof(txData) + 1]; // +1 for checksum

  create_message(bytes_to_send, sizeof(bytes_to_send), (uint8_t*)&txData, sizeof(txData));

  // hexDump(bytes_to_send, sizeof(bytes_to_send));
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
    for (size_t i = 0; i < sizeof(rxData); ++i) {
      LOG_I2C_PRINTF("%02x ", ((uint8_t*)&rxData)[i]);
    }
    LOG_I2C_PRINTF("\n");

    LOG_I2C_PRINTF("rxData.command: %u\n", rxData.command);
    LOG_I2C_PRINTF("rxData.axis: %u\n", rxData.axis);
    LOG_I2C_PRINTF("rxData.ignoreLimits: %u\n", rxData.ignoreLimits);
    LOG_I2C_PRINTF("rxData.value: %d\n", rxData.value);
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
