# Lesson 4: อ่าน Switch แบบ Debounce และ Edge Detection

## เป้าหมาย

อ่านปุ่มหรือ toggle switch ด้วย `DevSwitch` และใช้ `wasPressed()` / `wasReleased()` เพื่อทำงานครั้งเดียวตอนเกิด edge

## Pin ที่ใช้

| Switch | GPIO | Active Level |
|---|---:|---|
| SW1 | 34 | Active Low |
| SW2 | 35 | Active Low |
| SW3 | 32 | Active Low |

## แนวคิดสำคัญ

ปุ่มจริงมักมี bounce ทำให้ค่ากระพริบหลายครั้งในช่วงสั้น ๆ `DevSwitch` ช่วย debounce และให้ edge event ที่เป็น `true` แค่หนึ่งรอบหลังเรียก `update()`

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include "DevSwitch.h"
#include "DevRelay.h"

DevSwitch sw1(34, false);
DevRelay relay1(17, true);

void setup() {
  Serial.begin(9600);
  sw1.begin();
  relay1.begin();
  Serial.println("Switch debounce test");
}

void loop() {
  sw1.update();

  if (sw1.wasPressed()) {
    Serial.println("SW1 pressed: toggle relay");
    relay1.toggle();
  }

  if (sw1.wasReleased()) {
    Serial.println("SW1 released");
  }
}
```

## Callback แบบง่าย

```cpp
void onSw1Click() {
  Serial.println("SW1 click callback");
}

void setup() {
  Serial.begin(9600);
  sw1.begin();
  sw1.onClick(onSw1Click);
}
```

## ผลลัพธ์ที่คาดหวัง

- กด SW1 หนึ่งครั้ง relay toggle หนึ่งครั้ง
- Serial แสดง pressed/released ตามจังหวะกดจริง
- ไม่เกิดการ toggle รัวจาก bounce

## ข้อควรระวัง

- ต้องเรียก `sw1.update()` ก่อนตรวจ `wasPressed()`
- `wasPressed()` เป็นจริงแค่ loop รอบที่เกิด edge เท่านั้น
- GPIO34/35 เป็น input-only และควรมี pull-up/pull-down ภายนอกตามวงจร

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้างโค้ด PlatformIO สำหรับ ESP32 DevKit V2 อ่าน SW1 ที่ GPIO34 ด้วย DevSwitch active low แล้วเมื่อ wasPressed ให้ toggle DevRelay ที่ GPIO17 active low พร้อมพิมพ์ Serial 9600 ห้ามใช้ GPIO34 เป็น output และอ้างอิง blueprint_v2026.md
```

## คำถามทบทวน

- `isPressed()` ต่างจาก `wasPressed()` อย่างไร?
- ทำไมต้อง debounce switch?
- ถ้าลืมเรียก `update()` จะเกิดอะไรขึ้น?

