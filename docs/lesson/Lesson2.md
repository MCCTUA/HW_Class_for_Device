# Lesson 2: เข้าใจ Pin Map และกติกาการใช้ GPIO

## เป้าหมาย

เข้าใจขา GPIO สำคัญของ ESP32 DevKit V2 ก่อนเริ่มต่ออุปกรณ์จริง เพื่อลดปัญหา boot ไม่ขึ้น, upload ไม่ผ่าน, หรือใช้ขาผิดประเภท

## Pin สำคัญของบอร์ดนี้

| GPIO | บทบาทในชุดทดลอง |
|---:|---|
| 1 | UART0 TX ใช้ upload/debug และ RS485 TX |
| 3 | UART0 RX ใช้ upload/debug และ RS485 RX |
| 4 | Relay 3 active low |
| 14 | DS18B20 1-Wire data |
| 16 | Relay 2 active low |
| 17 | Relay 1 active low |
| 21 | OLED I2C SDA |
| 22 | OLED I2C SCL |
| 27 | Isolated input 2 |
| 32 | Switch 3 |
| 33 | Isolated input 1 |
| 34 | Switch 1 input-only |
| 35 | Switch 2 input-only |

## ขาที่ควรระวัง

| GPIO | เหตุผล |
|---:|---|
| 0 | เกี่ยวข้องกับ boot mode |
| 1, 3 | ใช้ UART0 ร่วมกับ upload/debug และ RS485 |
| 5, 12, 15 | strapping pins มีผลตอน boot |
| 6-11 | ต่อกับ SPI flash ภายใน ห้ามใช้ |
| 34, 35, 36, 39 | input-only และควรมี resistor ภายนอก |

## กติกาของโปรเจกต์นี้

- ใช้ `Serial` / UART0 สำหรับ RS485 เพราะมี switch ภายนอกสลับ RS232/RS485
- GPIO14 ถูกจองให้ DS18B20 ไม่ใช้เป็น AUX output
- OLED ใช้ I2C ที่ GPIO21/GPIO22
- Relay เป็น active low: `LOW = ON`, `HIGH = OFF`
- Switch และ isolated input โดยปกติเป็น active low: `LOW = pressed/active`

## การทดลองอย่างง่าย

ใช้โค้ดนี้ตรวจว่าบอร์ดทำงาน และเตือนตัวเองผ่าน Serial:

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Pin policy loaded:");
  Serial.println("GPIO14 = DS18B20");
  Serial.println("GPIO21/22 = OLED I2C");
  Serial.println("GPIO1/3 = UART0 RS485 + Upload");
}

void loop() {
}
```

## Checklist ก่อนต่อวงจร

- DS18B20 DATA ต่อ GPIO14 พร้อม pull-up 4.7k ไป 3V3
- OLED ต่อ SDA GPIO21 และ SCL GPIO22
- RS485 ใช้ GPIO1/GPIO3 ผ่าน switch ภายนอก
- Switch บน GPIO34/35 ต้องไม่ตั้งเป็น output
- Relay ใช้ GPIO17/16/4 แบบ active low

## Prompt สำหรับสั่ง AI

```text
อ่าน blueprint_v2026.md แล้วช่วยสรุป pin map ที่ต้องใช้สำหรับ ESP32 DevKit V2 โปรเจกต์นี้ โดยเน้น GPIO14 เป็น DS18B20, GPIO21/22 เป็น OLED, GPIO1/3 เป็น UART0 RS485, GPIO17/16/4 เป็น relay และ GPIO34/35/32 เป็น switch พร้อมเตือนขาที่ห้ามใช้
```

## คำถามทบทวน

- GPIO14 ถูกใช้ทำอะไรในบอร์ดนี้?
- GPIO34/35 ใช้เป็น output ได้ไหม?
- Relay active low หมายความว่าอะไร?

