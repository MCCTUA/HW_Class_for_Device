# Lesson 6: อ่าน DS18B20 ที่ GPIO14

## เป้าหมาย

อ่านอุณหภูมิจาก DS18B20 ผ่าน 1-Wire บน GPIO14 ด้วย `OneWire` และ `DallasTemperature`

## อุปกรณ์

- DS18B20
- ตัวต้านทาน pull-up 4.7k ระหว่าง DATA และ 3V3
- ESP32 DevKit V2

## Pin ที่ใช้

| DS18B20 | ESP32 |
|---|---|
| DATA | GPIO14 |
| VDD | 3V3 |
| GND | GND |

## แนวคิดสำคัญ

GPIO14 ถูกจองเป็นขา DS18B20 ใน blueprint นี้ ห้ามใช้เป็น AUX output เว้นแต่แก้ hardware จริง

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_PIN 14

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

void setup() {
  Serial.begin(9600);
  ds18b20.begin();
  Serial.println("DS18B20 test on GPIO14");
}

void loop() {
  ds18b20.requestTemperatures();
  float tempC = ds18b20.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("DS18B20 disconnected");
  } else {
    Serial.printf("Temperature: %.2f C\n", tempC);
  }

  delay(1000);
}
```

## ผลลัพธ์ที่คาดหวัง

Serial Monitor แสดงอุณหภูมิทุก 1 วินาที เช่น:

```text
Temperature: 28.25 C
```

## ข้อควรระวัง

- ต้องมี pull-up 4.7k จาก DATA ไป 3V3
- ตรวจ `DEVICE_DISCONNECTED_C` ก่อนใช้ค่า
- ถ้าใช้หลายตัวบน bus เดียวกัน ควรอ่านด้วย device address

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้างโค้ด ESP32 PlatformIO อ่าน DS18B20 ที่ GPIO14 โดยใช้ OneWire และ DallasTemperature แสดงค่า Celsius ทาง Serial 9600 ทุก 1 วินาที ตรวจ DEVICE_DISCONNECTED_C และอ้างอิง blueprint_v2026.md ว่า GPIO14 ถูกจองสำหรับ DS18B20
```

## คำถามทบทวน

- ทำไม DS18B20 ต้องมี pull-up resistor?
- `DEVICE_DISCONNECTED_C` ใช้ตรวจอะไร?
- ทำไมไม่ควรใช้ GPIO14 เป็น relay ในโปรเจกต์นี้?

