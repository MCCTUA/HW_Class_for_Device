# Lesson 8: อ่าน XY-MD03 ผ่าน RS485 UART0

## เป้าหมาย

อ่านอุณหภูมิและความชื้นจาก XY-MD03 ด้วย `DevXYMDSensor` ผ่าน RS485 บน `Serial` / UART0

## Hardware Policy

โปรเจกต์นี้ใช้ UART0:

| Signal | ESP32 |
|---|---|
| UART0 TX | GPIO1 |
| UART0 RX | GPIO3 |

มี switch ภายนอกสำหรับสลับ:

- RS232/USB serial mode สำหรับ upload/debug
- RS485 mode สำหรับอ่าน sensor

## Modbus Setting

| Parameter | Value |
|---|---|
| Baud rate | 9600 |
| Serial config | 8N1 |
| Function | 0x04 Read Input Registers |
| Temperature register | 0x0001 |
| Humidity register | 0x0002 |
| Scale | raw / 10.0 |

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include "DevXYMDSensor.h"

DevXYMDSensor sensor(&Serial, 1);

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("Switch to RS485 mode, then reset if needed");
  sensor.begin(9600);
}

void loop() {
  if (sensor.update()) {
    Serial.printf("Temp: %.1f C, Humidity: %.1f %%\n",
                  sensor.getTemperature(),
                  sensor.getHumidity());
  } else {
    Serial.println("XY-MD03 read failed");
  }

  delay(3000);
}
```

## ขั้นตอนทดลอง

1. ตั้ง switch เป็น RS232
2. Upload firmware
3. ตั้ง switch เป็น RS485
4. Reset ESP32 ถ้าจำเป็น
5. เปิด Serial Monitor หรือดูผลตามช่องทางที่วงจรอนุญาต

## ข้อควรระวัง

- อย่าย้ายไป `Serial2` เพราะ hardware นี้มี switch ภายนอกและ blueprint กำหนดให้ใช้ UART0
- ตรวจ Slave ID ให้ตรงกับ sensor จริง
- อ่านทุก 2-3 วินาทีเพื่อความเสถียร
- ถ้าอ่านไม่ได้ ให้เช็ค A/B, GND, power, baud rate และ switch mode

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้าง main.cpp อ่าน XY-MD03 ผ่าน DevXYMDSensor โดยใช้ Serial/UART0 ไม่ใช้ Serial2 ตั้ง baud 9600 slave id 1 อ่านทุก 3 วินาที แสดงอุณหภูมิและความชื้นทาง Serial อ้างอิง blueprint_v2026.md และเตือน workflow RS232 upload / RS485 read
```

## คำถามทบทวน

- ทำไมบทนี้ยังใช้ `Serial` แทน `Serial2`?
- Register ของ humidity คืออะไร?
- ถ้า upload ไม่ผ่านหลังต่อ RS485 ควรทำอะไร?

