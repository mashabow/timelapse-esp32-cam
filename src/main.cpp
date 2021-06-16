#include <Arduino.h>
#include <esp_camera.h>
#include "network.h"

// 撮影間隔 [ms]
const int INTERVAL = 10 * 60 * 1000;
// カメラ起動時後、ホワイトバランスが安定するまでに待つ時間 [ms]
const int CAMERA_WAIT = 20 * 1000;

// 最後に撮影・送信に成功した際の、撮影時刻の Unix time [ms]
// deep sleep しても値は保持される
RTC_DATA_ATTR long long lastCapturedAt = 0;

const int LED_BUILTIN = 4;

// 現在の Unix time をミリ秒単位で返す
long long getCurrentMSec()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
}

// 撮影間隔がちょうど INTERVAL [ms] になるように、delay を挟む
void waitUntilNextCaptureTime(const long long lastCapturedAt)
{
  if (!lastCapturedAt)
    return;

  // 前回の撮影・送信に失敗していた場合も、撮影間隔が INTERVAL の倍数になるようにする
  const int wait = INTERVAL - (getCurrentMSec() - lastCapturedAt) % INTERVAL;
  Serial.println("Waiting until the next capture time... (" + String(wait) + " ms)");
  delay(wait);
}

// deep sleep して終了。wake 時には setup から始まる
void deepSleep()
{
  // 内蔵 LED を完全にオフにする
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  gpio_hold_en((gpio_num_t)LED_BUILTIN);
  gpio_deep_sleep_hold_en();

  Serial.println("Enter deep sleep mode.");
  // WiFi 接続や送信処理などにかかる時間と、RTC の誤差を吸収するための余裕時間
  const int margin = 15 * 1000; // [ms]
  ESP.deepSleep((INTERVAL - CAMERA_WAIT - margin) * 1000);
  delay(1000); // deep sleep が始まるまで待つ
}

// カメラを初期化する
void setupCamera()
{
  Serial.println("Initializing camera...");

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
    throw "Failed to initialize camera with error 0x" + String(err, HEX);
  }

  // 各項目の意味についてはリンク先参照
  // https://github.com/mashabow/timelapse-esp32-cam/issues/4#issuecomment-850776836
  const auto sensor = esp_camera_sensor_get();
  sensor->set_aec2(sensor, 1);
  sensor->set_vflip(sensor, 1);
  sensor->set_hmirror(sensor, 1);
  sensor->set_dcw(sensor, 0);

  // ホワイトバランスが安定するまで待つ
  delay(CAMERA_WAIT);

  Serial.println("Initialized.");
}

// 画像を撮影する
const camera_fb_t *captureImage()
{
  const auto image = esp_camera_fb_get();
  Serial.println("Captured! (" + String(image->len) + " bytes)");
  return image;
}

// 2021-05-29T13-16-07.jpeg のような形式のファイル名を返す
const String toFilename(long long unixTimeMSec)
{
  const time_t unixTimeSec = unixTimeMSec / 1000;
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H-%M-%S.jpeg", localtime(&unixTimeSec));
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

    waitUntilNextTime(lastCapturedAt);
    const auto capturedAt = getCurrentMSec();
    const auto image = captureImage();
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
