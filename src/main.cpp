#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"
#include "upload.h"

String getTimestamp()
{
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  delay(1000); // NTP による現在時刻取得が完了するのを待つ

  const time_t t = time(NULL);
  if (t < 1600000000) // 最近の時刻っぽい値にセットされたか確認
  {
    throw "Failed to sync current time";
  }

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%F %T", localtime(&t));
  return String(buffer);
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  setupWiFi();
  String timestamp = getTimestamp();
  Serial.println(timestamp);

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
  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  const auto *frameBuffer = esp_camera_fb_get();
  putImage(frameBuffer->buf, frameBuffer->len);
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10000);
}
