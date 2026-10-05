# Lesson 3: ทดลอง Relay Active Low ด้วย DevRelay

## เป้าหมาย

ควบคุม relay บนบอร์ดด้วยคลาส `DevRelay` และเข้าใจ logic แบบ active low

## อุปกรณ์

- ESP32 DevKit V2
- Relay ที่ต่อกับ GPIO17, GPIO16 หรือ GPIO4
- ไฟเลี้ยงและโหลดตามความปลอดภัยของ relay

## Pin ที่ใช้

| Relay | GPIO | Active Level |
|---|---:|---|
| RL1 | 17 | Active Low |
| RL2 | 16 | Active Low |
| RL3 | 4 | Active Low |

## แนวคิดสำคัญ

`DevRelay` ซ่อนรายละเอียด active low ให้เราเรียกใช้งานแบบ logical ได้:

- `relay.on()` คือเปิด relay
- `relay.off()` คือปิด relay
- ถ้า active low ภายในจะเขียน `LOW` เมื่อต้องการเปิด

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include "DevRelay.h"

DevRelay relay1(17, true);

void setup() {
  Serial.begin(9600);
  relay1.begin();
  Serial.println("Relay test start");
}

void loop() {
  Serial.println("Relay ON");
  relay1.on();
  delay(1000);

  Serial.println("Relay OFF");
  relay1.off();
  delay(1000);
}
```

## ทดลอง Relay Timer

```cpp
#include <Arduino.h>
#include "DevRelay.h"

DevRelayWithTimer relay1(17, true);

void setup() {
  Serial.begin(9600);
  relay1.begin();
  relay1.onWithTimer(3000);
  Serial.println("Relay ON for 3 seconds");
}

void loop() {
  if (relay1.checkTimer()) {
    Serial.println("Timer expired, relay OFF");
  }
}
```

## ผลลัพธ์ที่คาดหวัง

- relay เปิด/ปิดทุก 1 วินาทีในตัวอย่างแรก
- relay เปิด 3 วินาทีแล้วปิดเองในตัวอย่าง timer

## ข้อควรระวัง

- Relay active low หมายถึงถ้าเขียน GPIO เอง `LOW = ON`
- อย่าต่อโหลด AC โดยไม่มีความรู้ด้านไฟฟ้า
- เรียก `begin()` ก่อนใช้งาน relay เสมอ เพื่อให้เริ่มที่สถานะปิด

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้าง src/main.cpp สำหรับทดสอบ DevRelay บน ESP32 DevKit V2 โดยใช้ relay RL1 ที่ GPIO17 แบบ active low ให้เปิด 1 วินาที ปิด 1 วินาที และพิมพ์สถานะทาง Serial 9600 อ้างอิง blueprint_v2026.md
```

## คำถามทบทวน

- ทำไม `begin()` ของ relay จึงเรียก `off()` ตอนเริ่มต้น?
- ถ้า relay เป็น active high ต้องสร้าง object อย่างไร?
- `DevRelayWithTimer` ต่างจากใช้ `delay()` อย่างไร?

