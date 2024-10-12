#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Koneksi Wi-Fi
const char* ssid = "P";
const char* password = "hxlnc@91";

// Informasi endpoint API Flask
const char* serverName = "http://10.163.129.118:5000/data"; // Ganti IP dengan IP server Flask

// Pin untuk DHT dan Relay
#define DHTPIN 15      // Pin untuk DHT11
#define DHTTYPE DHT11  // Tipe sensor DHT
#define RELAY_PIN 5    // Pin untuk relay pompa
#define LIGHT_CONTROL_PIN 16 // Pin untuk mengontrol lampu

// Pin configuration for TFT
#define TFT_CS     4
#define TFT_RST    26   // Or set to -1 and connect to ESP32 RESET pin
#define TFT_DC     27

// Color definitions
#define BLACK   ST77XX_BLACK
#define RED     ST77XX_RED
#define GREEN   ST77XX_GREEN
#define BLUE    ST77XX_BLUE
#define WHITE   ST77XX_WHITE
#define YELLOW  ST77XX_YELLOW

// Initialize the TFT display
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Inisialisasi NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000); // 25200 adalah offset waktu untuk GMT+7 (WIB), refresh setiap 60 detik

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Set relay sebagai output
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LIGHT_CONTROL_PIN, OUTPUT); // Set pin untuk lampu
  digitalWrite(RELAY_PIN, HIGH); // Pompa off (HIGH = OFF)
  digitalWrite(LIGHT_CONTROL_PIN, LOW); // Lampu off (LOW = OFF)

  // Inisialisasi TFT
  tft.initR(INITR_BLACKTAB); // Initialize ST7735R chip
  tft.fillScreen(BLACK); // Fill screen with black

  // Set text size and color for TFT
  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  // Display static title on TFT
  tft.setCursor(10, 10);
  tft.print("Monitoring Suhu & Kelembapan");

  // Koneksi Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Inisialisasi NTPClient
  timeClient.begin();
}

void loop() {
  // Perbarui waktu dari NTP
  timeClient.update();

  if(WiFi.status() == WL_CONNECTED) {
    // Membaca suhu dan kelembapan
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Menampilkan data ke layar TFT
    displayData(temp, hum);

    // Membuat payload JSON
    String jsonData = "{\"temperature\": " + String(temp) + ", \"humidity\": " + String(hum) + "}";
    Serial.println("Sending data: " + jsonData);

    // Inisialisasi HTTPClient
    HTTPClient http;
    http.begin(serverName); // Endpoint API Flask
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

  // Cek apakah ada perintah dari Arduino melalui serial
    // Cek apakah ada perintah dari Arduino melalui serial

  delay(1000); // Kirim data setiap 1 detik
}

// Function to display data on TFT screen
void displayData(float temperature, float humidity) {
  // Clear previous readings
  tft.fillRect(0, 30, 128, 128, BLACK);

  // Draw a separator line
  tft.drawLine(0, 30, 128, 30, WHITE);

  // Display temperature
  tft.setCursor(25, 40);
  tft.setTextColor(YELLOW);
  tft.setTextSize(1.5);
  tft.print("Suhu: ");
  tft.print(temperature);
  tft.print("C");

  // Display humidity
  tft.setCursor(5, 55);
  tft.setTextColor(GREEN);
  tft.setTextSize(1.5);
  tft.print("Kelembapan: ");
  tft.print(humidity);
  tft.print(" %");

  // Get the current time and date from NTPClient
  unsigned long epochTime = timeClient.getEpochTime();
  String timeString = timeClient.getFormattedTime();
  String dateString = getFormattedDate(epochTime);

  // Display time
  tft.setCursor(20, 70);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("Waktu: ");
  tft.print(timeString);

  // Display date
  tft.setCursor(10, 80);
  tft.print("Tanggal: ");
  tft.print(dateString);
}

// Function to format the date from epoch time
String getFormattedDate(unsigned long epochTime) {
  // Konversi epoch time ke tanggal
  time_t rawTime = epochTime;
  struct tm *timeinfo;
  timeinfo = localtime(&rawTime);

  String dayStr = String(timeinfo->tm_mday);
  String monthStr = String(timeinfo->tm_mon + 1); // tm_mon is 0-11
  String yearStr = String(timeinfo->tm_year + 1900); // tm_year is years since 1900

  // Tambahkan leading zero untuk tanggal dan bulan
  if (timeinfo->tm_mday < 10) dayStr = "0" + dayStr;
  if (timeinfo->tm_mon + 1 < 10) monthStr = "0" + monthStr;

  return dayStr + "/" + monthStr + "/" + yearStr;
}