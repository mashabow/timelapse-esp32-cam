#include <Arduino.h>
#include <esp_camera.h>
#include "upload.h"

// deep sleep して終了。wake 時には setup から始まる
void deepSleep()
{
  Serial.println("Start deep sleep mode.");
  const int intervalMinutes = 10;
  ESP.deepSleep(intervalMinutes * 60 * 1000 * 1000);
  delay(1000); // deep sleep が始まるまで待つ
}

// カメラを初期化し、撮影した画像のフレームバッファを取得する
const camera_fb_t *captureImage()
{
  const camera_config_t cameraConfig = {
      // https://github.com/espressif/esp32-camera/blob/7da9cb5ea320c5ebed1083431447c0e13eb8cc16/examples/take_picture.c#L67-L88
      .pin_pwdn = 32,
      .pin_reset = -1,
      .pin_xclk = 0,
      .pin_sscb_sda = 26,
      .pin_sscb_scl = 27,
      .pin_d7 = 35,
      .pin_d6 = 34,
      .pin_d5 = 39,
      .pin_d4 = 36,
      .pin_d3 = 21,
      .pin_d2 = 19,
      .pin_d1 = 18,
      .pin_d0 = 5,
      .pin_vsync = 25,
      .pin_href = 23,
      .pin_pclk = 22,

      .xclk_freq_hz = 20000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,
      .pixel_format = PIXFORMAT_JPEG,
      .frame_size = FRAMESIZE_UXGA,
      .jpeg_quality = 10,
      .fb_count = 2};

  const auto err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK)
  {
    throw "Camera init failed with error 0x" + String(err, HEX);
  }

  // 各項目の意味についてはリンク先参照
  // https://github.com/mashabow/timelapse-esp32-cam/issues/4#issuecomment-850776836
  const auto sensor = esp_camera_sensor_get();
  sensor->set_aec2(sensor, 1);
  sensor->set_vflip(sensor, 1);
  sensor->set_hmirror(sensor, 1);
  sensor->set_dcw(sensor, 0);

  // ホワイトバランスが安定するまで待ってから撮影。20秒ぐらいでもいけるかもしれない
  delay(30000);
  return esp_camera_fb_get();
}

// 2021-05-29T13-16-07 のような形式のタイムスタンプ文字列を返す
const String getTimestamp(time_t capturedTimeSinceBoot)
{
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  delay(1000); // NTP による現在時刻取得が完了するのを待つ

  const auto deltaSec = millis() / 1000 - capturedTimeSinceBoot;
  const time_t capturedTime = time(NULL) - deltaSec;
  if (capturedTime < 1600000000) // 最近の時刻っぽい値にセットされたか確認
  {
    throw "Failed to sync current time";
  }

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H-%M-%S", localtime(&capturedTime));
  return String(buffer);
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  try
  {
    setupWiFi();
    const auto image = captureImage();
    const auto filename = getTimestamp(image->timestamp.tv_sec) + ".jpeg";
    sendImage(image->buf, image->len, filename);
  }
  catch (const String message)
  {
    Serial.println(message);
  }
  catch (const char *message)
  {
    Serial.println(message);
  }
  stopWiFi();

  deepSleep();
}

void loop() {}
