#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "DevRelay.h"
#include "DevSwitch.h"

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

void updateOled() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("RELAY STATUS");
  display.drawLine(0, 11, SCREEN_WIDTH - 1, 11, SSD1306_WHITE);
  drawRelayRow(0, "Relay1", relay1);
  drawRelayRow(1, "Relay2", relay2);
  drawRelayRow(2, "Relay3", relay3);
  display.display();
}

// กดสวิตช์ 1 ครั้ง = สลับ ON <-> OFF ของ relay ที่คู่กัน
void onSw1Press() { relay1.toggle(); }
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
  sw1.onPress(onSw1Press);
  sw2.onPress(onSw2Press);
  sw3.onPress(onSw3Press);

  Wire.begin(21, 22);
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!oledReady) Serial.println("OLED init failed");
}

void loop() {
  sw1.update();
  sw2.update();
  sw3.update();

  if (oledReady && millis() - lastOled >= OLED_INTERVAL_MS) {
    lastOled = millis();
    updateOled();
  }
}
