#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

byte localAddress = 0xA2;     // address of this device
//ID:6-1=0xA1;
//ID:6-2=0xA2;
byte gatawayAddress = 0xFF;      // Gataway Address受信機

int callback_count = 2;//再送要求の回数

unsigned long timeout = 1000;//再送要求のタイムアウト

/* シリアル,LoRa通信の開始 */
void setup() {
 Serial1.begin(9600);
  Serial.begin(9600);

  /*
  //シリアルモニタを開かずとも通信は開始される仕様
  while (!Serial);
  Serial.println("LoRa Sender");
 */

  if (!LoRa.begin(920E6)) { // 日本はAS920-923 (“AS1”)の周波数帯を用いる
    //Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
 if (Serial1.available() > 0) { // 受信したデータが存在する
    String data = Serial1.readStringUntil('\n');
    delay(10);
    data.trim();
    Serial.println(data);
    if ((data.indexOf("{") == -1) || (data.indexOf("}") == -1)) { //先頭"{" 後"}"がない時再送要求
      Serial1.println("re");//再送要求
      delay(1);
    }
    else {
      Serial1.println("ok");//成功
      //LoRa send
      int count_num = 0;//再送した回数のカウント
      while (callback_count >= count_num) {
        LoRa.beginPacket();
        //パケットのヘッダにgatawayアドレスとデバイスのアドレスをつける
        LoRa.write(gatawayAddress);
        LoRa.write(localAddress);
        LoRa.print(data);//json文字列
        LoRa.endPacket();

        //返信待ち
        int packetSize;
        unsigned long base_time = millis();
        while (timeout > millis() - base_time ) {
          packetSize = LoRa.parsePacket();
          if (packetSize > 0) {
            break;
          }
        }

        if (localAddress && LoRa.read()) { //自分向けに送信されたものかチェック
          String message;
          for (int i = 0; i < packetSize - 1; i++) {
            message += (char)LoRa.read();
          }
          if (message == "re") {
            count_num++;
          }
          else {
            break;//okの場合はぬける
          }
        }
      }

      /*String ver = Serial.readStringUntil("\n");//ウェアラブル1 設置型6
        if(ver == "6"){//設置型
        String id = Serial.readStringUntil("\n");//id
        String date_time =Serial.readStringUntil("\n");//date_time
        String uv_t = Serial.readStringUntil("\n");//top
        String uv_u = Serial.readStringUntil("\n");//under
        String uv_n = Serial.readStringUntil("\n");//north
        String uv_e = Serial.readStringUntil("\n");//east
        String uv_s = Serial.readStringUntil("\n");//south
        String uv_w = Serial.readStringUntil("\n");//west


        // json

        doc["id"] = ver + "-" + id;//id ver-idで示す
        doc["date_time"] = date_time;
        //uv_date top under north east south west
        doc["uv_t"] = uv_t;
        doc["uv_u"] = uv_u;
        doc["uv_n"] = uv_n;
        doc["uv_e"] = uv_e;
        doc["uv_s"] = uv_s;
        doc["uv_w"] = uv_w;

        }

        if(ver == "1"){//ウェアラブル
        }
      */
      /*
        //sample
        doc["id"] = "6-1";//id ver-idで示す
        doc["date_time"] = "12/13/2020-01:02:03";
        //uv_date top under north east south west
        doc["uv_t"] = "2.12";
        doc["uv_u"] = "0.12";
        doc["uv_n"] = "1.10";
        doc["uv_e"] = "1.23";
        doc["uv_s"] = "1.56";
        doc["uv_w"] = "1.98";
      */
      // send packet
      //String data;
      //serializeJson(doc, data);
      //LoRa.beginPacket();
      //LoRa.print(data);
      //LoRa.endPacket();
      //Serial.println(data);
      //StaticJsonDocument<150> doc;
      //delay(3000);
      //}
    }
  }
  delay(1);
}
