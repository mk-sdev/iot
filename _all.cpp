#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "BMP280.h"
#include "Wire.h"

const char* ssid = "HH71V1_605F_2.4G";
const char* password = "umGKtAby";

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* topic_bmp = "test/bmp280";
const char* topic_api = "test/open-meteo";

WiFiClient espClient;
PubSubClient client(espClient);

#define P0 1013.25
BMP280 bmp;

unsigned long lastHttp = 0;
const unsigned long httpInterval = 60000; // 60 sekund

bool connectMQTT() {
  if (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  client.setServer(mqtt_server, mqtt_port);

  if(!bmp.begin()) {
    Serial.println("BMP init failed!");
    while(1);
  } else {
    Serial.println("BMP init success!");
  }
  bmp.setOversampling(4);
}

void loop() {
  if (client.connected()) {
    client.loop();
  } else {
    connectMQTT();
  }

  double T, P;
  char result = bmp.startMeasurment();
  if(result != 0) {
    delay(result);
    result = bmp.getTemperatureAndPressure(T, P);
    if(result != 0) {
      double A = bmp.altitude(P, P0);

      Serial.print("BMP -> T = "); Serial.print(T, 2);
      Serial.print(" degC\tP = "); Serial.print(P, 2);
      Serial.print(" mBar\tA = "); Serial.print(A, 2); Serial.println(" m");

      if(connectMQTT()){
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"T\":%.2f,\"P\":%.2f,\"A\":%.2f}", T, P, A);
        client.publish(topic_bmp, buf);
      }

    } else {
      Serial.println("BMP read error.");
    }
  } else {
    Serial.println("BMP startMeasurment error.");
  }

  if (millis() - lastHttp > httpInterval) {
    lastHttp = millis();

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin("https://api.open-meteo.com/v1/forecast?latitude=50&longitude=20&current=temperature_2m");
      int httpCode = http.GET();

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("HTTP Response:");
        Serial.println(payload);

        // parsowanie JSON 
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          float tempApi = doc["current"]["temperature_2m"];
          Serial.print("API Temperature 2m: ");
          Serial.println(tempApi);

          if(connectMQTT()){
            char tempStr[16];
            dtostrf(tempApi, 4, 1, tempStr);
            client.publish(topic_api, tempStr);
          }

        } else {
          Serial.print("JSON parse error: ");
          Serial.println(error.c_str());
        }

      } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
      }

      http.end();
    }
  }

  delay(100); // małe opóźnienie dla stabilności pętli
}