#include "weather.h"

Weather::Weather(float latitude, float longitude) {
  _latitude = latitude;
  _longitude = longitude;
  _temp = NAN;
  _rh = NAN;
}

void Weather::update() {
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();
  http.setTimeout(3000);
  http.setReuse(false);
  if (http.begin(client, "https://data.cma.cn/server/weather_content3.php?stationName=none&location=1&apiTp=1&lat=" + String(_latitude) + "&lag=" + String(_longitude) + "&dist=60")) {
    int status = http.GET();
    if (status == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      deserializeJson(doc, payload);
      const char *t = doc["DS"]["valueTem"];
      if (t != NULL)
        _temp = strtof(t, NULL);
      const char *hum = doc["DS"]["valueRhu"];
      if (hum != NULL)
        _rh = atoi(hum);
    }
  }
  http.end();
}

float Weather::temp() {
  if (isnan(_temp) || millis() - _lastUpdated > UPDATE_INTERVAL * 60 * 1000) {
    update();
    _lastUpdated = millis();
  }
  return _temp;
}

float Weather::rh() {
  if (isnan(_rh) || millis() - _lastUpdated > UPDATE_INTERVAL * 60 * 1000) {
    update();
    _lastUpdated = millis();
  }
  return _rh;
}
