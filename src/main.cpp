#include <WiFi.h>
#include <ESP32Ping.h>


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
}

void loop() {
  IPAddress ip(8, 8, 8, 8);

  Serial.print("Pinging 8.8.8.8 ... ");
  if (Ping.ping(ip)) {
    Serial.println("Success!");
  } else {
    Serial.println("Failed!");
  }

  delay(2000); // co 2 sekundy ping
}