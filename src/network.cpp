#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"

// https://www.amazontrust.com/repository/AmazonRootCA1.pem
constexpr char ROOT_CA[] = R"(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)";

void setupWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting WiFi...");
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (retryCount++ > 20)
      throw "Failed to connect WiFi";
    delay(500);
  }
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void stopWiFi()
{
  WiFi.disconnect(true);
}

void syncTime()
{
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  delay(1000); // NTP による現在時刻取得が完了するのを待つ

  if (time(NULL) < 1600000000) // 最近の時刻っぽい値にセットされたか確認
  {
    throw "Failed to sync current time.";
  }
}

void sendImage(uint8_t *buffer, const size_t length, const String filename)
{
  WiFiClientSecure wiFiClient;
  wiFiClient.setCACert(ROOT_CA);

  // Add a scoping block for HTTPClient to make sure it is destroyed before WiFiClientSecure is
  {
    HTTPClient https;

    if (!https.begin(wiFiClient, UPLOAD_URL + filename))
    {
      throw "Failed to connect to host";
    }

    https.addHeader("X-Api-Key", UPLOAD_API_KEY);
    https.addHeader("Content-Type", "image/jpeg");

    Serial.println("Uploading " + filename + "...");
    const int code = https.PUT(buffer, length);
    https.end();

    if (code < 0)
    {
      throw "Failed to upload: " + https.errorToString(code);
    }
    Serial.println("Uploaded.");
  }
}
