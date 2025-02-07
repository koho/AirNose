#include <Arduino.h>
#include <ArduinoJson.h>
#include "PMS.h"

PMS::PMS(Stream& stream)
{
  this->_stream = &stream;
}

// Standby mode. For low power consumption and prolong the life of the sensor.
void PMS::sleep()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
  _stream->write(command, sizeof(command));
}

// Operating mode. Stable data should be got at least 30 seconds after the sensor wakeup from the sleep mode because of the fan's performance.
void PMS::wakeUp()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
  _stream->write(command, sizeof(command));
}

// Active mode. Default mode after power up. In this mode sensor would send serial data to the host automatically.
void PMS::activeMode()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
  _stream->write(command, sizeof(command));
  _mode = MODE_ACTIVE;
}

// Passive mode. In this mode sensor would send serial data to the host only for request.
void PMS::passiveMode()
{
  uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
  _stream->write(command, sizeof(command));
  _mode = MODE_PASSIVE;
}

// Request read in Passive Mode.
void PMS::requestRead()
{
  if (_mode == MODE_PASSIVE)
  {
    uint8_t command[] = { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };
    _stream->write(command, sizeof(command));
  }
}

// Blocking function for parse response. Default timeout is 1s.
bool PMS::readUntil(DATA *data, uint16_t timeout)
{
  uint32_t start = millis();
  do
  {
    if (read(data))
    {
      return true;
    }
  } while (millis() - start < timeout);

  return false;
}

// Non-blocking function for parse response.
bool PMS::read(DATA *data)
{
  if (_stream->available())
  {
    uint8_t ch = _stream->read();

    switch (_index)
    {
    case 0:
      if (ch != 0x42)
      {
        return false;
      }
      _calculatedChecksum = ch;
      break;

    case 1:
      if (ch != 0x4D)
      {
        _index = 0;
        return false;
      }
      _calculatedChecksum += ch;
      break;

    case 2:
      _calculatedChecksum += ch;
      _frameLen = ch << 8;
      break;

    case 3:
      _frameLen |= ch;
      // Unsupported sensor, different frame length, transmission error e.t.c.
      if (_frameLen != 2 * 9 + 2 && _frameLen != 2 * 13 + 2)
      {
        _index = 0;
        return false;
      }
      _calculatedChecksum += ch;
      break;

    default:
      if (_index == _frameLen + 2)
      {
        _checksum = ch << 8;
      }
      else if (_index == _frameLen + 2 + 1)
      {
        _checksum |= ch;

        if (_calculatedChecksum == _checksum)
        {
          for (uint16_t i = 0; i < _frameLen - 4; i += 2)
          {
            *(uint16_t*)((uint8_t*)data + i) = (_payload[i] << 8) | _payload[i+1];
          }
        }

        _index = 0;
        return true;
      }
      else
      {
        _calculatedChecksum += ch;
        uint8_t payloadIndex = _index - 4;

        if (payloadIndex < sizeof(_payload))
        {
          _payload[payloadIndex] = ch;
        }
      }

      break;
    }

    _index++;
  }
  return false;
}

// https://community.purpleair.com/t/calibration-of-purpleair-monitors/482
float PMS::altCF3(PMS::DATA& data) {
  return 3 * (0.00030418 * (data.NUM_UM_0_3 - data.NUM_UM_0_5) +
       0.0018512 * (data.NUM_UM_0_5 - data.NUM_UM_1_0) +
       0.02069706 * (data.NUM_UM_1_0 - data.NUM_UM_2_5));
}

// https://www.airgradient.com/documentation/correction-algorithms/
float PMS::epa(float x, float rh) {
  if (isnan(rh)) return NAN;
  float v;
  if ((x >= 0) && (x < 30)) {
    v = (0.524 * x) - (0.0862 * rh) + 5.75;
  } else if (x < 50) {
    v = (
      ((0.786 * (x/20 - 3./2)) + (0.524 * (1 - (x/20 - 3./2)))) * x
      - (0.0862 * rh) + 5.75
    );
  } else if (x < 210) {
    v = (0.786 * x) - (0.0862 * rh) + 5.75;
  } else if (x < 260) {
    v = (
      ((0.69 * (x/50 - 21./5)) + (0.786 * (1 - (x/50 - 21./5)))) * x
      - (0.0862 * rh * (1 - (x/50 - 21./5)))
      + (2.966 * (x/50 - 21./5))
      + (5.75 * (1 - (x/50 - 21./5)))
      + (8.84 * pow(10, -4) * pow(x, 2) * (x/50 - 21./5))
    );
  } else {
    v = 2.966 + (0.69 * x) + (pow(8.8410, -4) * pow(x, 2));
  }
  return v < 0 ? 0 : v;
}

// When the air quality is poor, the sensor will underestimate the PM10.
float PMS::pm10_delta(float pm25) {
  int delta = 0;
  if (pm25 >= 50 && pm25 < 60) {
    delta = 10;
  } else if (pm25 >= 60 && pm25 < 70) {
    delta = 15;
  } else if (pm25 >= 70) {
    delta = 20;
  }
  return delta;
}
