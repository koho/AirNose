#ifndef WEATHER_H
#define WEATHER_H

#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

class Weather
{
public:
  static const int UPDATE_INTERVAL = 15;
  Weather(float latitude, float longitude);
  float rh();
  float temp();

private:
  float _temp, _rh;
  float _latitude, _longitude;
  unsigned long _lastUpdated = 0;
  void update();
};
#endif