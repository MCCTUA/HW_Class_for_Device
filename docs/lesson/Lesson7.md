# Lesson 7: แสดงข้อมูลบน OLED SSD1306 I2C

## เป้าหมาย

ใช้งาน OLED SSD1306 128x64 ผ่าน I2C เพื่อแสดงข้อความและค่าจาก DS18B20

## Pin ที่ใช้

| OLED | ESP32 |
|---|---|
| SDA | GPIO21 |
| SCL | GPIO22 |
| VCC | 3V3 หรือ 5V ตามโมดูล |
| GND | GND |

## Library

- `Wire`
- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `OneWire`
- `DallasTemperature`

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define DS18B20_PIN 14

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

void setup() {
  Serial.begin(9600);
  ds18b20.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED not found");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 OLED Ready");
  display.display();
}

void loop() {
  ds18b20.requestTemperatures();
  float tempC = ds18b20.getTempCByIndex(0);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DS18B20");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 22);

  if (tempC == DEVICE_DISCONNECTED_C) {
    display.println("ERROR");
  } else {
    display.printf("%.2f C", tempC);
  }

  display.display();
  delay(1000);
}
```

## ผลลัพธ์ที่คาดหวัง

OLED แสดงหัวข้อ `DS18B20` และอุณหภูมิจาก sensor

## Troubleshooting

- ถ้าจอไม่ขึ้น ลองตรวจ address `0x3C` หรือ `0x3D`
- ตรวจสาย SDA/SCL ว่าถูก GPIO21/GPIO22
- ตรวจไฟเลี้ยงจอ OLED ว่าเหมาะกับโมดูล

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้าง main.cpp สำหรับ ESP32 DevKit V2 แสดงค่า DS18B20 GPIO14 บน OLED SSD1306 128x64 I2C address 0x3C โดยใช้ SDA GPIO21 SCL GPIO22 แสดง ERROR ถ้า DEVICE_DISCONNECTED_C และอ้างอิง blueprint_v2026.md
```

## คำถามทบทวน

- OLED I2C ใช้ GPIO ไหน?
- ถ้า OLED ไม่แสดงผล ควรเช็คอะไรเป็นอย่างแรก?
- ทำไมต้องเรียก `display.display()` หลังวาดข้อความ?

