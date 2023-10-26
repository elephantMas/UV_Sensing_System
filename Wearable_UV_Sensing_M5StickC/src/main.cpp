#include <Arduino.h>
#include <M5StickCPlus.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi credentials.
// const char *WIFI_SSID = "CPSLAB_WLX";
// const char *WIFI_PASS = "6bepa8ideapbu";

#include "esp_wpa2.h"                          //wpa2 library for connections to Enterprise networks
#define EAP_IDENTITY "20FI094"                 // 学籍番号
#define EAP_USERNAME "20FI094"                 // 学籍番号
#define EAP_PASSWORD "Que_sera7760"            // 大学パスワード
const char *ssid = "TDU_MRCL_WLAN_DOT1X_2.4G"; // SSID
int counter = 0;                               // タイムアウト監視用

// MQTT credentials.
const char *brokerAdr = "test.mosquitto.org";
const int brokerPort = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// 基本設定
uint8_t sampling = 30;
int id = 1;
int uv_num = 3;
unsigned long timeout = 500; // 再送要求timeout時間

// マルチプレクサ
#include <SparkFun_I2C_Mux_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_I2C_Mux_Arduino_Library
QWIICMUX myMux;

// GPS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
SFE_UBLOX_GNSS myGNSS;
#include <TimeLib.h>  // https://github.com/PaulStoffregen/Time GPSのUSTをJSTに変換
const int offset = 9; // UTCをJSTに変換する
bool timeset = false;

// UV
#include <SparkFun_VEML6075_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_VEML6075_Arduino_Library
VEML6075 uv;

/* Prottype */
void reconnect();
String createDataString(String date_data[]);
void samp_time(String date_data[]);
void time_adjust(uint8_t sampling);
void uv_now(int uv_num, String uv_data[]);
void uv_json(String date_data[], String uv_data[]);
void gps_rtc_setting();
void gps_json(String lat, String lon, String date_data[]);

void setup()
{
  M5.begin();
  Serial.begin(115200, SERIAL_8N1, 32, 33);
  M5.Lcd.setRotation(1);

  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);   // 文字表示の左上位置を設定
  M5.Lcd.setTextColor(RED); // 文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, BLUE, YELLOW...)
  M5.Lcd.setTextSize(2);    // 文字の大きさを設定（1～7）

  Wire.begin(0, 26); // SDA,SCL

  if (myMux.begin() == false)
  {
    while (1)
      ;
  }
  Serial.println("MQTTMUX Running");

  // 各UVセンサの初期設定
  for (u_int8_t i = 1; i <= uv_num; i++)
  {
    delay(1);
    myMux.setPort(i);
    delay(1);
    if (uv.begin() == false)
    {
      while (1)
        ;
    }

    // Integration time:
    // uv.setIntegrationTime(VEML6075::IT_50MS);
    uv.setIntegrationTime(VEML6075::IT_100MS); // default
    // uv.setIntegrationTime(VEML6075::IT_200MS);
    // uv.setIntegrationTime(VEML6075::IT_400MS);
    // uv.setIntegrationTime(VEML6075::IT_800MS);

    Serial.println("UV " + String(i) + " Running");
  }

  // wifi------------
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Connecting to Network...");
  WiFi.disconnect(true); // disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA);   // init wifi mode

  // Example1 (most common): a cert-file-free eduroam with PEAP (or TTLS)
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 60)
    { // after 30 seconds timeout - reset board
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address set: ");
  Serial.println(WiFi.localIP()); // print LAN IP

  // WiFi接続
  // WiFi.begin(WIFI_SSID, WIFI_PASS);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  // }

  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Start WiFi and Publish");
  mqttClient.setServer(brokerAdr, brokerPort);
  delay(3000);

  // MQTT初期接続
  reconnect();
  mqttClient.publish("/test", "Running");
  delay(1000);
}

void loop()
{
  M5.update();
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.lcd.println("Press M5 to Get GPS Value");

  String date_data[2];
  if (timeset == true)
  {
    samp_time(date_data);
    M5.lcd.println(createDataString(date_data));
    M5.Lcd.println("Press Right Side Button");
  }

  delay(100);

  /* gps */
  if (M5.BtnA.wasPressed())
  { // RTC GPS Setting
    gps_rtc_setting();
    timeset = true;
  }

  if (M5.BtnB.wasPressed())
  {
    while (M5.BtnA.wasPressed() == false)
    {
      M5.update();

      // 時間調整
      time_adjust(sampling);
      String uv_data[uv_num];
      samp_time(date_data);
      uv_now(uv_num, uv_data);
      // UV値送信
      uv_json(date_data, uv_data);

      // 画面表示
      M5.Lcd.fillScreen(WHITE);
      M5.Lcd.setCursor(0, 0);                      // 文字表示の左上位置を設定
      M5.lcd.println(createDataString(date_data)); // 時刻表示
      M5.lcd.println("MQTT Transmission Start");

      // GPS値送信
      gps_json(String(myGNSS.getLatitude()), String(myGNSS.getLongitude()), date_data);

      delay(1000);
    }
    M5.lcd.println("stop");
  }
}

void reconnect()
{
  while (!mqttClient.connected())
  {
    if (!mqttClient.connect("M5StickCPlus"))
    {
      delay(5000);
    }
  }
}

String createDataString(String date_data[])
{ // date_date(日付データ)
  String dataString = date_data[0];
  dataString += ",";
  dataString += date_data[1];

  return dataString;
}

void samp_time(String date_data[])
{
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

void time_adjust(uint8_t sampling)
{
  while (second() % sampling != 0)
  {
    delay(1);
  }
}

void uv_now(int uv_num, String uv_data[])
{
  // String uv_data[uv_num];
  for (uint8_t i = 1; i <= 3; i++)
  {
    delay(1);
    myMux.setPort(i);
    delay(1);
    uv_data[i] = String(uv.uvb());
    // uv_data[i] = "5276";
    Serial.println(uv_data[i]);
  }
}

void uv_json(String date_data[], String uv_data[])
{
  StaticJsonDocument<260> doc;
  doc["id"] = String(id);
  doc["func"] = "uv";
  doc["date"] = date_data[0];
  doc["time"] = date_data[1];
  // uv_date top under north east south west
  doc["uv_t"] = uv_data[0];
  doc["uv_f"] = uv_data[1];
  doc["uv_b"] = uv_data[2];

  char buffer[256];
  serializeJson(doc, buffer);

  // idで1ごとにdelayで送信を遅らせる
  delay((id - 1) * 2000); // 2000ミリ秒遅らせる

  // uartで送信
  Serial.println(buffer);
  mqttClient.publish("/test", buffer);

  unsigned long base_time = millis();
  while (timeout > (millis() - base_time))
  { // 返信待ち
    if (Serial.available() > 0)
    {
      String reply = Serial.readStringUntil('\n');
      if (reply == "re")
      {
        mqttClient.publish("/test", buffer);
      }
      else
      {
        break;
      }
    }
  }
}

void gps_rtc_setting()
{
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0); // 文字表示の左上位置を設定
  M5.lcd.print("GPS loading...");

  // i2cMux.setPort(GPS_CH);
  if (myGNSS.begin() == false) // Connect to the u-blox module using Wire port
  {
    Serial.println(F("u-blox GNSS not detected at default I2C address. Please check wiring. Freezing."));
    while (1)
      ;
  }

  myGNSS.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)
  // myGNSS.saveConfiguration();        //Optional: Save the current settings to flash and BBR

  // time set up loop
  while (myGNSS.getTimeValid() == false)
  {
    Serial.println("not");
    delay(1000);
  }
  // GPSの時刻UTCをJSTに変換
  setTime((int)myGNSS.getHour(), (int)myGNSS.getMinute(), (int)myGNSS.getSecond(), (int)myGNSS.getDay(), (int)myGNSS.getMonth(), (int)myGNSS.getYear()); // hour,minute,second,day,month,year
  adjustTime(offset * SECS_PER_HOUR);                                                                                                                    // システム時刻を変換

  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("lat:");
  M5.Lcd.println(String(myGNSS.getLatitude()));
  M5.Lcd.print(" lng:");
  M5.Lcd.println(String(myGNSS.getLongitude()));
  String date_data[2];
  samp_time(date_data);
  M5.Lcd.println(createDataString(date_data));

  delay(5000);
}

void gps_json(String lat, String lon, String date_data[])
{
  StaticJsonDocument<140> doc;
  doc["id"] = String(id);
  doc["func"] = "gps";
  doc["date"] = date_data[0];
  doc["time"] = date_data[1];
  // uv_date top under north east south west
  doc["lat"] = lat;
  doc["lon"] = lon;

  char buffer[256];
  serializeJson(doc, buffer);

  // idで1ごとにdelayで送信を遅らせる
  delay((id - 1) * 1000);

  // uartで送信
  Serial.println(buffer);
  mqttClient.publish("/test", buffer);

  unsigned long base_time = millis();
  while (timeout > (millis() - base_time))
  { // 返信待ち
    if (Serial2.available() > 0)
    {
      String reply = Serial2.readStringUntil('\n');
      if (reply == "re")
      {
        mqttClient.publish("/test", buffer);
      }
      else
      {
        break;
      }
    }
  }
}
