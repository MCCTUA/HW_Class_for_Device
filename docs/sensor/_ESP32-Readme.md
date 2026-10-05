# ESP32 DevKit V2 Pin Map และ Library Reference

เอกสารนี้ทำไว้เป็นคู่มืออ้างอิงสำหรับโปรเจกต์ ESP32 DevKit V2.0 ในรูปแบบที่คนอ่านง่าย และ AI เข้าใจง่าย โดยแยกหน้าที่ของขา GPIO, อุปกรณ์ที่ต่อใช้งาน, library ที่จำเป็น และข้อควรระวังสำคัญ

## Project Summary

```yaml
project_name: ESP32_DEVKIT_V2_CODE_SOLUTION
board_family: ESP32 DevKit V2 / DOIT ESP32 DEVKIT V1 compatible
platformio_env: esp32doit-devkit-v1
framework: Arduino
serial_monitor_baud: 9600
main_file: src/main.cpp
sensor_class: DevXYMDSensor
sensor_header: include/DevXYMDSensor.h
primary_sensor: XY-MD03 temperature and humidity sensor
sensor_protocol: Modbus RTU over RS485
display: SSD1306 OLED 128x64 I2C address 0x3C
```

## Hardware ที่ใช้งานในโปรเจกต์ปัจจุบัน

| อุปกรณ์ | หน้าที่ | Protocol / Interface | ไฟล์/Library ที่เกี่ยวข้อง |
|---|---|---|---|
| ESP32 DevKit V2 | Controller หลัก | Arduino Framework | `src/main.cpp` |
| XY-MD03 Sensor | อ่านอุณหภูมิและความชื้น | Modbus RTU / RS485 | `DevXYMDSensor.h`, `ModbusMaster` |
| RS485 Auto Direction Module | แปลง UART เป็น RS485 | UART TTL to RS485 | ใช้ `Serial` |
| OLED SSD1306 128x64 | แสดงค่า Sensor | I2C | `Adafruit_SSD1306`, `Adafruit_GFX`, `Wire` |

## Pin Map สำหรับโปรเจกต์ปัจจุบัน

| GPIO | ชื่อในระบบ | ทิศทาง | ใช้งานกับ | รายละเอียด |
|---:|---|---|---|---|
| GPIO 1 | UART0 TX | Output | RS485 / Serial TX | ส่งข้อมูล Modbus RTU ไปยัง XY-MD03 ผ่าน RS485 |
| GPIO 3 | UART0 RX | Input | RS485 / Serial RX | รับข้อมูล Modbus RTU จาก XY-MD03 ผ่าน RS485 |
| GPIO 21 | SDA | I2C Data | OLED SSD1306 | ขา SDA มาตรฐานของ ESP32 สำหรับ I2C |
| GPIO 22 | SCL | I2C Clock | OLED SSD1306 | ขา SCL มาตรฐานของ ESP32 สำหรับ I2C |
| 3V3 | Power | Output | OLED / logic module | จ่ายไฟ 3.3V ให้โมดูลที่รองรับ 3.3V |
| 5V / VIN | Power | Output/Input | RS485 / Sensor | ใช้เลี้ยงอุปกรณ์ภายนอกตามสเปกโมดูล |
| GND | Ground | Ground | ทุกอุปกรณ์ | ต้องต่อกราวด์ร่วมกันทุกโมดูล |

> หมายเหตุ: โค้ดปัจจุบันใช้ `Serial` สำหรับ XY-MD03 ที่ baud rate 9600 ดังนั้น UART0 จะถูกใช้ทั้ง Serial Monitor และ RS485 ถ้าต้อง upload โปรแกรมหรือ debug ผ่าน USB อาจต้องสลับ switch/โหมด RS232-RS485 ตามวงจรของบอร์ด

## Pin Map สำหรับ ESP32 DevKit V2 โดยรวม

ตารางนี้เป็น reference สำหรับให้ AI หรือผู้พัฒนารู้ว่าขาไหนเหมาะกับงานใด

### Digital GPIO ทั่วไป

| GPIO | ใช้เป็น Input | ใช้เป็น Output | ข้อควรระวัง |
|---:|---|---|---|
| 2 | ได้ | ได้ | มักต่อ LED บนบอร์ด, เป็น strapping pin บางบอร์ด |
| 4 | ได้ | ได้ | ใช้กับ relay/output ได้ |
| 5 | ได้ | ได้ | เป็น strapping pin, ใช้กับ SPI CS ได้ |
| 12 | ได้ | ได้ | strapping pin, ต้องระวังตอน boot |
| 13 | ได้ | ได้ | ใช้กับ sensor/output ได้ |
| 14 | ได้ | ได้ | ใช้กับ SPI/ทั่วไปได้ |
| 15 | ได้ | ได้ | strapping pin, ต้องระวังตอน boot |
| 16 | ได้ | ได้ | ใช้กับ UART2 RX หรือ GPIO ทั่วไปได้ |
| 17 | ได้ | ได้ | ใช้กับ UART2 TX หรือ GPIO ทั่วไปได้ |
| 18 | ได้ | ได้ | SPI SCK default |
| 19 | ได้ | ได้ | SPI MISO default |
| 21 | ได้ | ได้ | I2C SDA default |
| 22 | ได้ | ได้ | I2C SCL default |
| 23 | ได้ | ได้ | SPI MOSI default |
| 25 | ได้ | ได้ | DAC1, GPIO ทั่วไป |
| 26 | ได้ | ได้ | DAC2, GPIO ทั่วไป |
| 27 | ได้ | ได้ | GPIO ทั่วไป |
| 32 | ได้ | ได้ | ADC1, GPIO ทั่วไป |
| 33 | ได้ | ได้ | ADC1, GPIO ทั่วไป |

### Input-only GPIO

| GPIO | ใช้เป็น Input | ใช้เป็น Output | เหมาะสำหรับ |
|---:|---|---|---|
| 34 | ได้ | ไม่ได้ | Switch, sensor input, ADC |
| 35 | ได้ | ไม่ได้ | Switch, sensor input, ADC |
| 36 | ได้ | ไม่ได้ | ADC, sensor input |
| 39 | ได้ | ไม่ได้ | ADC, sensor input |

> GPIO 34, 35, 36, 39 เป็น input-only และไม่มี internal pull-up/pull-down ในบางกรณี ควรมี resistor ภายนอกถ้าใช้กับ switch หรือ digital input

### ขาที่ควรหลีกเลี่ยงหรือใช้อย่างระวัง

| GPIO | เหตุผล |
|---:|---|
| GPIO 0 | Boot mode / flash mode, กด BOOT มักเกี่ยวข้องกับขานี้ |
| GPIO 1 | UART0 TX ใช้ upload/debug ผ่าน USB Serial |
| GPIO 3 | UART0 RX ใช้ upload/debug ผ่าน USB Serial |
| GPIO 6-11 | ต่อกับ SPI Flash บนโมดูล ESP32 ไม่ควรใช้งาน |
| GPIO 12 | strapping pin มีผลกับ boot voltage selection |
| GPIO 15 | strapping pin มีผลตอน boot |

## Interface Reference

### I2C OLED SSD1306

```yaml
device: OLED SSD1306 128x64
address: 0x3C
interface: I2C
sda: GPIO 21
scl: GPIO 22
libraries:
  - Wire
  - Adafruit GFX Library
  - Adafruit SSD1306
```

ตัวอย่างการกำหนดค่าในโค้ด:

```cpp
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
```

### RS485 / Modbus RTU สำหรับ XY-MD03

```yaml
device: XY-MD03 Temperature and Humidity Sensor
interface: RS485
protocol: Modbus RTU
uart: Serial / UART0
baud_rate: 9600
serial_config: SERIAL_8N1
default_slave_id: 1
current_code_slave_id: 2
function_code: 04 Read Input Registers
temperature_register: 0x0001
humidity_register: 0x0002
scale: raw_value / 10.0
class: DevXYMDSensor
```

ตัวอย่างการใช้งานในโค้ด:

```cpp
#include "DevXYMDSensor.h"

DevXYMDSensor tempSensor(&Serial, 2);

void setup() {
  Serial.begin(9600);
  tempSensor.begin(9600);
}

void loop() {
  if (tempSensor.update()) {
    float temperature = tempSensor.getTemperature();
    float humidity = tempSensor.getHumidity();
  }
}
```

## Library สำคัญสำหรับโปรเจกต์นี้

รายการนี้อ้างอิงจาก `platformio.ini` ปัจจุบัน

```ini
lib_deps =
  adafruit/Adafruit GFX Library
  adafruit/Adafruit SSD1306
  4-20ma/ModbusMaster@^2.0.1
```

| Library | ใช้ทำอะไร | ใช้กับไฟล์ |
|---|---|---|
| `Wire` | I2C communication ของ Arduino core | `src/main.cpp` |
| `Adafruit GFX Library` | วาดตัวอักษร เส้น และกราฟิกพื้นฐาน | `src/main.cpp` |
| `Adafruit SSD1306` | ควบคุมจอ OLED SSD1306 | `src/main.cpp` |
| `ModbusMaster` | อ่าน register จาก XY-MD03 ผ่าน Modbus RTU | `include/DevXYMDSensor.h` |
| `Arduino.h` | ฟังก์ชันพื้นฐานของ Arduino เช่น `millis`, `delay`, `Serial` | ทุกไฟล์หลัก |

## Custom Classes ในโปรเจกต์

| Class | Header | หน้าที่ |
|---|---|---|
| `DevXYMDSensor` | `include/DevXYMDSensor.h` | อ่านค่าอุณหภูมิและความชื้นจาก XY-MD03 ผ่าน Modbus RTU |
| `DevRelay` | `include/DevRelay.h` | ควบคุม relay แบบ active low |
| `DevSwitch` | `include/DevSwitch.h` | อ่าน switch พร้อม debounce |
| `DevIsoInput` | `include/DevIsoInput.h` | อ่าน isolated input พร้อม debounce |

> โค้ดปัจจุบันใน `src/main.cpp` ใช้ `DevXYMDSensor` เป็นหลัก ส่วน class อื่นยังเก็บไว้เป็น utility สำหรับงาน I/O ของบอร์ด

## Pin Map เดิมสำหรับ I/O Board Utility

ถ้ากลับไปใช้ relay, switch และ isolated input สามารถอ้างอิง mapping นี้ได้

| GPIO | ชื่อ | ทิศทาง | Active Level | หน้าที่ |
|---:|---|---|---|---|
| GPIO 34 | SW1 | Input | Active Low | Toggle switch 1 |
| GPIO 35 | SW2 | Input | Active Low | Toggle switch 2 |
| GPIO 32 | SW3 | Input | Active Low | Toggle switch 3 |
| GPIO 33 | ISOIN1 | Input | Active Low | Isolated input 1 |
| GPIO 27 | ISOIN2 | Input | Active Low | Isolated input 2 |
| GPIO 17 | RL1 | Output | Active Low | Relay 1 |
| GPIO 16 | RL2 | Output | Active Low | Relay 2 |
| GPIO 4 | RL3 | Output | Active Low | Relay 3 |
| GPIO 12 | AUX1 | Output | Active High | TTL output 3.3V |
| GPIO 14 | AUX2 | Output | Active High | TTL output 3.3V |
| GPIO 15 | AUX3 | Output | Active High | TTL output 3.3V |
| GPIO 25 | AUX4 | Output | Active High | TTL output 3.3V |

## ข้อควรระวังสำหรับ AI เวลาแก้โค้ด

1. ถ้าแก้ class sensor ให้ใช้ชื่อ `DevXYMDSensor` ไม่ใช่ชื่อเก่า `DevTempHumidity`
2. ถ้าเพิ่ม UART ใหม่ แนะนำใช้ `Serial2` บน GPIO 16/17 เพื่อไม่ชนกับ USB Serial
3. ถ้าใช้ GPIO 34/35/36/39 อย่าตั้งเป็น output เพราะเป็น input-only
4. ถ้าใช้ relay active low ให้จำว่า `LOW = ON` และ `HIGH = OFF`
5. ถ้าใช้ switch active low ให้จำว่า `LOW = pressed/active`
6. หลีกเลี่ยง GPIO 6-11 เพราะใช้กับ flash ภายในโมดูล
7. ถ้าใช้ OLED ให้ตรวจ address `0x3C` ก่อน ถ้าไม่ขึ้นอาจเป็น `0x3D`
8. ถ้า upload ไม่ได้และใช้ RS485 ร่วมกับ UART0 ให้ถอด/สลับ RS485 ออกจาก UART0 ชั่วคราว

## PlatformIO Commands

```powershell
python -m platformio run
python -m platformio run --target upload
python -m platformio device monitor --baud 9600
```

## Suggested Future Improvement

ถ้าต้องการให้ debug ง่ายขึ้น ควรย้าย XY-MD03 จาก `Serial` ไปใช้ `Serial2` เพื่อลดการชนกับ USB Serial Monitor:

```yaml
recommended_uart_for_rs485: Serial2
recommended_rx: GPIO 16
recommended_tx: GPIO 17
debug_uart: Serial / UART0
debug_baud: 115200 or 9600
```

