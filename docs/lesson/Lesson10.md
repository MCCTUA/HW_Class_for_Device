# Lesson 10: รวมระบบ Mini Monitoring Board

## เป้าหมาย

รวมสิ่งที่เรียนทั้งหมดเป็น mini project: อ่าน switch, isolated input, relay, DS18B20, OLED, XY-MD03 และ PZEM โดยรักษา pin policy ของบอร์ด

## Feature ของการทดลอง

- SW1 GPIO34 กดเพื่อ toggle relay RL1 GPIO17
- ISOIN1 GPIO33 active แล้วเปิด relay
- DS18B20 GPIO14 แสดงอุณหภูมิทุก 1 วินาที
- OLED GPIO21/22 แสดงสถานะหลัก
- XY-MD03 อ่านผ่าน Serial/UART0 ทุก 3 วินาที
- PZEM-016 อ่านผ่าน Serial/UART0 ทุก 3 วินาที
- ใช้ `millis()` แทนการ `delay()` ยาว

## ตัวอย่างโครงโค้ด

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DevSwitch.h"
#include "DevIsoInput.h"
#include "DevRelay.h"
#include "DevXYMDSensor.h"
#include "DevPZEM.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define DS18B20_PIN 14

DevSwitch sw1(34, false);
DevIsoInput iso1(33, false);
DevRelay relay1(17, true);
DevXYMDSensor xymd(&Serial, 1);
DevPZEM pzem(&Serial, 0x01);

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long lastDsRead = 0;
unsigned long lastModbusRead = 0;

float boardTempC = 0.0;
bool dsOk = false;

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 Monitor");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 14);
  display.printf("Relay: %s\n", relay1.getState() ? "ON" : "OFF");
  display.printf("ISO1: %s\n", iso1.getStateText().c_str());

  if (dsOk) {
    display.printf("DS: %.2f C\n", boardTempC);
  } else {
    display.println("DS: ERROR");
  }

  if (pzem.isDataValid()) {
    display.printf("P: %.1f W\n", pzem.getPower());
  } else {
    display.println("PZEM: no data");
  }

  display.display();
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  sw1.begin();
  iso1.begin();
  relay1.begin();
  ds18b20.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED not found");
  }

  xymd.begin(9600);
  pzem.begin();
}

void loop() {
  sw1.update();
  iso1.update();

  if (sw1.wasPressed()) {
    relay1.toggle();
  }

  if (iso1.wasActivated()) {
    relay1.on();
  }

  if (iso1.wasDeactivated()) {
    relay1.off();
  }

  if (millis() - lastDsRead >= 1000) {
    lastDsRead = millis();
    ds18b20.requestTemperatures();
    boardTempC = ds18b20.getTempCByIndex(0);
    dsOk = boardTempC != DEVICE_DISCONNECTED_C;
  }

  if (millis() - lastModbusRead >= 3000) {
    lastModbusRead = millis();

    if (xymd.update()) {
      xymd.printInfo();
    }

    if (pzem.update() && pzem.isDataValid()) {
      Serial.println(pzem.toJSON());
    }
  }

  updateDisplay();
}
```

## ขั้นตอนทดลอง

1. Upload firmware ตอน switch อยู่โหมด RS232
2. หลัง upload เสร็จ สลับเป็น RS485
3. Reset ESP32 ถ้าจำเป็น
4. ดู OLED ว่าข้อมูลแสดงหรือไม่
5. ทดสอบกด SW1 เพื่อ toggle relay
6. ทดสอบ ISOIN1 ว่า relay ตอบสนองหรือไม่
7. ดู Serial/ช่อง debug ว่าค่า XY-MD03 และ PZEM ออกมาถูกต้อง

## ปรับปรุงต่อ

- เพิ่ม MQTT publish ค่า `pzem.toJSON()`
- เพิ่ม alarm เมื่อ PZEM power เกินค่าที่กำหนด
- เพิ่ม averaging/filter สำหรับ DS18B20
- เพิ่มหน้า OLED หลายหน้า เปลี่ยนด้วย SW2

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้าง mini project main.cpp สำหรับ ESP32 DevKit V2 จาก blueprint_v2026.md รวม DevSwitch GPIO34, DevIsoInput GPIO33, DevRelay GPIO17 active low, DS18B20 GPIO14, OLED SSD1306 I2C GPIO21/22, DevXYMDSensor ผ่าน Serial/UART0 และ DevPZEM ผ่าน Serial/UART0 ใช้ millis แทน delay ยาว ห้ามใช้ Serial2 และต้องตรวจ isDataValid, DEVICE_DISCONNECTED_C, wasPressed, wasActivated ให้ถูกต้อง
```

## คำถามทบทวน

- ทำไมระบบรวมควรใช้ `millis()` มากกว่า `delay()`?
- ถ้า PZEM เข้า simulation mode ควรแจ้งผู้ใช้ตรงไหน?
- ถ้าจะเพิ่ม MQTT ควรส่งข้อมูลส่วนใดก่อน?

