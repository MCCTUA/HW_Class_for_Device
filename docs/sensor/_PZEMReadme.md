# 📘 คู่มือการใช้งาน DevPZEM Class สำหรับ PZEM-016

## 📋 สารบัญ
- [ภาพรวม](#ภาพรวม)
- [ข้อมูลจำเพาะของ PZEM-016](#ข้อมูลจำเพาะของ-pzem-016)
- [การเชื่อมต่อฮาร์ดแวร์](#การเชื่อมต่อฮาร์ดแวร์)
- [โครงสร้าง Class DevPZEM](#โครงสร้าง-class-devpzem)
- [คำสั่งและฟังก์ชันสำคัญ](#คำสั่งและฟังก์ชันสำคัญ)
- [ตัวอย่างการใช้งาน](#ตัวอย่างการใช้งาน)
- [Modbus Register Map](#modbus-register-map)
- [การแก้ปัญหา](#การแก้ปัญหา)

---

## 🔍 ภาพรวม

**DevPZEM** เป็น Wrapper Class สำหรับเซนเซอร์วัดกำลังไฟฟ้า **PZEM-016** ที่ทำงานบน ESP32 โดยใช้โปรโตคอล **Modbus RTU** ผ่าน **RS485** 

### ✨ คุณสมบัติหลัก
- 📊 วัดค่าแรงดันไฟฟ้า (Voltage)
- ⚡ วัดค่ากระแสไฟฟ้า (Current)
- 💡 วัดกำลังไฟฟ้า (Power)
- 🔋 วัดพลังงานสะสม (Energy)
- 📈 วัดความถี่ (Frequency)
- 📐 วัด Power Factor
- 🔔 ตรวจสอบสถานะ Alarm
- 🔄 รีเซ็ตค่าพลังงานสะสมได้

### 🎯 การใช้งาน
- **Communication**: Modbus RTU (9600 baud, 8N1)
- **Hardware**: MAX13487 Auto Direction RS485 Transceiver
- **Port**: Serial (UART0) - ใช้ร่วมกับการ Upload/Download โค้ด

---

## 📐 ข้อมูลจำเพาะของ PZEM-016

| พารามิเตอร์ | ช่วงการวัด | ความละเอียด |
|-------------|-----------|-------------|
| **แรงดันไฟฟ้า** | 80-260V AC | 0.1V |
| **กระแสไฟฟ้า** | 0-100A | 0.001A |
| **กำลังไฟฟ้า** | 0-23kW | 0.1W |
| **พลังงาน** | 0-9999.99 kWh | 1Wh |
| **ความถี่** | 45-65Hz | 0.1Hz |
| **Power Factor** | 0.00-1.00 | 0.01 |

### 🔌 ข้อมูลการสื่อสาร
- **โปรโตคอล**: Modbus RTU
- **Baud Rate**: 9600 bps
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Slave Address**: 0x01 (default)

---

## 🔧 การเชื่อมต่อฮาร์ดแวร์

### 📡 MAX13487 Auto Direction Mode

```
ESP32           MAX13487         PZEM-016
-----           ---------        ---------
TXD0 (GPIO1) -> DI (Input)
RXD0 (GPIO3) -> RO (Output)
                A            ->  A
                B            ->  B
GND          -> GND          ->  GND
```

### ⚠️ หมายเหตุสำคัญ

1. **Switch Mode**: ต้องมี Switch สลับระหว่าง RS232/RS485
   - **RS232 Mode**: สำหรับ Upload/Download โค้ดผ่าน USB
   - **RS485 Mode**: สำหรับสื่อสารกับ PZEM-016

2. **MAX13487 Features**:
   - ✅ ตรวจจับทิศทางการส่งข้อมูลอัตโนมัติ
   - ✅ ไม่ต้องควบคุม DE/RE pins
   - ✅ ลด EMI ด้วย Slew-rate limiting

3. **Power Supply**:
   - PZEM-016 ต้องมีแหล่งจ่ายไฟแยก
   - รองรับแรงดัน 80-260V AC

---

## 🏗️ โครงสร้าง Class DevPZEM

### 📦 ตัวแปรภายใน (Private)

```cpp
ModbusMaster node;              // Modbus Master object
HardwareSerial* serial;         // ตัวชี้ไปยัง Serial port
uint8_t slaveAddress;           // Modbus slave address (0x01)
bool initialized;               // สถานะการเชื่อมต่อ
bool dataValid;                 // สถานะความถูกต้องของข้อมูล
unsigned long lastReadTime;     // เวลาที่อ่านครั้งล่าสุด
const unsigned long readInterval = 2000;  // อ่านทุก 2 วินาที
```

### 📊 ข้อมูลที่เก็บ

```cpp
float voltage;       // แรงดัน (V)
float current;       // กระแส (A)
float power;         // กำลังไฟฟ้า (W)
float energy;        // พลังงาน (Wh -> kWh)
float frequency;     // ความถี่ (Hz)
float powerFactor;   // Power Factor (0-1)
uint16_t alarmStatus;// สถานะ Alarm
```

---

## 🛠️ คำสั่งและฟังก์ชันสำคัญ

### 1. 🎬 Constructor - สร้าง Object

```cpp
DevPZEM(HardwareSerial* serial = &Serial, uint8_t addr = 0x01)
```

**พารามิเตอร์**:
- `serial`: ตัวชี้ไปยัง HardwareSerial (default: &Serial)
- `addr`: Modbus slave address (default: 0x01)

**ตัวอย่าง**:
```cpp
DevPZEM pzem(&Serial);        // ใช้ Serial (UART0)
DevPZEM pzem(&Serial, 0x02);  // ใช้ Slave Address 0x02
```

---

### 2. 🚀 begin() - เริ่มต้นการทำงาน

```cpp
bool begin()
```

**คืนค่า**:
- `true`: เชื่อมต่อสำเร็จ
- `false`: เชื่อมต่อล้มเหลว

**การทำงาน**:
- ล้างบัฟเฟอร์ Serial
- ตั้งค่า Modbus slave address
- ตั้งค่า callbacks สำหรับ MAX13487
- ทดสอบการเชื่อมต่อ (retry 3 ครั้ง)
- ตรวจสอบแรงดันเริ่มต้น (ต้องอยู่ในช่วง 0-300V)

**ตัวอย่าง**:
```cpp
if (pzem.begin()) {
  Serial.println("PZEM-016 เชื่อมต่อสำเร็จ!");
} else {
  Serial.println("ไม่พบ PZEM-016!");
}
```

**การแก้ปัญหา**:
```
PZEM-016: No response from sensor after 3 attempts
  Check: 1) Switch is in RS485 mode
         2) PZEM wiring (A-A, B-B)
         3) PZEM power supply
```

---

### 3. 🔄 update() - อัพเดตข้อมูล

```cpp
bool update()
```

**คืนค่า**:
- `true`: อ่านข้อมูลสำเร็จ
- `false`: อ่านข้อมูลล้มเหลว

**การทำงาน**:
- ตรวจสอบว่าถึงเวลาอ่านหรือยัง (ทุก 2 วินาที)
- อ่านข้อมูลทั้งหมดจาก PZEM-016
- ตรวจสอบความถูกต้องของข้อมูล
- อัพเดตสถานะ `dataValid`

**ตัวอย่าง**:
```cpp
if (pzem.update()) {
  Serial.println("อ่านข้อมูลสำเร็จ!");
  pzem.printData();  // แสดงข้อมูล
} else {
  Serial.println("อ่านข้อมูลล้มเหลว!");
}
```

**หมายเหตุ**: มี delay 100ms ระหว่างแต่ละคำสั่ง Modbus เพื่อให้ MAX13487 มีเวลาสลับทิศทาง

---

### 4. 🗑️ resetEnergy() - รีเซ็ตพลังงานสะสม

```cpp
bool resetEnergy()
```

**คืนค่า**:
- `true`: รีเซ็ตสำเร็จ
- `false`: รีเซ็ตล้มเหลว

**การทำงาน**:
- ส่งคำสั่ง Write Single Register (0x06) ไปที่ register 0x0042
- รีเซ็ตค่า energy เป็น 0

**ตัวอย่าง**:
```cpp
if (pzem.resetEnergy()) {
  Serial.println("รีเซ็ตพลังงานสำเร็จ!");
}
```

---

### 5. 📖 Getter Methods - อ่านค่าข้อมูล

#### 5.1 สถานะการเชื่อมต่อ

```cpp
bool isInitialized()  // ตรวจสอบว่า begin() สำเร็จหรือไม่
bool isDataValid()    // ตรวจสอบว่ามีข้อมูลที่ถูกต้องหรือไม่
uint8_t getSlaveAddress()  // ดู Modbus slave address
```

#### 5.2 ข้อมูลการวัด

```cpp
float getVoltage()      // แรงดัน (V)
float getCurrent()      // กระแส (A)
float getPower()        // กำลังไฟฟ้า (W)
float getEnergy()       // พลังงานสะสม (kWh)
float getFrequency()    // ความถี่ (Hz)
float getPowerFactor()  // Power Factor (0-1)
uint16_t getAlarmStatus()  // สถานะ Alarm
```

**หมายเหตุ**: ถ้า `dataValid` เป็น false ฟังก์ชันจะคืนค่า 0

**ตัวอย่าง**:
```cpp
if (pzem.isDataValid()) {
  float v = pzem.getVoltage();
  float i = pzem.getCurrent();
  float p = pzem.getPower();
  float e = pzem.getEnergy();
  
  Serial.printf("V: %.2f V, I: %.3f A, P: %.2f W, E: %.3f kWh\n", 
                v, i, p, e);
}
```

---

### 6. 🖨️ printData() - แสดงข้อมูลใน Serial Monitor

```cpp
void printData()
```

**การทำงาน**:
- แสดงข้อมูลทั้งหมดในรูปแบบที่อ่านง่าย

**ตัวอย่างผลลัพธ์**:
```
========== PZEM-016 Data ==========
Voltage:      220.50 V
Current:      1.235 A
Power:        265.20 W
Energy:       12.456 kWh
Frequency:    50.0 Hz
Power Factor: 0.95
Alarm Status: 0x0000
===================================
```

---

### 7. 📤 toJSON() - สร้าง JSON String

```cpp
String toJSON()
```

**คืนค่า**: JSON String ที่มีข้อมูลทั้งหมด

**ตัวอย่างผลลัพธ์**:
```json
{
  "slaveId": 1,
  "voltage": 220.50,
  "current": 1.235,
  "power": 265.20,
  "energy": 12.456,
  "frequency": 50.0,
  "powerFactor": 0.95,
  "alarmStatus": 0,
  "valid": true
}
```

**ตัวอย่างการใช้งาน**:
```cpp
String json = pzem.toJSON();
mqttClient.publish("pzem/data", json);
```

---

## 💡 ตัวอย่างการใช้งาน

### ตัวอย่างที่ 1: การใช้งานพื้นฐาน

```cpp
#include <Arduino.h>
#include "DevPZEM.h"

// สร้าง object
DevPZEM pzem(&Serial);
bool pzem_available = false;

void setup() {
  Serial.begin(9600);  // PZEM-016 ใช้ 9600 baud
  delay(1000);
  
  Serial.println("กำลังเริ่มต้น PZEM-016...");
  
  // เริ่มต้นการทำงาน
  pzem_available = pzem.begin();
  
  if (pzem_available) {
    Serial.println("✓ PZEM-016 พร้อมใช้งาน!");
  } else {
    Serial.println("✗ ไม่พบ PZEM-016!");
    Serial.println("  ตรวจสอบ: Switch อยู่ในโหมด RS485 หรือไม่?");
  }
}

void loop() {
  if (pzem_available) {
    // อัพเดตข้อมูล
    if (pzem.update()) {
      // แสดงข้อมูล
      pzem.printData();
    }
  }
  
  delay(3000);  // อ่านทุก 3 วินาที
}
```

---

### ตัวอย่างที่ 2: การใช้งานกับ MQTT

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DevPZEM.h"

DevPZEM pzem(&Serial);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char* MQTT_TOPIC_PZEM = "esp32/pzem";

void publishPZEMData() {
  if (!pzem.isDataValid()) {
    Serial.println("ไม่มีข้อมูล PZEM ที่ถูกต้อง");
    return;
  }
  
  // ส่งข้อมูลแบบ JSON
  String json = pzem.toJSON();
  mqttClient.publish(MQTT_TOPIC_PZEM, json.c_str());
  Serial.println("✓ ส่งข้อมูล PZEM ผ่าน MQTT แล้ว");
  
  // หรือส่งแยกเป็นแต่ละ topic
  mqttClient.publish(
    (String(MQTT_TOPIC_PZEM) + "/voltage").c_str(), 
    String(pzem.getVoltage(), 2).c_str()
  );
  mqttClient.publish(
    (String(MQTT_TOPIC_PZEM) + "/current").c_str(), 
    String(pzem.getCurrent(), 3).c_str()
  );
  mqttClient.publish(
    (String(MQTT_TOPIC_PZEM) + "/power").c_str(), 
    String(pzem.getPower(), 2).c_str()
  );
  mqttClient.publish(
    (String(MQTT_TOPIC_PZEM) + "/energy").c_str(), 
    String(pzem.getEnergy(), 3).c_str()
  );
}

void loop() {
  if (pzem.update()) {
    publishPZEMData();
  }
  delay(3000);
}
```

---

### ตัวอย่างที่ 3: การใช้งานกับ OLED Display

```cpp
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "DevPZEM.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DevPZEM pzem(&Serial);

void displayPZEMData() {
  if (!pzem.isDataValid()) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("PZEM: No Data");
    display.display();
    return;
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println("=== PZEM-016 ===");
  display.println();
  display.printf("V: %.1f V\n", pzem.getVoltage());
  display.printf("I: %.2f A\n", pzem.getCurrent());
  display.printf("P: %.1f W\n", pzem.getPower());
  display.printf("E: %.2f kWh\n", pzem.getEnergy());
  display.printf("PF: %.2f\n", pzem.getPowerFactor());
  
  display.display();
}

void loop() {
  if (pzem.update()) {
    displayPZEMData();
  }
  delay(2000);
}
```

---

### ตัวอย่างที่ 4: การรีเซ็ตพลังงาน

```cpp
#include <Arduino.h>
#include "DevPZEM.h"

DevPZEM pzem(&Serial);

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  pzem.begin();
}

void loop() {
  pzem.update();
  
  // ตรวจสอบ Serial input
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd == 'r' || cmd == 'R') {
      Serial.println("กำลังรีเซ็ตพลังงานสะสม...");
      
      if (pzem.resetEnergy()) {
        Serial.println("✓ รีเซ็ตพลังงานสำเร็จ!");
        Serial.printf("  พลังงานปัจจุบัน: %.3f kWh\n", 
                      pzem.getEnergy());
      } else {
        Serial.println("✗ รีเซ็ตพลังงานล้มเหลว!");
      }
    }
  }
  
  delay(2000);
}
```

---

### ตัวอย่างที่ 5: การบันทึกข้อมูลลง NVS

```cpp
#include <Arduino.h>
#include <Preferences.h>
#include "DevPZEM.h"

DevPZEM pzem(&Serial);
Preferences preferences;

const char* NVS_NAMESPACE = "pzem";
const char* NVS_KEY_ENERGY = "energy";

void savePZEMEnergy() {
  if (!pzem.isDataValid()) return;
  
  preferences.begin(NVS_NAMESPACE, false);
  float energy = pzem.getEnergy();
  preferences.putFloat(NVS_KEY_ENERGY, energy);
  preferences.end();
  
  Serial.printf("✓ บันทึกพลังงาน: %.3f kWh\n", energy);
}

float loadPZEMEnergy() {
  preferences.begin(NVS_NAMESPACE, true);
  float energy = preferences.getFloat(NVS_KEY_ENERGY, 0.0);
  preferences.end();
  
  Serial.printf("✓ โหลดพลังงาน: %.3f kWh\n", energy);
  return energy;
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  pzem.begin();
  
  // โหลดค่าพลังงานที่บันทึกไว้
  float savedEnergy = loadPZEMEnergy();
  Serial.printf("พลังงานที่บันทึก: %.3f kWh\n", savedEnergy);
}

void loop() {
  static unsigned long lastSave = 0;
  
  pzem.update();
  
  // บันทึกทุก 5 นาที
  if (millis() - lastSave >= 300000) {
    lastSave = millis();
    savePZEMEnergy();
  }
  
  delay(3000);
}
```

---

## 📊 Modbus Register Map

| Register | ชื่อ | จำนวน Register | ประเภท | Scale | หน่วย |
|----------|------|----------------|--------|-------|-------|
| 0x0000 | Voltage | 1 | 16-bit | 0.1 | V |
| 0x0001 | Current | 2 | 32-bit | 0.001 | A |
| 0x0003 | Power | 2 | 32-bit | 0.1 | W |
| 0x0005 | Energy | 2 | 32-bit | 1 | Wh |
| 0x0007 | Frequency | 1 | 16-bit | 0.1 | Hz |
| 0x0008 | Power Factor | 1 | 16-bit | 0.01 | - |
| 0x0009 | Alarm Status | 1 | 16-bit | - | - |
| 0x0042 | Reset Energy | 1 | Write | - | - |

### 📝 หมายเหตุ
- **32-bit values**: PZEM-016 ส่งข้อมูล Low Word ก่อน, High Word ทีหลัง
- **Function Codes**: 
  - Read: 0x04 (Read Input Registers)
  - Write: 0x06 (Write Single Register)

### 🔍 ตัวอย่างการอ่าน 32-bit

```cpp
uint32_t read32BitValue(uint16_t startReg) {
  node.readInputRegisters(startReg, 2);
  
  // Register[0] = Low 16 bits
  // Register[1] = High 16 bits
  uint32_t value = node.getResponseBuffer(0) | 
                   ((uint32_t)node.getResponseBuffer(1) << 16);
  
  return value;
}
```

---

## 🔧 การแก้ปัญหา

### ❌ ปัญหา: ไม่พบ PZEM-016

**อาการ**:
```
PZEM-016: No response from sensor after 3 attempts
```

**สาเหตุและแก้ไข**:

1. **Switch ไม่ได้อยู่ในโหมด RS485**
   - ✅ ตรวจสอบ Switch ว่าอยู่ในตำแหน่ง RS485
   - ✅ Upload โค้ดเสร็จแล้วสลับ Switch จาก RS232 -> RS485

2. **การต่อสายไม่ถูกต้อง**
   - ✅ ตรวจสอบการต่อสาย A-A, B-B
   - ✅ ตรวจสอบการต่อ GND
   - ✅ ตรวจสอบการต่อสายของ MAX13487

3. **PZEM-016 ไม่มีไฟเลี้ยง**
   - ✅ ตรวจสอบว่า PZEM มีไฟเข้า 80-260V AC
   - ✅ ตรวจสอบไฟ LED บน PZEM ติดหรือไม่

4. **Baud Rate ไม่ตรงกัน**
   - ✅ ตรวจสอบว่า Serial.begin(9600)
   - ✅ PZEM-016 ใช้ 9600 baud เท่านั้น

---

### ❌ ปัญหา: อ่านข้อมูลได้แต่ค่าผิดปกติ

**อาการ**:
- ค่าแรงดันเป็น 0 หรือเกิน 300V
- ค่ากระแสติดลบ
- ข้อมูลกระโดด (unstable)

**แก้ไข**:

1. **ตรวจสอบแหล่งจ่ายไฟ**
   - ✅ PZEM ต้องมีโหลดจริงเชื่อมต่อ
   - ✅ แรงดันต้องอยู่ในช่วง 80-260V AC

2. **ปัญหา EMI/Noise**
   - ✅ ใช้สาย Twisted Pair สำหรับ RS485
   - ✅ เพิ่ม Termination Resistor (120Ω) ถ้าสายยาวเกิน 5 เมตร
   - ✅ แยกสาย Power จากสาย Signal

3. **Timing Issues**
   - ✅ เพิ่ม delay ระหว่างการอ่าน (100ms)
   - ✅ อ่านข้อมูลไม่เร็วเกินไป (แนะนำ 2-3 วินาที)

---

### ❌ ปัญหา: ไม่สามารถ Upload โค้ดได้

**อาการ**:
```
A fatal error occurred: Failed to connect to ESP32
```

**แก้ไข**:
1. ✅ สลับ Switch เป็นโหมด **RS232**
2. ✅ ถอดสาย PZEM ออกชั่วคราว
3. ✅ Upload โค้ดใหม่
4. ✅ สลับ Switch กลับเป็น **RS485**
5. ✅ ต่อสาย PZEM กลับ
6. ✅ Reset ESP32

---

### ❌ ปัญหา: Data Valid แต่ไม่สามารถ Reset Energy ได้

**อาการ**:
```
PZEM: Failed to reset energy (0x02)
```

**แก้ไข**:

1. **ตรวจสอบ Write Permission**
   - ✅ บาง PZEM มี Jumper สำหรับ Write Protection
   - ✅ ตรวจสอบ Datasheet

2. **ลองรีเซ็ตหลายครั้ง**
   ```cpp
   for (int i = 0; i < 3; i++) {
     if (pzem.resetEnergy()) {
       Serial.println("✓ รีเซ็ตสำเร็จ!");
       break;
     }
     delay(1000);
   }
   ```

---

### 🔍 การ Debug

**เปิดใช้งาน Debug Messages**:

```cpp
void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(true);  // เปิด debug
  
  pzem.begin();
}
```

**ตรวจสอบ Raw Data**:

```cpp
void loop() {
  if (pzem.update()) {
    Serial.println("========== Raw PZEM Data ==========");
    Serial.printf("Initialized: %s\n", pzem.isInitialized() ? "YES" : "NO");
    Serial.printf("Data Valid: %s\n", pzem.isDataValid() ? "YES" : "NO");
    Serial.printf("Slave Addr: 0x%02X\n", pzem.getSlaveAddress());
    Serial.println("===================================");
    pzem.printData();
  }
  delay(3000);
}
```

---

## � Source Code: DevPZEM.h

### 💻 Header File แบบเต็ม

```cpp
/**
 * @file DevPZEM.h
 * @brief PZEM-016 AC Power Monitor wrapper class for ESP32 using ModbusMaster
 * @note Uses the same Serial port as programming (Serial/UART0)
 * @note Uses MAX13487 for RS232 to RS485 conversion (Auto Direction)
 * 
 * PZEM-016 Specifications:
 * - Voltage: 80-260V AC
 * - Current: 0-100A (with external CT)
 * - Power: 0-23kW
 * - Communication: Modbus RTU (9600 8N1)
 * - Slave Address: 0x01 (default)
 * 
 * Modbus Register Map:
 * - 0x0000: Voltage (V) - 1 register, scale: 0.1
 * - 0x0001: Current (A) - 2 registers (32-bit), scale: 0.001
 * - 0x0003: Power (W) - 2 registers (32-bit), scale: 0.1
 * - 0x0005: Energy (Wh) - 2 registers (32-bit), scale: 1
 * - 0x0007: Frequency (Hz) - 1 register, scale: 0.1
 * - 0x0008: Power Factor - 1 register, scale: 0.01
 * - 0x0009: Alarm Status - 1 register
 * 
 * Hardware Connection (MAX13487 Auto Direction):
 * - ESP32 TXD0 (GPIO1) -> MAX13487 DI (Driver Input)
 * - ESP32 RXD0 (GPIO3) -> MAX13487 RO (Receiver Output)
 * - MAX13487 A -> PZEM-016 A
 * - MAX13487 B -> PZEM-016 B
 * 
 * MAX13487 Features:
 * - Auto direction detection (no DE/RE control needed)
 * - Slew-rate limited for reduced EMI
 * - Works with standard UART (no GPIO for direction control)
 */

#ifndef DEVPZEM_H
#define DEVPZEM_H

#include <Arduino.h>
#include <ModbusMaster.h>

// Callback functions for ModbusMaster (MAX13487 auto direction - no control needed)
void preTransmission() {
  // MAX13487 handles direction automatically - no action needed
  // Just ensure any pending data is sent
  Serial.flush();
}

void postTransmission() {
  // MAX13487 handles direction automatically - no action needed  
  // Small delay to ensure transmission is complete before listening
  delayMicroseconds(100);
}

class DevPZEM {
private:
  ModbusMaster node;
  HardwareSerial* serial;
  uint8_t slaveAddress;
  bool initialized;
  bool dataValid;
  unsigned long lastReadTime;
  const unsigned long readInterval = 2000;  // อ่านทุก 2 วินาที
  
  // ข้อมูลที่อ่านได้
  float voltage;      // แรงดัน (V)
  float current;      // กระแส (A)
  float power;        // กำลังไฟฟ้า (W)
  float energy;       // พลังงาน (Wh -> converted to kWh)
  float frequency;    // ความถี่ (Hz)
  float powerFactor;  // Power Factor (0-1)
  uint16_t alarmStatus; // Alarm status
  
  /**
   * @brief อ่านค่า 32-bit จาก 2 registers ต่อเนื่อง
   * @param startReg Register เริ่มต้น
   * @return ค่า 32-bit
   * @note PZEM-016 uses Low Word first, High Word second
   */
  uint32_t read32BitValue(uint16_t startReg) {
    uint8_t result = node.readInputRegisters(startReg, 2);
    if (result == node.ku8MBSuccess) {
      // PZEM-016: Register[0] = Low 16 bits, Register[1] = High 16 bits
      uint32_t value = node.getResponseBuffer(0) | ((uint32_t)node.getResponseBuffer(1) << 16);
      return value;
    }
    return 0;
  }

public:
  /**
   * @brief Constructor - สร้าง PZEM object โดยใช้ ModbusMaster
   * @param serial ตัวชี้ไปยัง HardwareSerial object (default: &Serial)
   * @param addr Modbus slave address ของ PZEM (default: 0x01)
   */
  DevPZEM(HardwareSerial* serial = &Serial, uint8_t addr = 0x01) {
    this->serial = serial;
    this->slaveAddress = addr;
    initialized = false;
    dataValid = false;
    lastReadTime = 0;
    voltage = 0.0;
    current = 0.0;
    power = 0.0;
    energy = 0.0;
    frequency = 0.0;
    powerFactor = 0.0;
    alarmStatus = 0;
  }

  /**
   * @brief Initialize PZEM sensor
   * @return true ถ้าสำเร็จ, false ถ้าล้มเหลว
   */
  bool begin() {
    // Clear serial buffer
    while (serial->available()) {
      serial->read();
    }
    
    // กำหนด Modbus slave address
    node.begin(slaveAddress, *serial);
    
    // Set callbacks for MAX13487 (auto direction - just flush/delay)
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    
    delay(200);  // Wait for serial and MAX13487 to stabilize
    
    // ทดสอบการเชื่อมต่อโดยพยายามอ่านแรงดัน (retry 3 times)
    for (int attempt = 0; attempt < 3; attempt++) {
      uint8_t result = node.readInputRegisters(0x0000, 1);
      
      if (result == node.ku8MBSuccess) {
        uint16_t rawVoltage = node.getResponseBuffer(0);
        if (rawVoltage > 0 && rawVoltage < 3000) {  // ช่วง 0-300V (raw: 0-3000)
          initialized = true;
          dataValid = false;  // ยังไม่ได้อ่านครบทุกค่า
          Serial.printf("PZEM-016: Initialized successfully (Raw V=%d)\n", rawVoltage);
          Serial.println("  Using MAX13487 Auto Direction Mode");
          return true;
        }
      }
      
      Serial.printf("  Attempt %d failed (0x%02X), retrying...\n", attempt + 1, result);
      delay(100);
    }
    
    initialized = false;
    dataValid = false;
    Serial.println("PZEM-016: No response from sensor after 3 attempts");
    Serial.println("  Check: 1) Switch is in RS485 mode");
    Serial.println("         2) PZEM wiring (A-A, B-B)");
    Serial.println("         3) PZEM power supply");
    return false;
  }

  /**
   * @brief อ่านค่าทั้งหมดจาก PZEM
   * @return true ถ้าอ่านสำเร็จ, false ถ้าล้มเหลว
   */
  bool update() {
    if (!initialized) {
      return false;
    }

    // ตรวจสอบว่าถึงเวลาอ่านหรือยัง
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < readInterval) {
      return dataValid;  // ยังไม่ถึงเวลาอ่าน ใช้ค่าเดิม
    }
    lastReadTime = currentTime;

    // Clear any stale data in serial buffer
    while (serial->available()) {
      serial->read();
    }

    bool readSuccess = true;
    
    // อ่าน Voltage (Register 0x0000, 1 register)
    uint8_t result = node.readInputRegisters(0x0000, 1);
    if (result == node.ku8MBSuccess) {
      voltage = node.getResponseBuffer(0) * 0.1;  // Scale: 0.1
    } else {
      readSuccess = false;
      Serial.printf("PZEM: Failed to read voltage (0x%02X)\n", result);
    }
    
    delay(100);  // Delay between requests (MAX13487 needs time to switch direction)
    
    // อ่าน Current (Register 0x0001, 2 registers, 32-bit)
    uint32_t rawCurrent = read32BitValue(0x0001);
    current = rawCurrent * 0.001;  // Scale: 0.001 A
    
    delay(100);
    
    // อ่าน Power (Register 0x0003, 2 registers, 32-bit)
    uint32_t rawPower = read32BitValue(0x0003);
    power = rawPower * 0.1;  // Scale: 0.1
    
    delay(100);
    
    // อ่าน Energy (Register 0x0005, 2 registers, 32-bit)
    uint32_t rawEnergy = read32BitValue(0x0005);
    energy = rawEnergy;  // Wh (will convert to kWh when needed)
    
    delay(100);
    
    // อ่าน Frequency (Register 0x0007, 1 register)
    result = node.readInputRegisters(0x0007, 1);
    if (result == node.ku8MBSuccess) {
      frequency = node.getResponseBuffer(0) * 0.1;  // Scale: 0.1
    }
    
    delay(100);
    
    // อ่าน Power Factor (Register 0x0008, 1 register)
    result = node.readInputRegisters(0x0008, 1);
    if (result == node.ku8MBSuccess) {
      powerFactor = node.getResponseBuffer(0) * 0.01;  // Scale: 0.01
    }
    
    delay(100);
    
    // อ่าน Alarm Status (Register 0x0009, 1 register)
    result = node.readInputRegisters(0x0009, 1);
    if (result == node.ku8MBSuccess) {
      alarmStatus = node.getResponseBuffer(0);
    }

    // ตรวจสอบความถูกต้องของข้อมูล
    if (!readSuccess || voltage < 0 || voltage > 300) {
      dataValid = false;
      Serial.println("PZEM: Data validation failed");
      return false;
    }

    dataValid = true;
    return true;
  }

  /**
   * @brief รีเซ็ตค่า Energy counter
   * @return true ถ้าสำเร็จ, false ถ้าล้มเหลว
   */
  bool resetEnergy() {
    if (!initialized) {
      return false;
    }
    
    // PZEM-016 Reset Command: Write to register 0x0042
    // Using function code 0x06 (Write Single Register)
    uint8_t result = node.writeSingleRegister(0x0042, 0x0000);
    
    if (result == node.ku8MBSuccess) {
      energy = 0.0;
      Serial.println("PZEM: Energy counter reset successfully");
      return true;
    }
    
    Serial.printf("PZEM: Failed to reset energy (0x%02X)\n", result);
    return false;
  }

  // Getter methods
  bool isInitialized() const { return initialized; }
  bool isDataValid() const { return dataValid; }
  uint8_t getSlaveAddress() const { return slaveAddress; }
  float getVoltage() const { return dataValid ? voltage : 0.0; }
  float getCurrent() const { return dataValid ? current : 0.0; }
  float getPower() const { return dataValid ? power : 0.0; }
  float getEnergy() const { return dataValid ? (energy / 1000.0) : 0.0; }  // Convert Wh to kWh
  float getFrequency() const { return dataValid ? frequency : 0.0; }
  float getPowerFactor() const { return dataValid ? powerFactor : 0.0; }
  uint16_t getAlarmStatus() const { return dataValid ? alarmStatus : 0; }

  /**
   * @brief แสดงข้อมูลทั้งหมดใน Serial Monitor
   */
  void printData() {
    if (!dataValid) {
      Serial.println("PZEM: No valid data");
      return;
    }

    Serial.println("========== PZEM-016 Data ==========");
    Serial.printf("Voltage:      %.2f V\n", voltage);
    Serial.printf("Current:      %.3f A\n", current);
    Serial.printf("Power:        %.2f W\n", power);
    Serial.printf("Energy:       %.3f kWh\n", getEnergy());
    Serial.printf("Frequency:    %.1f Hz\n", frequency);
    Serial.printf("Power Factor: %.2f\n", powerFactor);
    Serial.printf("Alarm Status: 0x%04X\n", alarmStatus);
    Serial.println("===================================");
  }

  /**
   * @brief สร้าง JSON string สำหรับส่งข้อมูล
   * @return JSON string
   */
  String toJSON() {
    String json = "{";
    json += "\"slaveId\":" + String(slaveAddress) + ",";
    json += "\"voltage\":" + String(voltage, 2) + ",";
    json += "\"current\":" + String(current, 3) + ",";
    json += "\"power\":" + String(power, 2) + ",";
    json += "\"energy\":" + String(getEnergy(), 3) + ",";
    json += "\"frequency\":" + String(frequency, 1) + ",";
    json += "\"powerFactor\":" + String(powerFactor, 2) + ",";
    json += "\"alarmStatus\":" + String(alarmStatus) + ",";
    json += "\"valid\":" + String(dataValid ? "true" : "false");
    json += "}";
    return json;
  }
};

#endif // DEVPZEM_H
```

### 🔍 อธิบาย Implementation สำคัญ

#### 1. Callback Functions สำหรับ MAX13487

```cpp
void preTransmission() {
  Serial.flush();  // รอให้ส่งข้อมูลออกจาก buffer
}

void postTransmission() {
  delayMicroseconds(100);  // รอให้การส่งข้อมูลเสร็จสมบูรณ์
}
```

**คำอธิบาย**: MAX13487 จัดการทิศทางอัตโนมัติ ไม่ต้องควบคุม DE/RE pins

#### 2. การอ่านค่า 32-bit (Private Method)

```cpp
uint32_t read32BitValue(uint16_t startReg) {
  uint8_t result = node.readInputRegisters(startReg, 2);
  if (result == node.ku8MBSuccess) {
    // PZEM-016 ส่ง Low Word ก่อน, High Word ทีหลัง
    uint32_t value = node.getResponseBuffer(0) | 
                     ((uint32_t)node.getResponseBuffer(1) << 16);
    return value;
  }
  return 0;
}
```

**ใช้สำหรับ**: Current, Power, Energy (ข้อมูล 32-bit)

#### 3. การตรวจสอบการเชื่อมต่อ (begin)

```cpp
// Retry 3 ครั้ง
for (int attempt = 0; attempt < 3; attempt++) {
  uint8_t result = node.readInputRegisters(0x0000, 1);
  
  if (result == node.ku8MBSuccess) {
    uint16_t rawVoltage = node.getResponseBuffer(0);
    // ตรวจสอบว่าแรงดันอยู่ในช่วงที่เป็นไปได้ (0-300V)
    if (rawVoltage > 0 && rawVoltage < 3000) {
      return true;  // สำเร็จ
    }
  }
  
  delay(100);  // รอก่อน retry
}
```

#### 4. Rate Limiting (update)

```cpp
unsigned long currentTime = millis();
if (currentTime - lastReadTime < readInterval) {
  return dataValid;  // ยังไม่ถึงเวลา ใช้ค่าเดิม
}
lastReadTime = currentTime;
```

**เหตุผล**: ป้องกันการอ่านบ่อยเกินไป (อ่านทุก 2 วินาที)

#### 5. Data Validation

```cpp
// ตรวจสอบความถูกต้อง
if (!readSuccess || voltage < 0 || voltage > 300) {
  dataValid = false;
  return false;
}
dataValid = true;
```

#### 6. Getter Methods with Safety Check

```cpp
float getVoltage() const { 
  return dataValid ? voltage : 0.0; 
}
```

**คืนค่า**: ถ้าข้อมูลไม่ valid คืนค่า 0 แทน

#### 7. Energy Unit Conversion

```cpp
float getEnergy() const { 
  return dataValid ? (energy / 1000.0) : 0.0;  // Wh -> kWh
}
```

**หน่วย**: PZEM เก็บเป็น Wh แต่ Getter คืนค่าเป็น kWh

#### 8. Reset Energy Command

```cpp
// Write Single Register (Function 0x06)
uint8_t result = node.writeSingleRegister(0x0042, 0x0000);
```

**Register 0x0042**: Special register สำหรับรีเซ็ต Energy counter

---

## �📚 เอกสารอ้างอิง

### 📖 Libraries ที่ใช้
- **ModbusMaster**: สำหรับสื่อสาร Modbus RTU
  - GitHub: https://github.com/4-20ma/ModbusMaster

### 🔗 Datasheets
- **PZEM-016**: AC Power Monitor
- **MAX13487**: Auto Direction RS485 Transceiver

### 🎓 Modbus Protocol
- **Function Code 0x04**: Read Input Registers
- **Function Code 0x06**: Write Single Register

---

## ✅ Checklist การใช้งาน

### เมื่อเริ่มใช้งานครั้งแรก

- [ ] ตรวจสอบการต่อสาย A-A, B-B
- [ ] ตรวจสอบการต่อ GND
- [ ] ตรวจสอบแหล่งจ่ายไฟ PZEM (80-260V AC)
- [ ] สลับ Switch เป็น RS232 mode
- [ ] Upload โค้ด
- [ ] สลับ Switch เป็น RS485 mode
- [ ] Reset ESP32
- [ ] ตรวจสอบ Serial Monitor ว่า PZEM ตอบสนอง

### การใช้งานประจำ

- [ ] เรียก `pzem.begin()` ใน `setup()`
- [ ] เรียก `pzem.update()` ใน `loop()` (ทุก 2-3 วินาที)
- [ ] ตรวจสอบ `isDataValid()` ก่อนอ่านค่า
- [ ] บันทึกค่า Energy ลง NVS ทุก 5 นาที (ถ้าต้องการ)

### เมื่อต้องการ Upload โค้ดใหม่

- [ ] สลับ Switch เป็น RS232 mode
- [ ] Upload โค้ด
- [ ] สลับ Switch เป็น RS485 mode
- [ ] Reset ESP32

---

## 🎯 Best Practices

### ⚡ Performance

1. **การอ่านข้อมูล**
   - อ่านทุก 2-3 วินาที (ไม่ควรเร็วเกินไป)
   - ใช้ `update()` แทนการอ่านแยกเป็นค่าๆ
   - มี delay 100ms ระหว่างคำสั่ง Modbus

2. **หน่วยความจำ**
   - ใช้ `getEnergy()` (คืนค่าเป็น kWh)
   - เก็บค่า Energy เป็น float แทน uint32_t

### 🔒 Reliability

1. **การตรวจสอบข้อมูล**
   ```cpp
   if (pzem.isInitialized() && pzem.isDataValid()) {
     // ใช้ข้อมูลได้
   }
   ```

2. **การจัดการ Error**
   ```cpp
   if (!pzem.update()) {
     Serial.println("อ่านข้อมูลล้มเหลว - ตรวจสอบการเชื่อมต่อ");
     // พยายามเชื่อมต่อใหม่
     pzem.begin();
   }
   ```

3. **การบันทึกข้อมูล**
   - บันทึก Energy ลง NVS เป็นระยะ
   - ไม่ควรบันทึกบ่อยเกินไป (แนะนำ 5 นาที)

---

## 📌 สรุป

### ข้อดี ✅
- ✅ ใช้งานง่าย - เรียก begin() และ update()
- ✅ อ่านครบทุกค่า - Voltage, Current, Power, Energy, Frequency, PF
- ✅ รองรับ JSON - ส่งข้อมูลผ่าน MQTT ได้ทันที
- ✅ Auto Direction - ไม่ต้องควบคุม DE/RE pins
- ✅ Validation - ตรวจสอบความถูกต้องของข้อมูล

### ข้อจำกัด ⚠️
- ⚠️ ใช้ Serial (UART0) ร่วมกับ Upload/Download
- ⚠️ ต้องมี Switch สลับ RS232/RS485
- ⚠️ อ่านข้อมูลทุก 2 วินาที (ไม่ควรอ่านเร็วกว่านี้)
- ⚠️ ต้องมีโหลดจริง (แรงดัน 80-260V AC)

---

## 🚀 การพัฒนาต่อ

### แนวทางการขยายความสามารถ

1. **Multi-PZEM Support**
   - รองรับหลาย PZEM ด้วย Slave Address ต่างกัน
   - ใช้ Software Serial สำหรับ PZEM ตัวที่สอง

2. **Data Logging**
   - บันทึกข้อมูลลง SD Card
   - สร้าง CSV file สำหรับ Excel

3. **Web Dashboard**
   - สร้าง Real-time Chart ด้วย Chart.js
   - แสดง Historical Data

4. **Alert System**
   - ตรวจจับ Over Voltage/Current
   - ส่ง Notification ผ่าน Telegram/LINE

5. **Integration**
   - เชื่อมต่อ Home Assistant
   - เชื่อมต่อ ThingsBoard
   - เชื่อมต่อ Grafana

---

## 📞 ติดต่อ/สนับสนุน

หากมีปัญหาหรือข้อสงสัย:
- 📧 เปิด Issue ใน GitHub Repository
- 📚 อ่าน Datasheet ของ PZEM-016
- 🔍 ค้นหาใน Forum/Community

---

**สร้างโดย**: Thai Tech Zone  
**วันที่**: 5 มกราคม 2026  
**เวอร์ชัน**: 1.0  
**License**: MIT

---

_เอกสารนี้สร้างขึ้นเพื่อเป็นพิมพ์เขียว (Blueprint) สำหรับการนำไปออกแบบต่อยอดด้วย AI และการพัฒนาโปรเจกต์ที่เกี่ยวข้องกับ PZEM-016 AC Power Monitor_
