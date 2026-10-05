# Lesson 9: อ่าน PZEM-016 ผ่าน RS485 และส่งออก JSON

## เป้าหมาย

อ่านค่าไฟฟ้าจาก PZEM-016 ด้วย `DevPZEM` และส่งข้อมูลออกเป็น JSON string เพื่อใช้ต่อกับ MQTT, logging หรือ dashboard

## ค่าที่อ่านได้

| Value | Getter | Unit |
|---|---|---|
| Voltage | `getVoltage()` | V |
| Current | `getCurrent()` | A |
| Power | `getPower()` | W |
| Energy | `getEnergy()` | kWh |
| Frequency | `getFrequency()` | Hz |
| Power Factor | `getPowerFactor()` | 0.00-1.00 |
| Alarm | `getAlarmStatus()` | raw |

## ตัวอย่างโค้ด

```cpp
#include <Arduino.h>
#include "DevPZEM.h"

DevPZEM pzem(&Serial, 0x01);

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("PZEM-016 test on Serial/UART0");
  Serial.println("Upload in RS232 mode, read in RS485 mode");

  pzem.begin();
}

void loop() {
  if (pzem.update()) {
    if (pzem.isDataValid()) {
      pzem.printData();
      Serial.println(pzem.toJSON());
    } else {
      Serial.println("PZEM data not valid yet");
    }
  } else {
    Serial.println("PZEM update failed");
  }

  delay(3000);
}
```

## ทดลอง reset energy

เพิ่มคำสั่งผ่าน Serial:

```cpp
if (Serial.available()) {
  char cmd = Serial.read();
  if (cmd == 'r' || cmd == 'R') {
    pzem.resetEnergy();
  }
}
```

## Simulation Fallback

`DevPZEM.h` ปัจจุบันมี simulation fallback ถ้าไม่พบ sensor จริงหลัง retry ดังนั้นควรตรวจ:

```cpp
if (pzem.isSimulationMode()) {
  Serial.println("Warning: PZEM is simulation mode");
}
```

## ข้อควรระวัง

- PZEM-016 เกี่ยวข้องกับไฟ AC 80-260V ต้องต่ออย่างปลอดภัย
- ใช้ UART0 ผ่าน switch ภายนอกเหมือน XY-MD03
- อ่านทุก 2-3 วินาที ไม่ควรยิง Modbus ถี่เกินไป
- ตรวจ `isDataValid()` ก่อนเอาค่าไปควบคุมระบบจริง

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้าง main.cpp สำหรับ ESP32 DevKit V2 อ่าน PZEM-016 ด้วย DevPZEM ผ่าน Serial/UART0 slave address 0x01 อ่านทุก 3 วินาที แสดง printData และ toJSON ตรวจ isDataValid และ isSimulationMode ห้ามย้ายไป Serial2 อ้างอิง blueprint_v2026.md
```

## คำถามทบทวน

- `getEnergy()` คืนค่าเป็น Wh หรือ kWh?
- ทำไมต้องตรวจ `isDataValid()`?
- `isSimulationMode()` สำคัญอย่างไร?

