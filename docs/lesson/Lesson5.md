# Lesson 5: อ่าน Isolated Input ด้วย DevIsoInput

## เป้าหมาย

ใช้ `DevIsoInput` อ่าน isolated input และนับจำนวนครั้งที่ input กลายเป็น active

## Pin ที่ใช้

| Input | GPIO | Active Level |
|---|---:|---|
| ISOIN1 | 33 | Active Low |
| ISOIN2 | 27 | Active Low |

## แนวคิดสำคัญ

Isolated input ใช้อ่านสัญญาณภายนอกที่ผ่านวงจร isolation มาแล้ว เช่น contact, sensor output หรือสัญญาณสถานะจากอุปกรณ์อื่น คลาสนี้มี debounce, callback, edge detection และ counter

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include "DevIsoInput.h"
#include "DevRelay.h"

DevIsoInput iso1(33, false);
DevRelay relay1(17, true);

void setup() {
  Serial.begin(9600);
  iso1.begin();
  relay1.begin();
  Serial.println("Isolated input test");
}

void loop() {
  iso1.update();

  if (iso1.wasActivated()) {
    Serial.print("ISO1 active count = ");
    Serial.println(iso1.getActivationCount());
    relay1.on();
  }

  if (iso1.wasDeactivated()) {
    Serial.println("ISO1 inactive");
    relay1.off();
  }
}
```

## ทดลอง callback

```cpp
void onIsoActive() {
  Serial.println("Callback: ISO active");
}

void setup() {
  Serial.begin(9600);
  iso1.begin();
  iso1.onActive(onIsoActive);
}
```

## ผลลัพธ์ที่คาดหวัง

- เมื่อ ISOIN1 active relay เปิด
- เมื่อ ISOIN1 inactive relay ปิด
- activation count เพิ่มครั้งละ 1 เฉพาะตอนเปลี่ยนจาก inactive เป็น active

## ข้อควรระวัง

- อย่าต่อแรงดันภายนอกเข้าขา ESP32 โดยตรงถ้าไม่ได้ผ่านวงจร isolation/level shifting
- ต้องต่อ GND หรือ reference ตามแบบวงจรของบอร์ด
- `wasActivated()` เป็น event หนึ่งรอบหลัง `update()` เท่านั้น

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้าง src/main.cpp สำหรับอ่าน isolated input ISOIN1 ที่ GPIO33 ด้วย DevIsoInput active low ถ้า wasActivated ให้เปิด relay GPIO17 และพิมพ์ activation count ถ้า wasDeactivated ให้ปิด relay อ้างอิง blueprint_v2026.md
```

## คำถามทบทวน

- `getActivationCount()` นับจังหวะไหน?
- `wasActivated()` ต่างจาก `isActive()` อย่างไร?
- ทำไม isolated input จึงควรมี debounce?

