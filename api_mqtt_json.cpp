#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- WiFi ---
const char* ssid = "HH71V1_605F_2.4G";
const char* password = "umGKtAby";

// --- MQTT ---
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "test";

WiFiClient espClient;
PubSubClient client(espClient);

// --- Funkcja połączenia z MQTT ---
bool connectMQTT() {
  if (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {  // client ID
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

  // --- Połączenie z WiFi ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // --- MQTT setup ---
  client.setServer(mqtt_server, mqtt_port);

  // --- HTTP request ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://api.open-meteo.com/v1/forecast?latitude=50&longitude=20&current=temperature_2m");
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("HTTP response:");
      Serial.println(payload);

      // --- Parsowanie JSON ---
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        float temp = doc["current"]["temperature_2m"];
        Serial.print("Temperature 2m: ");
        Serial.println(temp);

        // --- Połączenie z MQTT i wysyłka ---
        if (connectMQTT()) {
          char tempStr[16];
          dtostrf(temp, 4, 1, tempStr);  // float -> string
          client.publish(mqtt_topic, tempStr);
          Serial.println("Temperature wysłana na MQTT!");
        } else {
          Serial.println("Nie udało się połączyć z MQTT");
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

void loop() {
  // Obsługa MQTT loop
  if (client.connected()) {
    client.loop();
  }
}