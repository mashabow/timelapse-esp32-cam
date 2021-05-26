#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

namespace mqtt
{

  WiFiClientSecure wifiClient;
  PubSubClient mqttClient(wifiClient);

  void setupWiFi()
  {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.println("Connecting WiFi...");
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      if (retryCount++ > 40)
        throw "Failed to connect WiFi";
      delay(500);
    }
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }

  void setupMQTT()
  {
    wifiClient.setCACert(ROOT_CA);
    wifiClient.setCertificate(CERTIFICATE);
    wifiClient.setPrivateKey(PRIVATE_KEY);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    Serial.println("Connecting MQTT...");
    int retryCount = 0;
    while (!mqttClient.connect(THING_NAME))
    {
      Serial.println("Failed, state=" + String(mqttClient.state()));
      if (retryCount++ > 3)
        throw "Failed to connect MQTT";
      Serial.println("Try again in 5 seconds");
      delay(5000);
    }
    Serial.print("Connected.");
  }

  void connect()
  {
    setupWiFi();
    setupMQTT();
  }

}
