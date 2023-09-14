#include <Arduino.h>
#include <SPI.h>
#include <M5StickCPlus.h>
#include <ArduinoJson.h>

//サンプリング周期(秒)
uint8_t sampling = 20;
//int ver = 6;     //ウェアラブル1 設置型6
int uv_num = 6;  //uv_sensor数
int id = 1;

//サンプリング周期(秒)
// uint8_t sampling = 30;
// //int ver = 6;     //ウェアラブル1 設置型6
// int uv_num = 6;  //uv_sensor数
// int id = 2;

//I2C Pahub unit
#include <Wire.h>
#include <SparkFun_I2C_Mux_Arduino_Library.h>  // http://librarymanager/All#SparkFun_I2C_Mux         https://github.com/sparkfun/SparkFun_I2C_Mux_Arduino_Library
const int PaHub_I2C_ADDRESS = 0x71;
QWIICMUX i2cMux;

//GPS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>  //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;
#include <TimeLib.h>   // https://github.com/PaulStoffregen/Time GPSのUSTをJSTに変換
const int offset = 9;  //UTCをJSTに変換する

//uvbclick
#include "guvbc31sm.h"
GUVB sensor;


//温湿度センサー shtc3
#include "SparkFun_SHTC3.h"
SHTC3 mySHTC3;

bool timeset = false;

unsigned long timeout = 500;  //再送要求timeout時間
//sdカードの定義
File file;
String fname;  //ファイル名


void setup() {
  M5.begin();
  //Serial.begin(9600);
  //Serial2.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 32, 33);
  M5.Lcd.setRotation(1);
  //フローティング
  //gpio_pulldown_dis(GPIO_NUM_36);
  //gpio_pullup_dis(GPIO_NUM_36);

  //M5.Lcd.setBrightness(200);
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);   //文字表示の左上位置を設定
  M5.Lcd.setTextColor(RED);  //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, BLUE, YELLOW...)
  M5.Lcd.setTextSize(2);     //文字の大きさを設定（1～7）

  Wire.begin(0, 26);//SDA,SCL
  i2cMux.begin(PaHub_I2C_ADDRESS, Wire);

  //温湿度 shtc3
  mySHTC3.begin();

  //uvbclick
  for (uint8_t i = 0; i <= 5; i++) {
    delay(1);
    i2cMux.setPort(i);
    delay(1);
    // initialize the UVB Sensor with default paramenters
    if (sensor.begin() == false) {
      Serial.println("Cant verify chip ID, check connection?");
    }
    // Sensor ranges
    //sensor.setRange(GUVB_RANGE_X1);
    //sensor.setRange(GUVB_RANGE_X2);
    //sensor.setRange(GUVB_RANGE_X4);
    sensor.setRange(GUVB_RANGE_X8);
    //sensor.setRange(GUVB_RANGE_X16);
    //sensor.setRange(GUVB_RANGE_X32);
    //sensor.setRange(GUVB_RANGE_X64);
    //sensor.setRange(GUVB_RANGE_X128);

    // Sensor integration times
    sensor.setIntegrationTime(GUVB_INT_100MS);
    //sensor.setIntegrationTime(GUVB_INT_200MS);
    //sensor.setIntegrationTime(GUVB_INT_400MS);
    //sensor.setIntegrationTime(GUVB_INT_800MS);
    //uv_offset[i - 3] = sensor.getOffset();
  }
}

void loop() {
  //M5.Lcd.clear();
  M5.update();
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);
  mySHTC3.update();
  M5.lcd.println("Temperature:" + (String)mySHTC3.toDegC());
  M5.lcd.println("Humidity:" + (String)mySHTC3.toPercent());

  if (timeset == true) {
    String date_data[2];
    samp_time(date_data);
    M5.lcd.println(createDataString(date_data));
  }
  delay(100);

  if (M5.BtnA.wasPressed()) {  //RTC GPS Setting
    gps_rtc_setting();
    timeset = true;
  }
  if (M5.BtnB.wasPressed()) {  //測定開始
    //log_file_start();
    //String dataString;
    while (M5.BtnA.wasPressed() == false) {
      M5.update();
      //時間調整
      time_adjust(sampling);
      String date_data[2];
      String uv_data[uv_num];
      samp_time(date_data);
      uv_now(uv_num, uv_data);
      uv_json(date_data, uv_data);

      //sd書込み
      //String dataString = add_uv_date(uv_num, uv_data, createDataString(date_data));  //データ生成
      //writeData(dataString);                                                   //データ書き込み
      //M5.Lcd.setCursor(0, 100); //文字表示の左上位置を設定
      //M5.lcd.print("                                                                ");
      M5.Lcd.fillScreen(WHITE);
      M5.Lcd.setCursor(0, 0);  //文字表示の左上位置を設定
      M5.lcd.println("start");
      //M5.lcd.println(dataString);
      //Serial.println(dataString);

      delay(1000);
    }
    M5.lcd.println("stop");
  }
}

String createDataString(String date_data[]) {  //日付データ
  // create dataString(ex. 2017/6/7,12:20:00)
  //i2cMux.setPort(RTC_CH);
  String dataString = date_data[0];
  dataString += ",";
  dataString += date_data[1];

  return dataString;
}
// String add_uv_date(int uv_num, String uv_data[], String dataString) {
//   //add uvbclick
//   for (uint8_t i = 0; i <= 5; i++) {
//     dataString += ",";
//     dataString += uv_data[i];
//   }
//   //温湿度と(緯度経度)
//   //dataString += ",";
//   //mySHTC3.update();
//   // dataString += String(mySHTC3.toDegC());
//   // dataString += ",";
//   // dataString += String(mySHTC3.toPercent());

//   return dataString;
// }

void samp_time(String date_data[]) {
  date_data[0] = String(year());
  date_data[0] += "-";
  date_data[0] += String(month());
  date_data[0] += "-";
  date_data[0] += String(day());


  date_data[1] = String(hour());
  date_data[1] += ":";
  date_data[1] += String(minute());
  date_data[1] += ":";
  date_data[1] += String(second());
}

String add_uv_date(int uv_num, String uv_data[]) {
  String dataString;
  //add uvbclick
  for (int i = 0; i <= uv_num; i++) {
    dataString += ",";
    dataString += uv_data[i];
  }

  //温湿度と(緯度経度)
  //i2cMux.setPort(ENV2_CH);
  dataString += ",";
  //dataString += String(sht3x.readTemperature());
  dataString += String(mySHTC3.toDegC());
  dataString += ",";
  dataString += String(mySHTC3.toPercent());
  //dataString += ",";
  //dataString += String(gps.location.lat());
  //dataString += ",";
  //dataString += String(gps.location.lng());

  return dataString;
}

// void log_file_start() {  // SD.beginはM5.begin内で処理されているので不要
//   fname = "/";
//   //ファイル名を時刻にする
//   String dataString = String(year());
//   dataString += "_";
//   dataString += String(month());
//   dataString += "_";
//   dataString += String(day());
//   dataString += "_";
//   dataString += String(hour());
//   dataString += "_";
//   dataString += String(minute());
//   dataString += "_";
//   dataString += String(second());
//   fname += dataString;
//   fname += ".csv";

//   //1行目
//   dataString = "lat";
//   dataString += ",";
//   dataString += String(myGNSS.getLatitude());
//   dataString += ",";
//   dataString += "lon";
//   dataString += ",";
//   dataString += String(myGNSS.getLongitude());
//   writeData(dataString);

//   //2行目
//   dataString = "data";
//   dataString += ",";
//   dataString += "time";
//   dataString += ",";
//   dataString += "Davis_uv";
//   dataString += ",";
//   dataString += "click_t";
//   dataString += ",";
//   dataString += "click_u";
//   dataString += ",";
//   dataString += "click_n";
//   dataString += ",";
//   dataString += "click_e";
//   dataString += ",";
//   dataString += "click_s";
//   dataString += ",";
//   dataString += "click_w";
//   dataString += ",";
//   dataString += "temp";
//   dataString += ",";
//   dataString += "humid";
//   //dataString += ",";
//   //dataString += "lat";
//   //dataString += ",";
//   //dataString += "lng";
//   // dataString += ",";
//   // dataString += String(myGNSS.getLatitude());
//   // dataString += ",";
//   // dataString += String(myGNSS.getLongitude());
//   writeData(dataString);
// }

// void writeData(String dataString) {
//   // SDカードへの書き込み処理（ファイル追加モード）
//   file = SD.open(fname, FILE_APPEND);
//   file.println(dataString);
//   file.close();
// }
void gps_rtc_setting() {
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);  //文字表示の左上位置を設定
  M5.lcd.print("GPS loding...");

  //i2cMux.setPort(GPS_CH);
  if (myGNSS.begin() == false)  //Connect to the u-blox module using Wire port
  {
    //Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  myGNSS.setI2COutput(COM_TYPE_UBX);  //Set the I2C port to output UBX only (turn off NMEA noise)
  //myGNSS.saveConfiguration();        //Optional: Save the current settings to flash and BBR

  //time set up loop
  while (myGNSS.getTimeValid() == false) {
    //Serial.println("not");
    delay(1000);
  }
  //GPSの時刻UTCをJSTに変換
  setTime((int)myGNSS.getHour(), (int)myGNSS.getMinute(), (int)myGNSS.getSecond(), (int)myGNSS.getDay(), (int)myGNSS.getMonth(), (int)myGNSS.getYear());  //hour,minute,second,day,month,year
  adjustTime(offset * SECS_PER_HOUR);                                                                                                                     //システム時刻を変換

  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);
  //Serial.printf("lat: %f, lng: %f time: %u\r\n", gps.location.lat(), gps.location.lng(), gps.time.value());
  //M5.Lcd.printf("lat: %f, lng: %f time: r\n", gps.location.lat(), gps.location.lng());
  M5.Lcd.print("lat:");
  M5.Lcd.println(String(myGNSS.getLatitude()));
  M5.Lcd.print(" lng:");
  M5.Lcd.println(String(myGNSS.getLongitude()));
  String date_data[2];
  samp_time(date_data);
  M5.Lcd.println(createDataString(date_data));
  gps_json(String(myGNSS.getLatitude()), String(myGNSS.getLongitude()), date_data);
  delay(3000);
}
void time_adjust(uint8_t sampling) {
  while (second() % sampling != 0) {
    delay(1);
  }
}
void uv_now(int uv_num, String uv_data[]) {
  //String uv_data[uv_num];
  for (uint8_t i = 0; i <= 5; i++) {
    delay(1);
    i2cMux.setPort(i);
    delay(1);
    uv_data[i] = String(sensor.readUVB());
    //uv_data[i] = "5276";
    Serial.println(uv_data[i]);
  }
}

void uv_json(String date_data[], String uv_data[]) {
  StaticJsonDocument<260> doc;
  doc["id"] = String(id);
  doc["func"] = "uv";
  doc["date"] = date_data[0];
  doc["time"] = date_data[1];
  //uv_date top under north east south west
  doc["uv_t"] = uv_data[0];
  doc["uv_u"] = uv_data[1];
  doc["uv_n"] = uv_data[2];
  doc["uv_e"] = uv_data[3];
  doc["uv_s"] = uv_data[4];
  doc["uv_w"] = uv_data[5];
  mySHTC3.update();
  doc["temp"] = String(mySHTC3.toDegC());
  doc["humid"] = String(mySHTC3.toPercent());
  String data;
  serializeJson(doc, data);
  //idで1ごとにdelayで送信を遅らせる
  delay((id - 1) * 2000);//2000ミリ秒遅らせる
  //uartで送信
  Serial.println(data);
  Serial2.println(data);
  unsigned long base_time = millis();
  while (timeout > (millis() - base_time)) {  //返信待ち
    if (Serial2.available() > 0) {
      String reply = Serial2.readStringUntil('\n');
      if (reply == "re") {
        Serial2.println(data);
      } else {
        break;
      }
    }
  }
}
void gps_json(String lat, String lon, String date_data[]) {
  StaticJsonDocument<140> doc;
  doc["id"] = String(id);
  doc["func"] = "gps";
  doc["date"] = date_data[0];
  doc["time"] = date_data[1];
  //uv_date top under north east south west
  doc["lat"] = lat;
  doc["lon"] = lon;
  String data;
  serializeJson(doc, data);
  //idで1ごとにdelayで送信を遅らせる
  delay((id - 1) * 1000);
  //uartで送信
  Serial.println(data);
  Serial2.println(data);
  unsigned long base_time = millis();
  while (timeout > (millis() - base_time)) {  //返信待ち
    if (Serial2.available() > 0) {
      String reply = Serial2.readStringUntil('\n');
      if (reply == "re") {
        Serial2.println(data);
      } else {
        break;
      }
    }
  }
}
