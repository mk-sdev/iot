#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "HH71V1_605F_2.4G";
const char* password = "umGKtAby";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=50&longitude=20&current=temperature_2m");
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("HTTP Response:");
      Serial.println(payload);

      StaticJsonDocument<1024> doc;  // rozmiar w bajtach
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        float temp = doc["current"]["temperature_2m"];
        Serial.print("Temperature 2m: ");
        Serial.println(temp);
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

void loop() {
  
}