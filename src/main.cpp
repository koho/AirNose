#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "mqtt.h"
#include "PMS.h"
#include "accumulator.h"

MQTT mqtt(MQTT_ADDR, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);

#ifdef USE_TEMP
#include <Adafruit_SHT4x.h>
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
#else
#include "weather.h"
Weather weather(LAT, LNG);
#endif

#ifdef USE_TVOC
#include "SGP30.h"
SGP30 sgp;
Trend tvocTrend;
#endif

#ifdef USE_AQICN
#include "aqicn.h"
AQICN aqicn(AQICN_ID, AQICN_NAME, LAT, LNG, AQICN_TOKEN, AQICN_INTERVAL);
AQICN::DATA aqi;
#endif

// PM Sensor
PMS::DATA pm;
SoftwareSerial pmSerial(D6, D7);
PMS pms(pmSerial);
Trend pm25Trend;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(STASSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  mqtt.connect();

  // PMS
  pmSerial.begin(9600);
  pms.passiveMode();

  bool i2c = false;
  // TEMP
  #ifdef USE_TEMP
  if (!i2c) {
    Wire.begin(D1, D2);
    i2c = true;
  }
  if (sht4.begin()) {
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
  } else {
    Serial.println("SHT4x Sensor not found");
  }
  #endif

  // TVOC
  #ifdef USE_TVOC
  if (!i2c) {
    Wire.begin(D1, D2);
    i2c = true;
  }
  if (!sgp.begin()){
    Serial.println("TVOC Sensor not found");
  }
  #endif
}

void loop() {
  JsonDocument doc;

  float rh = NAN;
  #ifdef USE_TEMP
  sensors_event_t humEvent, tempEvent;
  float temp = NAN;
  if (sht4.getEvent(&humEvent, &tempEvent)) {
    temp = tempEvent.temperature;
    rh = humEvent.relative_humidity;
  }
  doc["temp"] = temp;
  doc["rh"] = rh;
  #ifdef USE_AQICN
  aqi.temp = temp;
  aqi.rh = rh;
  #endif
  #else
  rh = weather.rh();
  #endif

  while (pmSerial.available()) { pmSerial.read(); }
  pms.requestRead();
  bool ok = pms.readUntil(&pm);
  float trend = pm25Trend.calc(ok ? pm.PM_AE_UG_2_5 : NAN);
  doc["pm_1_0"] = ok ? pm.PM_AE_UG_1_0 : NAN;
  doc["pm_2_5"] = ok ? pm.PM_AE_UG_2_5 : NAN;
  doc["pm_2_5_alt"] = ok ? PMS::altCF3(pm) : NAN;
  doc["pm_2_5_epa"] = ok ? PMS::epa(pm.PM_AE_UG_2_5, rh) : NAN;
  doc["pm_10_0"] = ok ? pm.PM_AE_UG_10_0 : NAN;
  doc["pm_10_0_adj"] = ok ? pm.PM_AE_UG_10_0 + PMS::pm10_delta(pm.PM_AE_UG_2_5) : NAN;
  doc["pm_sp_1_0"] = ok ? pm.PM_SP_UG_1_0 : NAN;
  doc["pm_sp_2_5"] = ok ? pm.PM_SP_UG_2_5 : NAN;
  doc["pm_sp_10_0"] = ok ? pm.PM_SP_UG_10_0 : NAN;
  doc["pm_rh"] = rh;
  doc["num_0_3"] = ok ? pm.NUM_UM_0_3 : NAN;
  doc["num_0_5"] = ok ? pm.NUM_UM_0_5 : NAN;
  doc["num_1_0"] = ok ? pm.NUM_UM_1_0 : NAN;
  doc["num_2_5"] = ok ? pm.NUM_UM_2_5 : NAN;
  doc["num_5_0"] = ok ? pm.NUM_UM_5_0 : NAN;
  doc["num_10_0"] = ok ? pm.NUM_UM_10_0 : NAN;
  doc["pm_trend"] = trend;
  #ifdef USE_AQICN
  aqi.pm_1_0 = ok ? pm.PM_AE_UG_1_0 : NAN;
  aqi.pm_2_5 = ok ? PMS::epa(pm.PM_AE_UG_2_5, rh) : NAN;
  aqi.pm_10_0 = ok ? pm.PM_AE_UG_10_0 + PMS::pm10_delta(pm.PM_AE_UG_2_5) : NAN;
  #endif

  #ifdef USE_TVOC
  SGP30::DATA gas;
  #ifdef USE_TEMP
  ok = sgp.read(&gas, temp, rh);
  #else
  ok = sgp.read(&gas);
  #endif
  doc["tvoc"] = ok ? gas.TVOC_PPB : NAN;
  doc["co2"] = ok ? gas.eCO2 : NAN;
  doc["tvoc_base"] = ok ? gas.TVOCBase : NAN;
  doc["co2_base"] = ok ? gas.eCO2Base : NAN;
  doc["tvoc_trend"] = tvocTrend.calc(ok ? gas.TVOC_PPB : NAN);
  #ifdef USE_AQICN
  aqi.tvoc = ok ? gas.TVOC_PPB : NAN;
  #endif
  #endif

  static char body[500];
  serializeJson(doc, body);
  mqtt.publish("data", body);

  #ifdef USE_AQICN
  aqicn.feed(aqi, trend);
  #endif
  
  delay(1000);
}
