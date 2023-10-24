#include <Arduino.h>
#include <M5StickCPlus.h>

//GPS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;
#include <TimeLib.h>   // https://github.com/PaulStoffregen/Time GPSのUSTをJSTに変換
const int offset = 9;  //UTCをJSTに変換する

bool timeset = false;


void setup() {
M5.begin();
  Serial2.begin(9600, SERIAL_8N1, 32, 33);
  M5.Lcd.setRotation(1);

  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);   //文字表示の左上位置を設定
  M5.Lcd.setTextColor(RED);  //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, BLUE, YELLOW...)
  M5.Lcd.setTextSize(3);     //文字の大きさを設定（1～7）
}

void loop() {
  M5.update();
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);

  // if (timeset == true) {
  //   String date_data[2];
  //   samp_time(date_data);
  //   M5.lcd.println(createDataString(date_data));
  // }
}
