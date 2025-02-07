#include "SGP30.h"

void writeWord(int address, uint16_t number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

uint16_t readWord(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) {
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

SGP30::SGP30() {
  _lastSaved = millis();
  _TVOCBase = 0;
  _eCO2Base = 0;
}

bool SGP30::begin() {
  if (!_sgp.begin()) {
    return false;
  }
  EEPROM.begin(4);
  uint16_t tvoc = readWord(0);
  uint16_t co2 = readWord(2);
  EEPROM.end();
  if (tvoc != 0xffff && co2 != 0xffff) {
    Serial.print("****Restore baseline values: eCO2: 0x"); Serial.print(co2, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(tvoc, HEX);
    _TVOCBase = tvoc;
    _eCO2Base = co2;
    return _sgp.setIAQBaseline(co2, tvoc);
  }
  return true;
}

bool SGP30::read(DATA *data, float temperature, float humidity) {
  if (millis() - _lastSaved > 60 * 60 * 1000) {
    uint16_t TVOCBase, eCO2Base;
    if (_sgp.getIAQBaseline(&eCO2Base, &TVOCBase)) {
      _TVOCBase = TVOCBase;
      _eCO2Base = eCO2Base;
      Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2Base, HEX);
      Serial.print(" & TVOC: 0x"); Serial.println(TVOCBase, HEX);
      EEPROM.begin(4);
      writeWord(0, TVOCBase);
      writeWord(2, eCO2Base);
      EEPROM.end();
    } else {
      Serial.println("Failed to get baseline readings");
    }
    _lastSaved = millis();
  }
  if (!isnan(temperature) && !isnan(humidity)) {
    _sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));
  }
  if (_sgp.IAQmeasure()) {
    data->TVOC_PPB = _sgp.TVOC;
    data->eCO2 = _sgp.eCO2;
    data->TVOCBase = _TVOCBase;
    data->eCO2Base = _eCO2Base;
    return true;
  }
  return false;
}

bool SGP30::read(DATA *data) {
  return read(data, NAN, NAN);
}