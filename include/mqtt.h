#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "config.h"

class MQTT
{
public:
  MQTT(const char *domain, uint16_t port, const char *user, const char *pass);
  bool connect();
  bool publish(const char* topic, const char* payload);

private:
  WiFiClient _wifiClient;
  PubSubClient _client;
  const char *_user, *_pass;
  String _prefix;
  unsigned long _lastReconnectAttempt = 0;
  void sendHADiscovery();
};
#endif