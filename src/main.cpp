#include <Arduino.h>
#include <esp_camera.h>
#include "mqtt.h"
#include "camera_pins.h"

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // NTP による時刻合わせ。MQTT over TLS に必要
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  camera_config_t cameraConfig = {
      .pin_pwdn = PWDN_GPIO_NUM,
      .pin_reset = RESET_GPIO_NUM,
      .pin_xclk = XCLK_GPIO_NUM,
      .pin_sscb_sda = SIOD_GPIO_NUM,
      .pin_sscb_scl = SIOC_GPIO_NUM,
      .pin_d7 = Y9_GPIO_NUM,
      .pin_d6 = Y8_GPIO_NUM,
      .pin_d5 = Y7_GPIO_NUM,
      .pin_d4 = Y6_GPIO_NUM,
      .pin_d3 = Y5_GPIO_NUM,
      .pin_d2 = Y4_GPIO_NUM,
      .pin_d1 = Y3_GPIO_NUM,
      .pin_d0 = Y2_GPIO_NUM,
      .pin_vsync = VSYNC_GPIO_NUM,
      .pin_href = HREF_GPIO_NUM,
      .pin_pclk = PCLK_GPIO_NUM,

      .xclk_freq_hz = 20000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,
      .pixel_format = PIXFORMAT_JPEG,
      .frame_size = FRAMESIZE_UXGA,
      .jpeg_quality = 10,
      .fb_count = 2};

  // camera init
  const auto err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  const auto *frameBuffer = esp_camera_fb_get();

  try
  {
    mqtt::connect();
    Serial.println(frameBuffer->len);
    mqtt::publish(frameBuffer->buf, frameBuffer->len);
  }
  catch (const char *message)
  {
    Serial.println(message);
    return;
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10000);
}
