#include <Arduino.h>
#include <esp_camera.h>
#include "network.h"

// 撮影間隔 [s]
const int INTERVAL = 10 * 60;
// カメラ起動時後、ホワイトバランスが安定するまでに待つ時間 [s]
const int CAMERA_WAIT = 30; // 20 ぐらいでもいけるかもしれない

// 最後に撮影・送信に成功した際の、撮影時刻の Unix time [s]
// deep sleep しても値は保持される
RTC_DATA_ATTR time_t lastCapturedAt = 0;

// deep sleep して終了。wake 時には setup から始まる
void deepSleep()
{
  Serial.println("Enter deep sleep mode.");
  // WiFi 接続や送信処理などにかかる時間と、RTC の誤差を吸収するための余裕時間
  const int margin = 15; // [s]
  ESP.deepSleep((INTERVAL - CAMERA_WAIT - margin) * 1000 * 1000);
  delay(1000); // deep sleep が始まるまで待つ
}

// カメラを初期化する
void setupCamera()
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

  // ホワイトバランスが安定するまで待つ
  delay(CAMERA_WAIT * 1000);
}

// 撮影間隔がちょうど INTERVAL [s] になるように、delay を挟んで画像を撮影する
const camera_fb_t *captureImage(const time_t lastCapturedAt)
{
  if (lastCapturedAt)
  {
    // 前回の撮影・送信に失敗していた場合も、撮影間隔が INTERVAL の倍数になるようにする
    const int wait = INTERVAL - (time(NULL) - lastCapturedAt) % INTERVAL;
    Serial.println("Waiting until the next capture time... (" + String(wait) + " seconds)");
    delay(wait * 1000);
  }

  return esp_camera_fb_get();
}

// 撮影時刻を Unix time [s] で返す
const time_t getCapturedAt(const camera_fb_t *image)
{
  const auto deltaSec = millis() / 1000 - image->timestamp.tv_sec;
  return time(NULL) - deltaSec;
}

// 2021-05-29T13-16-07.jpeg のような形式のファイル名を返す
const String toFilename(time_t unixTime)
{
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H-%M-%S.jpeg", localtime(&unixTime));
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
    syncTime();
    setupCamera();

    const auto image = captureImage(lastCapturedAt);
    const auto capturedAt = getCapturedAt(image);
    sendImage(image->buf, image->len, toFilename(capturedAt));

    lastCapturedAt = capturedAt;
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
