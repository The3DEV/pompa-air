#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

// Koneksi Wi-Fi
const char* ssid = "P";
const char* password = "hxlnc@91";

// Informasi endpoint API Flask
const char* serverName = "http://10.167.10.3:5000/data"; // Ganti IP dengan IP server Flask

// Pin untuk DHT dan Relay
#define DHTPIN 15      // Pin untuk DHT11
#define DHTTYPE DHT11  // Tipe sensor DHT
#define RELAY_PIN 5    // Pin untuk relay pompa

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // Set relay sebagai output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pompa off (HIGH = OFF)

  // Koneksi Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if(WiFi.status() == WL_CONNECTED) {
    // Membaca suhu dan kelembapan
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    
    if (isnan(temp) || isnan(hum)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Membuat payload JSON
    String jsonData = "{\"temperature\": " + String(temp) + ", \"humidity\": " + String(hum) + "}";
    Serial.println("Sending data: " + jsonData);

    // Inisialisasi HTTPClient
    HTTPClient http;
    http.begin(serverName);   // Endpoint API Flask
    http.addHeader("Content-Type", "application/json"); // Header untuk JSON

    // Kirim data dengan POST request
    int httpResponseCode = http.POST(jsonData);
    
    // Cek hasil response dari server
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response from server: " + response);
    }
    else {
      Serial.println("Error sending POST: " + String(httpResponseCode));
    }
    
    // End HTTP connection
    http.end();
  }
  else {
    Serial.println("WiFi not connected");
  }

  delay(1000);  // Kirim data setiap 5 detik
}