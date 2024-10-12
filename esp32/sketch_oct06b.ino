#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>

// Koneksi Wi-Fi
char ssid[32] = "P";
char password[32] = "hxlnc@91";
char api_server[128] = "http://10.163.129.118:5000/data";

// Pin untuk DHT dan Relay
#define DHTPIN 15
#define DHTTYPE DHT11
#define RELAY_PIN 5
#define LIGHT_CONTROL_PIN 16

// Pin configuration for TFT
#define TFT_CS 4
#define TFT_RST 26
#define TFT_DC 27

// Color definitions
#define BLACK ST77XX_BLACK
#define RED ST77XX_RED
#define GREEN ST77XX_GREEN
#define BLUE ST77XX_BLUE
#define WHITE ST77XX_WHITE
#define YELLOW ST77XX_YELLOW

// Inisialisasi NTPClient dan TFT
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
WebServer server(80);

// Fungsi untuk menyimpan konfigurasi
void saveConfig(const char* newSSID, const char* newPassword, const char* newAPIServer) {
  EEPROM.writeString(0, newSSID);
  EEPROM.writeString(32, newPassword);
  EEPROM.writeString(64, newAPIServer);
  EEPROM.commit();
  Serial.println("Konfigurasi Wi-Fi dan API disimpan ke EEPROM.");
}

// Fungsi untuk memuat konfigurasi
void loadConfig() {
  String savedSSID = EEPROM.readString(0);
  String savedPassword = EEPROM.readString(32);
  String savedAPIServer = EEPROM.readString(64);

  if (savedSSID.length() > 0 && savedPassword.length() > 0) {
    savedSSID.toCharArray(ssid, 32);
    savedPassword.toCharArray(password, 32);
    Serial.println("Konfigurasi Wi-Fi dimuat.");
  }

  if (savedAPIServer.length() > 0) {
    savedAPIServer.toCharArray(api_server, 128);
    Serial.println("Konfigurasi API dimuat.");
  }
}

// Fungsi untuk memulai Access Point
void startAccessPoint() {
  WiFi.softAP("ESP32_AP", "123456789");
  Serial.println("Access Point aktif. IP: ");
  Serial.println(WiFi.softAPIP());
}

// Fungsi untuk memulai Web Server
void startWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
      <html><body>
      <h1>Konfigurasi Wi-Fi dan API</h1>
      <form action="/save" method="POST">
        SSID:<br><input type="text" name="ssid" value=")rawliteral" + String(ssid) + R"rawliteral("><br>
        Password:<br><input type="text" name="password" value=")rawliteral" + String(password) + R"rawliteral("><br>
        API Server:<br><input type="text" name="api_server" value=")rawliteral" + String(api_server) + R"rawliteral("><br>
        <input type="submit" value="Simpan">
      </form></body></html>)rawliteral");
  });

  server.on("/save", HTTP_POST, []() {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    String newAPIServer = server.arg("api_server");

    if (newSSID.length() > 0 && newPassword.length() > 0 && newAPIServer.length() > 0) {
      newSSID.toCharArray(ssid, 32);
      newPassword.toCharArray(password, 32);
      newAPIServer.toCharArray(api_server, 128);

      saveConfig(ssid, password, api_server);
      server.send(200, "text/html", "Konfigurasi tersimpan! Restart...");
      delay(2000);
      ESP.restart();
    } else {
      server.send(200, "text/html", "Input tidak valid.");
    }
  });

  server.begin();
  Serial.println("Web server aktif.");
}

// Fungsi untuk koneksi ke Wi-Fi
void connectToWiFi() {
  Serial.println("Menghubungkan ke Wi-Fi...");
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nTerhubung ke Wi-Fi. IP:");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nGagal menghubungkan ke Wi-Fi.");
    startAccessPoint();
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Set relay dan kontrol lampu sebagai output
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LIGHT_CONTROL_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pompa off (HIGH = OFF)
  digitalWrite(LIGHT_CONTROL_PIN, LOW); // Lampu off (LOW = OFF)

  // Inisialisasi TFT
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(BLACK);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(10, 10);
  tft.print("Monitoring Suhu & Kelembapan");

  // Inisialisasi EEPROM
  EEPROM.begin(192);
  loadConfig();

  // Koneksi ke Wi-Fi
  connectToWiFi();

  // Inisialisasi NTP
  timeClient.begin();

  // Memulai web server
  startWebServer();
}

void loop() {
  // Perbarui waktu dari NTP
  timeClient.update();
  server.handleClient(); // Handle HTTP request

  if (WiFi.status() == WL_CONNECTED) {
    // Membaca suhu dan kelembapan
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (!isnan(temp) && !isnan(hum)) {
      displayData(temp, hum);

      // Buat JSON untuk dikirim ke API
      String jsonData = "{\"temperature\": " + String(temp) + ", \"humidity\": " + String(hum) + "}";
      Serial.println("Mengirim data: " + jsonData);

      // Kirim data ke server API
      HTTPClient http;
      http.begin(api_server);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respon dari server: " + response);
      } else {
        Serial.println("Gagal mengirim data: " + String(httpResponseCode));
      }
      http.end();
    } else {
      Serial.println("Gagal membaca dari sensor DHT.");
    }
  } else {
    Serial.println("WiFi tidak terhubung.");
  }

  delay(1000); // Kirim data setiap 1 detik
}

// Fungsi untuk menampilkan data di layar TFT
void displayData(float temperature, float humidity) {
  tft.fillRect(0, 30, 128, 128, BLACK);
  tft.drawLine(0, 30, 128, 30, WHITE);

  tft.setCursor(25, 40);
  tft.setTextColor(YELLOW);
  tft.setTextSize(1.5);
  tft.print("Suhu: ");
  tft.print(temperature);
  tft.print("C");

  tft.setCursor(5, 55);
  tft.setTextColor(GREEN);
  tft.setTextSize(1.5);
  tft.print("Kelembapan: ");
  tft.print(humidity);
  tft.print(" %");

  // Tampilkan waktu dari NTP
  tft.setCursor(20, 70);
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("Waktu: ");
  tft.print(timeClient.getFormattedTime());

  // Tampilkan tanggal
  tft.setCursor(10, 80);
  tft.print("Tanggal: ");
  tft.print(getFormattedDate(timeClient.getEpochTime()));
}

// Fungsi untuk format tanggal
String getFormattedDate(unsigned long epochTime) {
  time_t rawTime = epochTime;
  struct tm *timeinfo = localtime(&rawTime);

  String dayStr = String(timeinfo->tm_mday);
  String monthStr = String(timeinfo->tm_mon + 1);
  String yearStr = String(timeinfo->tm_year + 1900);

  // Tambahkan leading zero untuk tanggal dan bulan
  if (timeinfo->tm_mday < 10) dayStr = "0" + dayStr;
  if (timeinfo->tm_mon + 1 < 10) monthStr = "0" + monthStr;

  return dayStr + "/" + monthStr + "/" + yearStr;
}
