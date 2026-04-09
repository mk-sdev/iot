#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "BMP280.h"
#include "Wire.h"
#include <ESP32Ping.h>

WiFiMulti wifiMulti;
BMP280 bmp;

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* MQTT_TOPIC = "PK/PH";

WiFiClient espClient;
PubSubClient client(espClient);

const IPAddress remote_ip(8, 8, 8, 8);

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-Kolos-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // BMP
  if(!bmp.begin()){
    Serial.println("BMP init failed!");
    while(1);
  }
  else Serial.println("BMP init success!");
  bmp.setOversampling(4);

  // WIFI
  wifiMulti.addAP("abcd", "12345678");
  Serial.println("[SETUP] Connecting to WiFi...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SETUP] WiFi connected!");
  Serial.print("[SETUP] IP address: ");
  Serial.println(WiFi.localIP());

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.publish(MQTT_TOPIC, "ESP32 connected");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Ping
  Serial.print("Pinging ip ");
  Serial.println(remote_ip);

  if(Ping.ping(remote_ip)) {
    Serial.println("Success!!");
  } else {
    Serial.println("Error :(");
  }

  // BMP280 temperature
  double T, P;
  char result = bmp.startMeasurment();
  float temp_sensor = NAN;

  if (result != 0) {
    delay(result);
    result = bmp.getTemperatureAndPressure(T, P);
    if (result != 0) {
      temp_sensor = T; // save sensor temperature
      Serial.print("T = \t");Serial.print(T,2); Serial.print(" degC\t"); 
    } else {
      Serial.println("BMP280 read failed");
    }
  } else {
    Serial.println("BMP280 measurement start failed");
  }

  // API temperature
  float temp_api = NAN;

  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("[HTTP] Sending GET request to weather API...");
    HTTPClient http;
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&current=temperature_2m");
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.print("[HTTP] GET response code: ");
      Serial.println(httpCode);

      // GET request
      String payload = http.getString();
      Serial.print("[HTTP] Response payload: ");
      Serial.println(payload);

      // Deseralize response
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        temp_api = doc["current"]["temperature_2m"];
        Serial.print("API Temperature = ");
        Serial.println(temp_api);
      } else {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP GET failed, code: ");
      Serial.println(httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  // MQTT publish
  StaticJsonDocument<128> mqttDoc;
  mqttDoc["temp_sensor"] = temp_sensor;
  mqttDoc["temp_api"] = temp_api;

  String mqttMsg;
  serializeJson(mqttDoc, mqttMsg);

  client.publish(MQTT_TOPIC, mqttMsg.c_str());
  Serial.println("Published JSON: " + mqttMsg);

  delay(10000);
}