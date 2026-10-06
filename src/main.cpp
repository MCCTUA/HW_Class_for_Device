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
#include <PubSubClient.h>

#include "DevRelay.h"
#include "DevSwitch.h"
#include "DevDS18B20.h"
#include "DevXYMDSensor.h"
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

constexpr uint8_t PIN_DS18B20 = 14;  // DATA + pull-up 4.7k ไป 3V3
DevDS18B20 ds18b20(PIN_DS18B20);

// XY-MD03 (Modbus RTU, ID 2) ผ่าน Serial0 (UART0: GPIO1 TX / GPIO3 RX) ผ่านวงจรสลับ RS232/RS485
// ห้ามใช้ Serial.print ใน firmware นี้ เพราะ Serial0 คือบัส Modbus
constexpr uint8_t XYMD_SLAVE_ID = 2;
DevXYMDSensor xymd(&Serial, XYMD_SLAVE_ID);

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
    delay(1500);
  }
  wm.setAPCallback(onConfigPortal);
  showMessage("Connecting WiFi...");
  if (!wm.autoConnect(AP_NAME)) {
    showMessage("WiFi failed", "Restarting...");
    delay(2000);
    ESP.restart();
  }
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
    (void)code;
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

// หน้าอุณหภูมิ DS18B20: ตัวเลขใหญ่ + min/max + กราฟประวัติ
void drawTempPage() {
  display.setCursor(0, 0);
  display.print("DS18B20");
  const char* tag = ds18b20.isSimulated() ? "SIM" : "LIVE";
  int16_t tw = strlen(tag) * 6 + 4;
  if (ds18b20.isSimulated()) {
    display.fillRoundRect(SCREEN_WIDTH - tw, 0, tw, 10, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRoundRect(SCREEN_WIDTH - tw, 0, tw, 10, 2, SSD1306_WHITE);
  }
  display.setCursor(SCREEN_WIDTH - tw + 2, 1);
  display.print(tag);
  display.setTextColor(SSD1306_WHITE);

  if (!ds18b20.hasReading()) {
    display.setCursor(0, 28);
    display.print("Reading...");
    return;
  }
  display.setTextSize(3);
  display.setCursor(0, 14);
  display.printf("%.1f", ds18b20.getTemp());
  display.setTextSize(1);
  display.drawCircle(88, 16, 2, SSD1306_WHITE);  // สัญลักษณ์ ° ตัวเล็ก
  display.setTextSize(2);
  display.setCursor(94, 14);
  display.print("C");
  display.setTextSize(1);
  display.setCursor(92, 34);
  display.printf("%.0f", ds18b20.getMax());
  display.setCursor(92, 42);
  display.printf("%.0f", ds18b20.getMin());
  display.setCursor(104, 34);
  display.print("max");
  display.setCursor(104, 42);
  display.print("min");

  // sparkline (y = 40..63, x = 0..87)
  uint8_t n = ds18b20.getHistoryCount();
  display.drawFastHLine(0, 39, 88, SSD1306_WHITE);
  if (n >= 2) {
    float lo = ds18b20.getHistory(0), hi = lo;
    for (uint8_t i = 1; i < n; i++) {
      float v = ds18b20.getHistory(i);
      if (v < lo) lo = v;
      if (v > hi) hi = v;
    }
    float span = max(hi - lo, 1.0f);
    int16_t px = 0, py = 0;
    for (uint8_t i = 0; i < n; i++) {
      int16_t x = i * 87 / (DevDS18B20::HISTORY_SIZE - 1);
      int16_t y = 62 - (int16_t)((ds18b20.getHistory(i) - lo) / span * 20);
      if (i) display.drawLine(px, py, x, y, SSD1306_WHITE);
      px = x;
      py = y;
    }
  }
}

// หน้า XY-MD03: Temp / Hum ตัวใหญ่
void drawXymdPage() {
  display.setCursor(0, 0);
  display.printf("XY-MD03 ID%u", xymd.getSlaveID());
  const char* tag = xymd.isSimulated() ? "SIM" : "LIVE";
  int16_t tw = strlen(tag) * 6 + 4;
  if (xymd.isSimulated()) {
    display.fillRoundRect(SCREEN_WIDTH - tw, 0, tw, 10, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRoundRect(SCREEN_WIDTH - tw, 0, tw, 10, 2, SSD1306_WHITE);
  }
  display.setCursor(SCREEN_WIDTH - tw + 2, 1);
  display.print(tag);
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(0, 11, SCREEN_WIDTH - 1, 11, SSD1306_WHITE);

  if (!xymd.hasReading()) {
    display.setCursor(0, 28);
    display.print("Reading...");
    return;
  }
  display.setTextSize(2);
  display.setCursor(0, 18);
  display.printf("T %.1f C", xymd.getTemperature());
  display.setCursor(0, 42);
  display.printf("H %.1f %%", xymd.getHumidity());
  display.setTextSize(1);
}

void updateOled() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  switch ((millis() / PAGE_INTERVAL_MS) % 4) {
    case 0: drawTempPage(); break;
    case 1: drawXymdPage(); break;
    case 2: drawRelayPage(); break;
    default: drawWeatherPage(); break;
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

// MQTT (นิยามด้านล่าง) ใช้ในหน้า dashboard
void fillMqttStatus(JsonObject m);

// สร้าง JSON สถานะรวมทุกอย่าง ใช้ทั้ง /api/status และ MQTT telemetry
void fillStatus(JsonDocument& doc, bool forDashboard) {
  JsonArray relays = doc["relays"].to<JsonArray>();
  for (int id = 1; id <= 3; id++) {
    JsonObject r = relays.add<JsonObject>();
    r["id"] = id;
    r["on"] = relayById(id)->getState();
  }

  JsonObject t = doc["temp"].to<JsonObject>();
  t["has"] = ds18b20.hasReading();
  t["value"] = ds18b20.getTemp();
  t["sim"] = ds18b20.isSimulated();
  t["min"] = ds18b20.getMin();
  t["max"] = ds18b20.getMax();
  if (forDashboard) {
    JsonArray hist = t["history"].to<JsonArray>();
    for (uint8_t i = 0; i < ds18b20.getHistoryCount(); i++) hist.add(serialized(String(ds18b20.getHistory(i), 1)));
  }

  JsonObject x = doc["xymd"].to<JsonObject>();
  x["has"] = xymd.hasReading();
  x["sim"] = xymd.isSimulated();
  x["id"] = xymd.getSlaveID();
  x["temp"] = xymd.getTemperature();
  x["hum"] = xymd.getHumidity();

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
  if (forDashboard) fillMqttStatus(doc["mqtt"].to<JsonObject>());
}

void handleStatus() {
  JsonDocument doc;
  fillStatus(doc, true);
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
}

// ---------- MQTT (HiveMQ public broker) ----------
// Topic ทั้งหมดอยู่ใต้ <base> เช่น hwclass/esp32-a1b2c3
//   <base>/telemetry          (publish, JSON รวมทุกค่า ทุก 5 วินาที)
//   <base>/status             (publish, retained: online/offline, มี Last Will)
//   <base>/relay/<n>/state    (publish, retained: ON/OFF เมื่อเปลี่ยน)
//   <base>/relay/<n>/set      (subscribe: on|off|toggle|1|0|true|false)
//   <base>/relay/all/set      (subscribe: คำสั่งเดียวกัน ใช้กับทุก relay)
#ifndef MQTT_HOST
#define MQTT_HOST "broker.hivemq.com"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
constexpr unsigned long MQTT_TELEMETRY_MS = 5000;
constexpr unsigned long MQTT_RETRY_MS = 10000;
constexpr uint8_t RELAY_COUNT = 3;

WiFiClient mqttNet;
PubSubClient mqtt(mqttNet);
String mqttBase;
String topicTelemetry, topicStatus;
String topicRelaySet[RELAY_COUNT], topicRelayState[RELAY_COUNT];
String topicRelayAll;
bool lastRelayState[RELAY_COUNT];
bool relayStatePublished = false;
unsigned long lastMqttTry = 0, lastTelemetry = 0, lastMqttPublish = 0;
String lastCommand;  // คำสั่งล่าสุดที่รับทาง MQTT (แสดงบน dashboard)

void setupMqttTopics() {
#ifdef MQTT_BASE_TOPIC
  mqttBase = MQTT_BASE_TOPIC;
#else
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  mqttBase = "hwclass/esp32-" + mac.substring(6);
#endif
  topicTelemetry = mqttBase + "/telemetry";
  topicStatus = mqttBase + "/status";
  topicRelayAll = mqttBase + "/relay/all/set";
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    topicRelaySet[i] = mqttBase + "/relay/" + (i + 1) + "/set";
    topicRelayState[i] = mqttBase + "/relay/" + (i + 1) + "/state";
  }
}

void fillMqttStatus(JsonObject m) {
  m["host"] = MQTT_HOST;
  m["port"] = MQTT_PORT;
  m["connected"] = mqtt.connected();
  m["base"] = mqttBase;
  m["last_pub"] = lastMqttPublish ? (millis() - lastMqttPublish) / 1000 : -1;
  m["last_cmd"] = lastCommand;
  JsonArray topics = m["topics"].to<JsonArray>();
  auto add = [&](const String& t, const char* dir, const char* desc) {
    JsonObject o = topics.add<JsonObject>();
    o["topic"] = t;
    o["dir"] = dir;
    o["desc"] = desc;
  };
  add(topicTelemetry, "pub", "JSON all data / 5s");
  add(topicStatus, "pub", "online / offline (retained)");
  for (uint8_t i = 0; i < RELAY_COUNT; i++) add(topicRelayState[i], "pub", "ON / OFF (retained)");
  for (uint8_t i = 0; i < RELAY_COUNT; i++) add(topicRelaySet[i], "sub", "on | off | toggle");
  add(topicRelayAll, "sub", "on | off | toggle (all relays)");
}

void publishMqtt(const String& topic, const String& payload, bool retain = false) {
  if (mqtt.publish(topic.c_str(), payload.c_str(), retain)) lastMqttPublish = millis();
}

void publishTelemetry() {
  JsonDocument doc;
  fillStatus(doc, false);
  String out;
  serializeJson(doc, out);
  publishMqtt(topicTelemetry, out);
}

void publishRelayState(uint8_t i) {
  publishMqtt(topicRelayState[i], relayById(i + 1)->getState() ? "ON" : "OFF", true);
}

// return: 1 = on, 0 = off, 2 = toggle, -1 = ไม่รู้จัก
int parseCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd == "on" || cmd == "1" || cmd == "true") return 1;
  if (cmd == "off" || cmd == "0" || cmd == "false") return 0;
  if (cmd == "toggle") return 2;
  return -1;
}

void applyCommand(DevRelay* r, int cmd) {
  if (cmd == 2) r->toggle();
  else r->setState(cmd == 1);
}

void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len && i < 16; i++) msg += (char)payload[i];
  int cmd = parseCommand(msg);
  if (cmd < 0) return;
  String t(topic);
  if (t == topicRelayAll) {
    for (uint8_t i = 1; i <= RELAY_COUNT; i++) applyCommand(relayById(i), cmd);
  } else {
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      if (t == topicRelaySet[i]) applyCommand(relayById(i + 1), cmd);
    }
  }
  lastCommand = t.substring(mqttBase.length() + 1) + " = " + msg;
}

void setupMqtt() {
  setupMqttTopics();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(1024);
  mqtt.setSocketTimeout(3);
  mqtt.setCallback(onMqttMessage);
}

void connectMqtt() {
  String clientId = "esp32-" + mqttBase.substring(mqttBase.lastIndexOf('/') + 1) + "-" + String((uint32_t)esp_random() & 0xffff, HEX);
  bool ok;
#if defined(MQTT_USER) && defined(MQTT_PASS)
  ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, topicStatus.c_str(), 1, true, "offline");
#else
  ok = mqtt.connect(clientId.c_str(), topicStatus.c_str(), 1, true, "offline");
#endif
  if (!ok) return;
  mqtt.subscribe(topicRelayAll.c_str());
  for (uint8_t i = 0; i < RELAY_COUNT; i++) mqtt.subscribe(topicRelaySet[i].c_str());
  publishMqtt(topicStatus, "online", true);
  relayStatePublished = false;  // ส่งสถานะ relay ทั้งหมดใหม่
  lastTelemetry = 0;
}

void handleMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!mqtt.connected()) {
    if (millis() - lastMqttTry >= MQTT_RETRY_MS || lastMqttTry == 0) {
      lastMqttTry = millis();
      connectMqtt();
    }
    return;
  }
  mqtt.loop();

  // relay เปลี่ยนจากทุกทาง (สวิตช์ / เว็บ / MQTT) -> ส่ง state + telemetry ทันที
  bool changed = false;
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    bool now = relayById(i + 1)->getState();
    if (!relayStatePublished || now != lastRelayState[i]) {
      lastRelayState[i] = now;
      publishRelayState(i);
      changed = true;
    }
  }
  relayStatePublished = true;

  if (changed || millis() - lastTelemetry >= MQTT_TELEMETRY_MS) {
    lastTelemetry = millis();
    publishTelemetry();
  }
}

// กดสวิตช์ 1 ครั้ง = สลับ ON <-> OFF ของ relay ที่คู่กัน
void onSw2Press() { relay2.toggle(); }
void onSw3Press() { relay3.toggle(); }

void setup() {
  xymd.begin(9600);  // Serial0 เท่านั้น

  relay1.begin();
  relay2.begin();
  relay3.begin();

  sw1.begin();
  sw2.begin();
  sw3.begin();
  ds18b20.begin();
  sw2.onPress(onSw2Press);
  sw3.onPress(onSw3Press);

  Wire.begin(21, 22);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  setupWifi();
  setupWebServer();
  setupMqtt();

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
  handleMqtt();
  sw1.update();
  handleSw1();
  sw2.update();
  sw3.update();
  ds18b20.update();
  xymd.update();

  if (millis() - lastWeather >= WEATHER_INTERVAL_MS) {
    lastWeather = millis();
    fetchWeather();
  }

  if (oledReady && !sw1Counting && millis() - lastOled >= OLED_INTERVAL_MS) {
    lastOled = millis();
    updateOled();
  }
}
