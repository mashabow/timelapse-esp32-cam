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
    Serial.println("Connected.");
  }

  void connect()
  {
    setupWiFi();
    setupMQTT();
  }

  void publish(const uint8_t *payload, unsigned int length)
  {
    boolean res = mqttClient.setBufferSize(200000);
    mqttClient.setSocketTimeout(200);
    mqttClient.setKeepAlive(200);
    if (!res)
    {
      Serial.println("resize failed");
    }
    String topic = String("data/") + THING_NAME;
    const boolean succeeded = mqttClient.publish(topic.c_str(), payload, length);
    if (!succeeded)
    {
      Serial.println("publish failed: " + String(mqttClient.state()));
    }
  }

}
