# Lesson 1: สร้างโปรเจกต์ PlatformIO สำหรับ ESP32 DevKit V2

## เป้าหมาย

สร้างโปรเจกต์พื้นฐานสำหรับ ESP32 DevKit V2 ใน VS Code + PlatformIO และทดสอบว่าอัปโหลดโค้ดกับเปิด Serial Monitor ได้ถูกต้อง

## อุปกรณ์

- ESP32 DevKit V2 / DOIT ESP32 DEVKIT V1 compatible
- สาย USB สำหรับอัปโหลด
- VS Code + PlatformIO extension

## แนวคิดสำคัญ

บอร์ดนี้ใช้ `esp32doit-devkit-v1` ใน PlatformIO และใช้ Arduino Framework เป็นฐาน โค้ดทั้งหมดในชุดนี้ตั้งค่า Serial Monitor ที่ `9600` baud เพื่อให้สอดคล้องกับ RS485/Modbus ที่ใช้ในบทถัดไป

## platformio.ini ที่ต้องมี

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 9600

lib_deps =
  4-20ma/ModbusMaster@^2.0.1
  adafruit/Adafruit GFX Library
  adafruit/Adafruit SSD1306
  paulstoffregen/OneWire
  milesburton/DallasTemperature
```

## main.cpp ขั้นแรก

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("ESP32 DevKit V2 ready");
}

void loop() {
  Serial.println("Hello from PlatformIO");
  delay(1000);
}
```

## ขั้นตอนทดลอง

1. สร้างโปรเจกต์ PlatformIO ใหม่ เลือกบอร์ด `DOIT ESP32 DEVKIT V1`
2. ใส่ค่า `platformio.ini` ตามด้านบน
3. สร้างหรือแก้ `src/main.cpp`
4. ต่อบอร์ดกับคอมพิวเตอร์
5. กด Build
6. กด Upload
7. เปิด Serial Monitor ที่ 9600 baud

## ผลลัพธ์ที่คาดหวัง

Serial Monitor แสดง:

```text
ESP32 DevKit V2 ready
Hello from PlatformIO
Hello from PlatformIO
```

## ข้อควรระวัง

- ถ้าอัปโหลดไม่ได้ ให้ลองกดปุ่ม BOOT ตอนขึ้น `Connecting...`
- ถ้ามี switch RS232/RS485 บนบอร์ด ให้สลับไปโหมด RS232/USB serial ก่อนอัปโหลด
- อย่าใช้ GPIO 6-11 เพราะเป็นขา flash ภายใน ESP32

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้างโปรเจกต์ PlatformIO สำหรับ ESP32 DevKit V2 โดยใช้ board esp32doit-devkit-v1, Arduino framework, monitor_speed 9600 และสร้าง src/main.cpp สำหรับทดสอบ Serial Monitor พิมพ์ข้อความทุก 1 วินาที อ้างอิง blueprint_v2026.md และอย่าย้าย UART0 ไป Serial2
```

## คำถามทบทวน

- `monitor_speed = 9600` มีผลกับอะไร?
- ทำไมต้องสลับ switch เป็น RS232 ก่อน upload?
- ทำไม GPIO 6-11 ไม่ควรถูกใช้งาน?

