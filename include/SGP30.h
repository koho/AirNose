#ifndef SGP30_H
#define SGP30_H

#include <Adafruit_SGP30.h>
#include <EEPROM.h>

class SGP30
{
public:
  struct DATA {
    uint16_t TVOC_PPB;
    uint16_t eCO2;
    uint16_t TVOCBase;
    uint16_t eCO2Base;
  };
  SGP30();
  bool begin();
  bool read(DATA *data);
  bool read(DATA *data, float temperature, float humidity);

private:
  Adafruit_SGP30 _sgp;
  uint16_t _TVOCBase, _eCO2Base;
  unsigned long _lastSaved = 0;
  unsigned long _startupTime = 0;
};
#endif