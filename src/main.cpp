#include <Arduino.h>

#include "DevRelay.h"
#include "DevSwitch.h"
#include "DevIsoInput.h"
#include "DevPZEM.h"
#include "DevXYMDSensor.h"

// Pin assignment (ปรับตามบอร์ดจริง)
constexpr uint8_t PIN_RELAY = 26;
constexpr uint8_t PIN_SWITCH = 27;
constexpr uint8_t PIN_ISO_INPUT = 34;

DevRelayWithTimer relay(PIN_RELAY);
DevSwitch button(PIN_SWITCH);
DevIsoInput isoInput(PIN_ISO_INPUT);
DevPZEM pzem(&Serial);            // UART0 (auto direction RS485)
DevXYMDSensor xymd(&Serial2, 1);  // UART2 แยกจาก PZEM

constexpr unsigned long RELAY_INTERVAL_MS = 5000;

unsigned long lastPrint = 0;
unsigned long lastRelayToggle = 0;

void onButtonClick() {
  relay.toggle();
}

void onIsoActive() {
  relay.onWithTimer(5000);  // เปิด 5 วินาทีเมื่อ input active
}

void setup() {
  Serial.begin(9600);

  relay.begin();
  button.begin();
  isoInput.begin();
  pzem.begin();
  xymd.begin(9600);

  button.onClick(onButtonClick);
  isoInput.onActive(onIsoActive);
}

void loop() {
  button.update();
  isoInput.update();
  relay.checkTimer();

  // สลับ relay on/off ทุก 5 วินาที (ใช้ millis ไม่ใช้ delay)
  if (millis() - lastRelayToggle >= RELAY_INTERVAL_MS) {
    lastRelayToggle = millis();
    relay.toggle();
  }

  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    if (pzem.update()) pzem.printData();
    if (xymd.update()) xymd.printInfo();
  }
}
