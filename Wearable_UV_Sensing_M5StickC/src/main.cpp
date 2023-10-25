#include <Arduino.h>
#include <M5StickCPlus.h>

// GPS
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;
#include <TimeLib.h>  // https://github.com/PaulStoffregen/Time GPSのUSTをJSTに変換
const int offset = 9; // UTCをJSTに変換する

bool timeset = false;

/* Prottype */
String createDataString(String date_data[]);
void samp_time(String date_data[]);
void gps_rtc_setting();

void setup()
{
  M5.begin();
  Serial2.begin(115200, SERIAL_8N1, 32, 33);
  M5.Lcd.setRotation(1);

  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(0, 0);   // 文字表示の左上位置を設定
  M5.Lcd.setTextColor(RED); // 文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, BLUE, YELLOW...)
  M5.Lcd.setTextSize(1);    // 文字の大きさを設定（1～7）

  Wire.begin(0, 26); // SDA,SCL
}

void loop()
{

  /* gps */
  // M5.update();
  // M5.Lcd.fillScreen(WHITE);
  // M5.Lcd.setCursor(0, 0);

  // M5.lcd.println("Press M5");

  // if (timeset == true)
  // {
  //   String date_data[2];
  //   samp_time(date_data);
  //   M5.lcd.println(createDataString(date_data));
  // }

  // delay(100);

  // if (M5.BtnA.wasPressed())
  // { // RTC GPS Setting
  //   gps_rtc_setting();
  //   timeset = true;
  // }
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
  // gps_json(String(myGNSS.getLatitude()), String(myGNSS.getLongitude()), date_data);
  delay(5000);
}
