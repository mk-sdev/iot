#include "BMP280.h"
#include "Wire.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define P0 1013.25

BMP280 bmp;

// WiFi
const char* ssid = "HH71V1_605F_2.4G";
const char* password = "umGKtAby";

// MQTT
const char* server = "test.mosquitto.org";
const int port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// callback (opcjonalny)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  // --- BMP280 ---
  if (!bmp.begin()) {
    Serial.println("BMP init failed!");
    while (1);
  }
  Serial.println("BMP init success!");
  bmp.setOversampling(4);

  // --- WiFi ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  // --- MQTT ---
  client.setServer(server, port);
  client.setCallback(callback);

  connectMQTT();
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("OK");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  double T, P;
  char result = bmp.startMeasurment();

  if (result != 0) {
    delay(result);
    result = bmp.getTemperatureAndPressure(T, P);

    if (result != 0) {
      double A = bmp.altitude(P, P0);

      // --- Serial ---
      Serial.print("T = "); Serial.print(T, 2); Serial.print(" C ");
      Serial.print("P = "); Serial.print(P, 2); Serial.print(" mBar ");
      Serial.print("A = "); Serial.print(A, 2); Serial.println(" m");

      // --- MQTT (wysyłka) ---
      char tempStr[10];
      dtostrf(T, 1, 2, tempStr); // konwersja double -> string

      client.publish("esp32/temperature", tempStr);

    } else {
      Serial.println("Error reading BMP");
    }
  } else {
    Serial.println("Measurement error");
  }

  delay(5000); // co 5 sekund
}