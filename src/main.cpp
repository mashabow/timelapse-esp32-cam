#include <Arduino.h>
#include <esp_camera.h>
#include "upload.h"

/**
 * カメラを初期化し、撮影した画像のフレームバッファを取得する
 */
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
  const auto s = esp_camera_sensor_get();
  Serial.println("{");
  Serial.printf("  framesize: %u,\n", s->status.framesize);
  Serial.printf("  quality: %u,\n", s->status.quality);
  Serial.printf("  brightness: %d,\n", s->status.brightness);
  Serial.printf("  contrast: %d,\n", s->status.contrast);
  Serial.printf("  saturation: %d,\n", s->status.saturation);
  Serial.printf("  sharpness: %d,\n", s->status.sharpness);
  Serial.printf("  special_effect: %u,\n", s->status.special_effect);
  Serial.printf("  wb_mode: %u,\n", s->status.wb_mode);
  Serial.printf("  awb: %u,\n", s->status.awb);
  Serial.printf("  awb_gain: %u,\n", s->status.awb_gain);
  Serial.printf("  aec: %u,\n", s->status.aec);
  Serial.printf("  aec2: %u,\n", s->status.aec2);
  Serial.printf("  ae_level: %d,\n", s->status.ae_level);
  Serial.printf("  aec_value: %u,\n", s->status.aec_value);
  Serial.printf("  agc: %u,\n", s->status.agc);
  Serial.printf("  agc_gain: %u,\n", s->status.agc_gain);
  Serial.printf("  gainceiling: %u,\n", s->status.gainceiling);
  Serial.printf("  bpc: %u,\n", s->status.bpc);
  Serial.printf("  wpc: %u,\n", s->status.wpc);
  Serial.printf("  raw_gma: %u,\n", s->status.raw_gma);
  Serial.printf("  lenc: %u,\n", s->status.lenc);
  Serial.printf("  vflip: %u,\n", s->status.vflip);
  Serial.printf("  hmirror: %u,\n", s->status.hmirror);
  Serial.printf("  dcw: %u,\n", s->status.dcw);
  Serial.printf("  colorbar: %u,\n", s->status.colorbar);
  // GUI から操作できない
  Serial.printf("  sharpness: %d,\n", s->status.sharpness);
  Serial.printf("  denoise: %u,\n", s->status.denoise);
  Serial.println("}");

  return esp_camera_fb_get();
}

/**
 * 2021-05-29T13-16-07 のような形式のタイムスタンプ文字列を返す
 */
const String getTimestamp()
{
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  delay(1000); // NTP による現在時刻取得が完了するのを待つ

  const auto t = time(NULL);
  if (t < 1600000000) // 最近の時刻っぽい値にセットされたか確認
  {
    throw "Failed to sync current time";
  }

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H-%M-%S", localtime(&t));
  return String(buffer);
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  setupWiFi();
  const auto filename = getTimestamp() + ".jpeg";
  const auto image = captureImage();
  sendImage(image->buf, image->len, filename);
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10000);
}
