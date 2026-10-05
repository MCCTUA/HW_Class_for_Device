# DevXYMDSensor Class Documentation

## 📖 ภาพรวม

`DevXYMDSensor` เป็นคลาสสำหรับจัดการเซนเซอร์อุณหภูมิและความชื้น **XY-MD03** ผ่านโปรโตคอล **Modbus RTU** บน ESP32 รองรับการใช้งานกับ Auto Direction RS485 (ไม่ต้องควบคุม DE/RE pin)

## 🔧 Hardware Requirements

- **ESP32 DEVKIT V2.0** หรือบอร์ดที่รองรับ
- **XY-MD03 Temperature & Humidity Sensor**
- **RS485 Module** (Auto Direction)
- **Switch สำหรับสลับโหมด RS232/RS485**

## 📋 Modbus Specifications

### Default Settings
| Parameter | Value |
|-----------|-------|
| Slave ID | 1 (0x01) |
| Baud Rate | 9600 |
| Data bits | 8 |
| Stop bit | 1 |
| Parity | None |
| Function Code | 04 (Read Input Register) |

### Register Map
| Register Address | Description | Unit | Formula |
|-----------------|-------------|------|---------|
| 0x0001 | Temperature | °C | value / 10 |
| 0x0002 | Humidity | % | value / 10 |

## 📦 Dependencies

```ini
lib_deps =
  4-20ma/ModbusMaster@^2.0.1
```

## 🚀 การใช้งาน

### 1. Include Header File

```cpp
#include "DevXYMDSensor.h"
```

### 2. สร้าง Object

```cpp
// สร้าง object โดยใช้ Serial หลัก (USB Serial)
DevXYMDSensor tempSensor(&Serial, 1);

// หรือใช้ Serial2 (ถ้ามี)
// DevXYMDSensor tempSensor(&Serial2, 1);
```

**Parameters:**
- `hwSerial`: ตัวชี้ไปที่ HardwareSerial object (เช่น `&Serial`, `&Serial2`)
- `slaveId`: Modbus Slave ID (default: 1)

### 3. เริ่มต้นการทำงาน

```cpp
void setup() {
  Serial.begin(9600);
  
  // เริ่มต้น sensor (Auto Direction RS485)
  tempSensor.begin(9600);
}
```

**Parameters:**
- `baudRate`: ความเร็วการสื่อสาร (default: 9600)

### 4. อ่านค่าจากเซนเซอร์

```cpp
void loop() {
  // อ่านค่าอุณหภูมิและความชื้น
  if (tempSensor.update()) {
    // อ่านค่าสำเร็จ
    Serial.printf("Temperature: %.1f°C\n", tempSensor.getTemperature());
    Serial.printf("Humidity: %.1f%%\n", tempSensor.getHumidity());
  } else {
    // อ่านค่าไม่สำเร็จ
    Serial.println("Failed to read sensor!");
  }
  
  delay(2000);
}
```

## 📚 Public Methods

### Constructor

#### `DevXYMDSensor(HardwareSerial* hwSerial, uint8_t slaveId = 1)`

สร้าง object ของเซนเซอร์อุณหภูมิและความชื้น

**Parameters:**
- `hwSerial`: ตัวชี้ไปที่ HardwareSerial object
- `slaveId`: Modbus Slave ID (default: 1)

**ตัวอย่าง:**
```cpp
DevXYMDSensor sensor(&Serial, 1);
```

---

### Initialization

#### `void begin(unsigned long baudRate = 9600)`

เริ่มต้นการทำงานของเซนเซอร์

**Parameters:**
- `baudRate`: ความเร็ว Baud Rate (default: 9600)

**ตัวอย่าง:**
```cpp
sensor.begin(9600);
```

---

### Data Reading

#### `bool update()`

อ่านค่าอุณหภูมิและความชื้นจากเซนเซอร์

**Returns:**
- `true`: อ่านค่าสำเร็จ
- `false`: อ่านค่าไม่สำเร็จ

**ตัวอย่าง:**
```cpp
if (sensor.update()) {
  // ดึงค่าที่อ่านได้
  float temp = sensor.getTemperature();
  float hum = sensor.getHumidity();
}
```

---

### Getters

#### `float getTemperature() const`

ดึงค่าอุณหภูมิล่าสุดที่อ่านได้

**Returns:** ค่าอุณหภูมิในหน่วย °C

**ตัวอย่าง:**
```cpp
float temp = sensor.getTemperature();
Serial.printf("Temp: %.1f°C\n", temp);
```

---

#### `float getHumidity() const`

ดึงค่าความชื้นล่าสุดที่อ่านได้

**Returns:** ค่าความชื้นในหน่วย %

**ตัวอย่าง:**
```cpp
float hum = sensor.getHumidity();
Serial.printf("Humidity: %.1f%%\n", hum);
```

---

#### `bool isLastReadSuccess() const`

ตรวจสอบสถานะการอ่านค่าครั้งล่าสุด

**Returns:**
- `true`: อ่านค่าสำเร็จ
- `false`: อ่านค่าไม่สำเร็จ

**ตัวอย่าง:**
```cpp
if (sensor.isLastReadSuccess()) {
  Serial.println("Last read was successful!");
}
```

---

#### `unsigned long getLastReadTime() const`

ดึงเวลาที่อ่านค่าครั้งล่าสุด

**Returns:** เวลาใน milliseconds

**ตัวอย่าง:**
```cpp
unsigned long lastRead = sensor.getLastReadTime();
unsigned long timeSinceRead = millis() - lastRead;
Serial.printf("Time since last read: %lu ms\n", timeSinceRead);
```

---

### Utility Methods

#### `void printInfo() const`

แสดงข้อมูลทั้งหมดของเซนเซอร์ทาง Serial Monitor

**ตัวอย่าง:**
```cpp
sensor.printInfo();
```

**Output:**
```
=== XY-MD03 Temperature & Humidity Sensor ===
Temperature: 25.3 °C
Humidity: 65.2 %
Last Read: Success (1234 ms ago)
=============================================
```

---

### Configuration

#### `void setSlaveID(uint8_t newSlaveId)`

เปลี่ยน Modbus Slave ID

**Parameters:**
- `newSlaveId`: Slave ID ใหม่

**ตัวอย่าง:**
```cpp
sensor.setSlaveID(2);  // เปลี่ยนเป็น Slave ID = 2
```

---

#### `uint8_t getSlaveID() const`

ดึง Modbus Slave ID ปัจจุบัน

**Returns:** Slave ID

**ตัวอย่าง:**
```cpp
uint8_t id = sensor.getSlaveID();
Serial.printf("Current Slave ID: %d\n", id);
```

---

## 💡 ตัวอย่างโปรแกรมสมบูรณ์

### ตัวอย่างที่ 1: Basic Usage

```cpp
#include <Arduino.h>
#include "DevXYMDSensor.h"

DevXYMDSensor sensor(&Serial, 1);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("=== XY-MD03 Sensor Test ===");
  sensor.begin(9600);
  
  Serial.println("Note: Switch to RS485 mode!");
  delay(2000);
}

void loop() {
  Serial.println("\n--- Reading sensor ---");
  
  if (sensor.update()) {
    // อ่านค่าสำเร็จ
    float temp = sensor.getTemperature();
    float hum = sensor.getHumidity();
    
    Serial.printf("Temperature: %.1f°C\n", temp);
    Serial.printf("Humidity: %.1f%%\n", hum);
    
    // ตรวจสอบค่าที่ผิดปกติ
    if (temp < -40 || temp > 80) {
      Serial.println("Warning: Temperature out of range!");
    }
    
    if (hum < 0 || hum > 100) {
      Serial.println("Warning: Humidity out of range!");
    }
  } else {
    // อ่านค่าไม่สำเร็จ
    Serial.println("Failed to read sensor!");
    Serial.println("- Check RS485 connection");
    Serial.println("- Check switch position (RS485 mode)");
    Serial.println("- Check Slave ID setting");
  }
  
  delay(3000);
}
```

---

### ตัวอย่างที่ 2: With OLED Display

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DevXYMDSensor.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DevXYMDSensor sensor(&Serial, 1);

void setup() {
  Serial.begin(9600);
  
  // เริ่มต้น OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED not found!");
    while(1);
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("XY-MD03 Sensor");
  display.println("Initializing...");
  display.display();
  
  // เริ่มต้น Sensor
  sensor.begin(9600);
  delay(2000);
}

void loop() {
  if (sensor.update()) {
    float temp = sensor.getTemperature();
    float hum = sensor.getHumidity();
    
    // แสดงผลบน OLED
    display.clearDisplay();
    
    // หัวข้อ
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=== XY-MD03 ===");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    
    // อุณหภูมิ
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.printf("%.1fC", temp);
    
    // ความชื้น
    display.setCursor(0, 32);
    display.printf("%.1f%%", hum);
    
    // สถานะ
    display.setTextSize(1);
    display.setCursor(0, 54);
    display.print("Status: OK");
    
    display.display();
    
    // Serial output
    Serial.printf("Temp: %.1f°C, Hum: %.1f%%\n", temp, hum);
  } else {
    // แสดงข้อความ Error
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("Sensor Error!");
    display.println("Check RS485 mode");
    display.display();
    
    Serial.println("Sensor read failed!");
  }
  
  delay(2000);
}
```

---

### ตัวอย่างที่ 3: Multiple Sensors

```cpp
#include <Arduino.h>
#include "DevXYMDSensor.h"

// สร้าง object สำหรับ sensor 2 ตัว (Slave ID ต่างกัน)
DevXYMDSensor sensor1(&Serial, 1);  // Slave ID = 1
DevXYMDSensor sensor2(&Serial, 2);  // Slave ID = 2

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("=== Multiple XY-MD03 Sensors ===");
  
  // เริ่มต้นทั้ง 2 sensors
  sensor1.begin(9600);
  sensor2.begin(9600);
  
  delay(2000);
}

void loop() {
  Serial.println("\n=== Reading All Sensors ===");
  
  // อ่านค่าจาก Sensor 1
  Serial.println("\nSensor 1 (ID=1):");
  if (sensor1.update()) {
    Serial.printf("  Temp: %.1f°C, Hum: %.1f%%\n", 
                  sensor1.getTemperature(), 
                  sensor1.getHumidity());
  } else {
    Serial.println("  Read failed!");
  }
  
  delay(500);  // Delay ระหว่าง sensor
  
  // อ่านค่าจาก Sensor 2
  Serial.println("\nSensor 2 (ID=2):");
  if (sensor2.update()) {
    Serial.printf("  Temp: %.1f°C, Hum: %.1f%%\n", 
                  sensor2.getTemperature(), 
                  sensor2.getHumidity());
  } else {
    Serial.println("  Read failed!");
  }
  
  // คำนวณค่าเฉลี่ย
  if (sensor1.isLastReadSuccess() && sensor2.isLastReadSuccess()) {
    float avgTemp = (sensor1.getTemperature() + sensor2.getTemperature()) / 2.0;
    float avgHum = (sensor1.getHumidity() + sensor2.getHumidity()) / 2.0;
    
    Serial.println("\nAverage:");
    Serial.printf("  Temp: %.1f°C, Hum: %.1f%%\n", avgTemp, avgHum);
  }
  
  delay(3000);
}
```

---

## ⚠️ ข้อควรระวัง

### 1. การใช้งาน Serial Port
- ต้อง**สลับ switch ไปโหมด RS485** ก่อนอ่านค่าจากเซนเซอร์
- **สลับกลับเป็น RS232** ก่อนโหลดโปรแกรมใหม่
- ระหว่างอ่านค่า Serial Monitor อาจไม่สามารถแสดงผลได้

### 2. Timing
- แนะนำอ่านค่าทุก **2-3 วินาที** เพื่อความเสถียร
- ไม่ควรอ่านค่าบ่อยเกินไป (อาจทำให้เกิด Error)

### 3. Error Handling
- ตรวจสอบ return value ของ `update()` ทุกครั้ง
- ใช้ `isLastReadSuccess()` เพื่อตรวจสอบสถานะ

### 4. Power Supply
- ตรวจสอบให้แน่ใจว่าเซนเซอร์ได้รับไฟเลี้ยงที่เสถียร
- แรงดันไฟแนะนำ: **5V DC** หรือตามที่ระบุในเซนเซอร์

---

## 🐛 Troubleshooting

### ปัญหา: อ่านค่าไม่สำเร็จ (update() return false)

**วิธีแก้:**
1. ✅ ตรวจสอบว่า switch อยู่ในโหมด **RS485**
2. ✅ ตรวจสอบการเชื่อมต่อสาย RS485 (A, B)
3. ✅ ตรวจสอบ Slave ID ว่าถูกต้อง (default: 1)
4. ✅ ตรวจสอบ Baud Rate ว่าตรงกัน (default: 9600)
5. ✅ ตรวจสอบว่าเซนเซอร์ได้รับไฟเลี้ยง

### ปัญหา: ค่าที่อ่านได้ผิดปกติ

**วิธีแก้:**
1. ✅ ตรวจสอบว่า Register Address ถูกต้อง (0x0001, 0x0002)
2. ✅ ตรวจสอบว่ามีการหารด้วย 10 ถูกต้อง
3. ✅ ลองอ่านค่าใหม่อีกครั้ง (อาจเกิด noise)

### ปัญหา: Serial Monitor ไม่แสดงผล

**วิธีแก้:**
1. ✅ สลับ switch กลับเป็นโหมด **RS232**
2. ✅ ตรวจสอบ Baud Rate ของ Serial Monitor (ต้องตรงกับ `Serial.begin()`)

---

## 📊 Error Codes

| Error Code | Description | Solution |
|-----------|-------------|----------|
| 0xE0 | Invalid Slave ID | ตรวจสอบ Slave ID |
| 0xE1 | Invalid Function | ใช้ Function Code 04 |
| 0xE2 | Invalid Data Address | ตรวจสอบ Register Address |
| 0xE3 | Invalid Data Value | ตรวจสอบค่าที่ส่ง |
| 0xE4 | Slave Device Failure | รีสตาร์ทเซนเซอร์ |

---

## 📝 Class Structure

```
DevXYMDSensor
├── Private Members
│   ├── ModbusMaster modbus
│   ├── uint8_t slaveID
│   ├── HardwareSerial* serial
│   ├── float temperature
│   ├── float humidity
│   ├── bool lastReadSuccess
│   └── unsigned long lastReadTime
│
└── Public Methods
    ├── Constructor
    │   └── DevXYMDSensor(HardwareSerial*, uint8_t)
    │
    ├── Initialization
    │   └── begin(unsigned long)
    │
    ├── Data Reading
    │   └── update()
    │
    ├── Getters
    │   ├── getTemperature()
    │   ├── getHumidity()
    │   ├── isLastReadSuccess()
    │   ├── getLastReadTime()
    │   └── getSlaveID()
    │
    └── Utilities
        ├── printInfo()
        └── setSlaveID(uint8_t)
```

---

## 🔗 References

- [XY-MD03 Datasheet](https://www.google.com/search?q=XY-MD03+datasheet)
- [Modbus Protocol Specification](https://www.modbus.org/)
- [ModbusMaster Library](https://github.com/4-20ma/ModbusMaster)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)

---

## 📄 License

This class is part of ESP32_DEVKIT_V2_CODE_SOLUTION project.

---

## 👤 Author

**ThaiTechZone**
- GitHub: [@thaitechzone](https://github.com/thaitechzone)

---

## 📅 Version History

| Version | Date | Description |
|---------|------|-------------|
| 1.0.0 | 2026-02-01 | Initial release with basic features |

---

**Happy Coding! 🚀**
