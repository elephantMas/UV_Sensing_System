#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
//#include <ArduinoJson.h>

byte gatewayAddress = 0xFF;     // Gateway address

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");

  if (!LoRa.begin(920E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.receive();
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  // read packet
  if (packetSize > 0) {
    if (gatewayAddress && LoRa.read()) { //gataway向けに送信されたものかチェック
      byte nodeAddress = LoRa.read(); //nodeのアドレス

      String data;
      for (int i = 0; i < packetSize - 2; i++) {
        data += (char)LoRa.read();
      }

      //返信メッセージ 再送要求[nodeAddresu + "re"] OK[nodeAddresu + "ok"]
      if ((data.indexOf("{") == -1) || (data.indexOf("}") == -1)) { //先頭"{" or 後"}"がない時 再送要求
        LoRa.beginPacket();
        LoRa.write(nodeAddress);
        LoRa.print("re");
        LoRa.endPacket();
      }
      else {
        data.replace("}", ",\"RSSI\":\"");
        data += String(LoRa.packetRssi()) + "\"}";
        Serial.println(data);
        LoRa.beginPacket();
        LoRa.write(nodeAddress);
        LoRa.print("ok");
        LoRa.endPacket();

        //okなのでdataをserial out
        //Serial.println(data);
      }
    }
    else {
      for (int i = 0; i < packetSize; i++) {
        LoRa.read();
      }

      /*
        StaticJsonDocument<150> doc;
        deserializeJson(doc, data);
        const char* id = doc["id"];
        Serial.println(id);

        //print RSSI of packet
        Serial.print("with RSSI");
        Serial.println(LoRa.packetRssi());

        serializeJsonPretty(doc, Serial);
        Serial.println();
      */
    }
  }
}
