#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WebServer.h>
#include <ESPmDNS.h>

#include "DevRelay.h"
#include "DevSwitch.h"
#include "DashboardPage.h"

// Pin assignment ตาม blueprint.md
constexpr uint8_t PIN_RELAY1 = 17;  // Active Low
constexpr uint8_t PIN_RELAY2 = 16; // Active Low
constexpr uint8_t PIN_RELAY3 = 4;   // Active Low
constexpr uint8_t PIN_SW1 = 34;     // Active Low, pull-up ภายนอก
constexpr uint8_t PIN_SW2 = 35;
constexpr uint8_t PIN_SW3 = 32;

DevRelay relay1(PIN_RELAY1);
DevRelay relay2(PIN_RELAY2);
DevRelay relay3(PIN_RELAY3);
DevSwitch sw1(PIN_SW1);
DevSwitch sw2(PIN_SW2);
DevSwitch sw3(PIN_SW3);

// OLED SSD1306 128x64 (I2C: SDA=GPIO21, SCL=GPIO22)
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t SCREEN_ADDRESS = 0x3C;
constexpr unsigned long OLED_INTERVAL_MS = 200;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool oledReady = false;
unsigned long lastOled = 0;

// WiFi Manager: กด SW1 ค้างตอนบูตเพื่อ reset ค่า WiFi
constexpr unsigned long WIFI_RESET_HOLD_MS = 5000;
constexpr unsigned long WIFI_RESET_WINDOW_MS = 3000;  // ช่วงรอกด SW1 หลังบูต
constexpr const char* AP_NAME = "ESP32-Setup";

void showMessage(const char* l1, const char* l2 = "", const char* l3 = "", const char* l4 = "") {
  if (!oledReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  const char* lines[] = {l1, l2, l3, l4};
  for (uint8_t i = 0; i < 4; i++) {
    display.setCursor(0, i * 16);
    display.print(lines[i]);
  }
  display.display();
}

// ถ้า SW1 ถูกกดตอนเริ่มทำงาน ให้นับถอยหลัง 5 วินาที (ปล่อยก่อน = ยกเลิก)
// return true เมื่อกดค้างครบ 5 วินาที
bool checkWifiResetHold() {
  // ช่วงรอให้ผู้ใช้กด SW1 หลังบูต
  unsigned long waitStart = millis();
  while (!sw1.readRawState()) {
    if (millis() - waitStart >= WIFI_RESET_WINDOW_MS) return false;
    showMessage("Hold SW1 5s", "to reset WiFi");
    delay(20);
  }
  unsigned long start = millis();
  int lastShown = -1;
  while (sw1.readRawState()) {
    unsigned long elapsed = millis() - start;
    if (elapsed >= WIFI_RESET_HOLD_MS) return true;
    int remain = (WIFI_RESET_HOLD_MS - elapsed + 999) / 1000;
    if (remain != lastShown) {
      lastShown = remain;
      char buf[24];
      snprintf(buf, sizeof(buf), "Reset in %d s", remain);
      showMessage("RESET WIFI", buf, "Release to cancel");
      Serial.println(buf);
    }
    delay(20);
  }
  showMessage("Reset cancelled");
  delay(1000);
  return false;
}

void onConfigPortal(WiFiManager* wm) {
  showMessage("WiFi Setup Mode", "Connect to AP:", AP_NAME, WiFi.softAPIP().toString().c_str());
}

void setupWifi() {
  WiFiManager wm;
  if (checkWifiResetHold()) {
    wm.resetSettings();
    showMessage("WiFi reset done", "Restarting setup...");
    Serial.println("WiFi settings cleared");
    delay(1500);
  }
  wm.setAPCallback(onConfigPortal);
  showMessage("Connecting WiFi...");
  if (!wm.autoConnect(AP_NAME)) {
    showMessage("WiFi failed", "Restarting...");
    delay(2000);
    ESP.restart();
  }
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void drawRelayRow(uint8_t row, const char* name, const DevRelay& r) {
  int16_t y = 16 + row * 16;
  display.setCursor(0, y);
  display.print(name);
  display.setCursor(64, y);
  display.print(r.getState() ? "ON" : "OFF");
  if (r.getState()) {
    display.fillCircle(116, y + 3, 4, SSD1306_WHITE);
  } else {
    display.drawCircle(116, y + 3, 4, SSD1306_WHITE);
  }
}

void drawRelayPage() {
  display.setCursor(0, 0);
  display.print("IP ");
  display.print(WiFi.localIP());
  display.drawLine(0, 11, SCREEN_WIDTH - 1, 11, SSD1306_WHITE);
  drawRelayRow(0, "Relay1", relay1);
  drawRelayRow(1, "Relay2", relay2);
  drawRelayRow(2, "Relay3", relay3);
}

// ---------- OpenWeatherMap ----------
// API key อ่านจากไฟล์ .env (OWM_API_KEY=...) ผ่าน scripts/load_env.py
#ifndef OWM_API_KEY
#error "OWM_API_KEY is not defined - create .env from .env.example"
#endif
const char* WEATHER_CITY = "Nonthaburi";                  // จังหวัด (ชื่อภาษาอังกฤษ)
const char* WEATHER_COUNTRY = "TH";
constexpr unsigned long WEATHER_INTERVAL_MS = 10UL * 60UL * 1000UL;
constexpr unsigned long PAGE_INTERVAL_MS = 4000;

struct WeatherData {
  bool valid = false;
  float temp = 0;   // °C
  int hum = 0;      // %
  float pm25 = 0;   // ug/m3
  int aqi = 0;      // 1-5 (OWM index)
  int rainPop = 0;  // % โอกาสฝนตก (พยากรณ์ช่วงถัดไป)
};
WeatherData weather;
unsigned long lastWeather = 0;
bool weatherFetched = false;

bool httpGetJson(const String& url, JsonDocument& doc) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    ok = !deserializeJson(doc, http.getStream());
  } else {
    Serial.printf("HTTP error %d for %s\n", code, url.c_str());
  }
  http.end();
  return ok;
}

bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;
  const String base = "https://api.openweathermap.org/data/2.5/";
  const String key = String("&appid=") + OWM_API_KEY;
  WeatherData w;
  JsonDocument doc;

  // 1) อุณหภูมิ / ความชื้น + พิกัดของจังหวัด
  if (!httpGetJson(base + "weather?units=metric&q=" + WEATHER_CITY + "," + WEATHER_COUNTRY + key, doc)) return false;
  w.temp = doc["main"]["temp"];
  w.hum = doc["main"]["humidity"];
  float lat = doc["coord"]["lat"];
  float lon = doc["coord"]["lon"];
  String pos = "lat=" + String(lat, 4) + "&lon=" + String(lon, 4);

  // 2) PM2.5 / AQI
  doc.clear();
  if (!httpGetJson(base + "air_pollution?" + pos + key, doc)) return false;
  w.aqi = doc["list"][0]["main"]["aqi"];
  w.pm25 = doc["list"][0]["components"]["pm2_5"];

  // 3) โอกาสฝนตก (pop) ของช่วงพยากรณ์ถัดไป
  doc.clear();
  if (!httpGetJson(base + "forecast?cnt=1&" + pos + key, doc)) return false;
  w.rainPop = (int)((float)doc["list"][0]["pop"] * 100 + 0.5f);

  w.valid = true;
  weather = w;
  Serial.printf("%s: T=%.1fC H=%d%% PM2.5=%.1f AQI=%d Rain=%d%%\n", WEATHER_CITY, w.temp, w.hum, w.pm25, w.aqi, w.rainPop);
  return true;
}

const char* aqiText(int aqi) {
  switch (aqi) {
    case 1: return "Good";
    case 2: return "Fair";
    case 3: return "Moderate";
    case 4: return "Poor";
    case 5: return "Very Poor";
    default: return "-";
  }
}

void drawWeatherPage() {
  display.setCursor(0, 0);
  display.print(WEATHER_CITY);
  display.drawLine(0, 11, SCREEN_WIDTH - 1, 11, SSD1306_WHITE);
  if (!weather.valid) {
    display.setCursor(0, 20);
    display.print(weatherFetched ? "Weather error" : "Loading...");
    return;
  }
  display.setCursor(0, 14);
  display.printf("Temp %.1f C  Hum %d%%", weather.temp, weather.hum);
  display.setCursor(0, 26);
  display.printf("PM2.5 %.1f ug/m3", weather.pm25);
  display.setCursor(0, 38);
  display.printf("AQI %d (%s)", weather.aqi, aqiText(weather.aqi));
  display.setCursor(0, 50);
  display.printf("Rain chance %d%%", weather.rainPop);
}

void updateOled() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if ((millis() / PAGE_INTERVAL_MS) % 2 == 0) {
    drawRelayPage();
  } else {
    drawWeatherPage();
  }
  display.display();
}

// ---------- Web Dashboard ----------
// เปิดจากเบราว์เซอร์ในวง LAN เดียวกัน: http://<IP ที่แสดงบน OLED> หรือ http://esp32.local
WebServer server(80);
constexpr const char* MDNS_NAME = "esp32";

DevRelay* relayById(int id) {
  switch (id) {
    case 1: return &relay1;
    case 2: return &relay2;
    case 3: return &relay3;
    default: return nullptr;
  }
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", DASHBOARD_HTML);
}

void handleStatus() {
  JsonDocument doc;
  JsonArray relays = doc["relays"].to<JsonArray>();
  for (int id = 1; id <= 3; id++) {
    JsonObject r = relays.add<JsonObject>();
    r["id"] = id;
    r["on"] = relayById(id)->getState();
  }

  JsonObject w = doc["weather"].to<JsonObject>();
  w["city"] = WEATHER_CITY;
  w["valid"] = weather.valid;
  w["fetched"] = weatherFetched;
  w["temp"] = weather.temp;
  w["hum"] = weather.hum;
  w["pm25"] = weather.pm25;
  w["aqi"] = weather.aqi;
  w["rain"] = weather.rainPop;
  w["age"] = (millis() - lastWeather) / 1000;

  JsonObject f = doc["wifi"].to<JsonObject>();
  f["ssid"] = WiFi.SSID();
  f["rssi"] = WiFi.RSSI();
  f["ip"] = WiFi.localIP().toString();
  f["gateway"] = WiFi.gatewayIP().toString();
  f["mac"] = WiFi.macAddress();
  doc["uptime"] = millis() / 1000;

  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

// POST /api/relay?id=1&state=on|off|toggle
void handleRelay() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"use POST\"}");
    return;
  }
  DevRelay* r = relayById(server.arg("id").toInt());
  String state = server.arg("state");
  if (!r || (state != "on" && state != "off" && state != "toggle")) {
    server.send(400, "application/json", "{\"error\":\"bad id or state\"}");
    return;
  }
  if (state == "toggle") r->toggle();
  else r->setState(state == "on");
  server.send(200, "application/json", String("{\"on\":") + (r->getState() ? "true" : "false") + "}");
}

void setupWebServer() {
  if (MDNS.begin(MDNS_NAME)) MDNS.addService("http", "tcp", 80);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/relay", handleRelay);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.printf("Dashboard: http://%s  (http://%s.local)\n", WiFi.localIP().toString().c_str(), MDNS_NAME);
}

// กดสวิตช์ 1 ครั้ง = สลับ ON <-> OFF ของ relay ที่คู่กัน
void onSw2Press() { relay2.toggle(); }
void onSw3Press() { relay3.toggle(); }

void setup() {
  Serial.begin(9600);

  relay1.begin();
  relay2.begin();
  relay3.begin();

  sw1.begin();
  sw2.begin();
  sw3.begin();
  sw2.onPress(onSw2Press);
  sw3.onPress(onSw3Press);

  Wire.begin(21, 22);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!oledReady) Serial.println("OLED init failed");

  setupWifi();
  setupWebServer();

  showMessage("Loading weather...");
  fetchWeather();
  weatherFetched = true;
  lastWeather = millis();
}

// SW1: กดสั้น (< 1 วินาที) = สลับ Relay1, กดค้างครบ 5 วินาที = reset WiFi
constexpr unsigned long SW1_SHORT_PRESS_MS = 1000;
bool sw1Tracking = false;
bool sw1Counting = false;
unsigned long sw1PressStart = 0;

void handleSw1() {
  if (sw1.wasPressed()) {
    sw1Tracking = true;
    sw1Counting = false;
    sw1PressStart = millis();
  }
  if (!sw1Tracking) return;

  unsigned long held = millis() - sw1PressStart;
  if (sw1.isPressed()) {
    if (held >= SW1_SHORT_PRESS_MS) {
      sw1Counting = true;
      if (held >= WIFI_RESET_HOLD_MS) {
        showMessage("WiFi reset done", "Restarting...");
        WiFiManager wm;
        wm.resetSettings();
        delay(1500);
        ESP.restart();
      }
      char buf[24];
      snprintf(buf, sizeof(buf), "Reset in %lu s", (WIFI_RESET_HOLD_MS - held + 999) / 1000);
      showMessage("RESET WIFI", buf, "Release to cancel");
    }
  } else {
    if (!sw1Counting) relay1.toggle();  // กดสั้น
    sw1Tracking = false;
    sw1Counting = false;
  }
}

void loop() {
  server.handleClient();
  sw1.update();
  handleSw1();
  sw2.update();
  sw3.update();

  if (millis() - lastWeather >= WEATHER_INTERVAL_MS) {
    lastWeather = millis();
    fetchWeather();
  }

  if (oledReady && !sw1Counting && millis() - lastOled >= OLED_INTERVAL_MS) {
    lastOled = millis();
    updateOled();
  }
}
