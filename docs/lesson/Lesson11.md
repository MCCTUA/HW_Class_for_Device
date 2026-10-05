# Lesson 11: เชื่อมต่อ MQTT เพื่อส่งข้อมูล Sensor และสถานะอุปกรณ์

## เป้าหมาย

เชื่อมต่อ ESP32 DevKit V2 เข้ากับ Wi-Fi และ MQTT broker เพื่อ publish ข้อมูลจากอุปกรณ์ต่าง ๆ ในบอร์ด ได้แก่ DS18B20, XY-MD03, PZEM-016, relay, switch และ isolated input

บทนี้ออกแบบเป็น markdown สำหรับให้ AI ใช้เป็นโจทย์ในการช่วยเขียนโปรแกรมทีละขั้น โดยต้องอ้างอิง `blueprint_v2026.md` เป็นหลัก

## สิ่งที่ต้องมี

- ESP32 DevKit V2
- Wi-Fi 2.4GHz
- MQTT broker เช่น Mosquitto, EMQX, HiveMQ, ThingsBoard หรือ Home Assistant MQTT
- Library เพิ่มเติมสำหรับ MQTT

## Library ที่ต้องเพิ่ม

ใน `platformio.ini` ให้มี library เดิมจาก blueprint และเพิ่ม `PubSubClient`

```ini
lib_deps =
  4-20ma/ModbusMaster@^2.0.1
  adafruit/Adafruit GFX Library
  adafruit/Adafruit SSD1306
  paulstoffregen/OneWire
  milesburton/DallasTemperature
  knolleary/PubSubClient
```

## Hardware Policy ที่ต้องจำ

- PZEM-016 และ XY-MD03 ใช้ `Serial` / UART0 ผ่าน RS485 switch ภายนอก
- ห้ามย้ายไป `Serial2` ถ้า user ไม่สั่งเปลี่ยน hardware
- Upload firmware ให้สลับ switch เป็น RS232
- อ่าน Modbus sensor ให้สลับ switch เป็น RS485
- DS18B20 ใช้ GPIO14
- OLED ใช้ GPIO21/GPIO22
- Relay เป็น active low

## Topic Design

แนะนำ topic แบบอ่านง่ายและต่อ dashboard ได้สะดวก

| Topic | Payload | แหล่งข้อมูล |
|---|---|---|
| `esp32/devkit/status` | `online`, `offline` | MQTT availability |
| `esp32/devkit/ds18b20/temperature` | temperature Celsius | DS18B20 GPIO14 |
| `esp32/devkit/xymd/temperature` | temperature Celsius | XY-MD03 |
| `esp32/devkit/xymd/humidity` | humidity percent | XY-MD03 |
| `esp32/devkit/pzem/json` | JSON object | `pzem.toJSON()` |
| `esp32/devkit/pzem/voltage` | voltage | PZEM |
| `esp32/devkit/pzem/current` | current | PZEM |
| `esp32/devkit/pzem/power` | power | PZEM |
| `esp32/devkit/pzem/energy` | energy kWh | PZEM |
| `esp32/devkit/io/relay1` | `ON` / `OFF` | DevRelay |
| `esp32/devkit/io/sw1` | `PRESSED` / `RELEASED` | DevSwitch |
| `esp32/devkit/io/iso1` | `ACTIVE` / `INACTIVE` | DevIsoInput |

## MQTT Retain Policy

| Topic Type | Retain |
|---|---|
| Availability/status | yes |
| Relay state | yes |
| Sensor values | optional |
| Fast-changing debug data | no |

## MQTT Command Topics

ถ้าต้องการให้ MQTT ควบคุม relay ได้ ให้ subscribe topic เหล่านี้

| Topic | Payload | Action |
|---|---|---|
| `esp32/devkit/cmd/relay1` | `ON` | เปิด relay1 |
| `esp32/devkit/cmd/relay1` | `OFF` | ปิด relay1 |
| `esp32/devkit/cmd/relay1` | `TOGGLE` | toggle relay1 |
| `esp32/devkit/cmd/pzem/reset_energy` | `RESET` | เรียก `pzem.resetEnergy()` |

## โครงสร้างโปรแกรมที่ AI ควรสร้าง

```text
setup()
  Serial.begin(9600)
  begin I/O classes
  begin DS18B20
  begin OLED ถ้าใช้
  connect Wi-Fi
  setup MQTT server/callback
  connect MQTT
  begin XY-MD03
  begin PZEM

loop()
  keep Wi-Fi connected
  keep MQTT connected
  mqtt.loop()
  update switch/isolated input
  publish I/O edge events
  read DS18B20 every 1s
  read XY-MD03 every 3s
  read PZEM every 3s
  publish sensor values
  update OLED status ถ้ามี
```

## ตัวอย่าง Config ที่ควรแยกไว้ด้านบนของ main.cpp

```cpp
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

const char* MQTT_HOST = "192.168.1.10";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASSWORD = "";

const char* DEVICE_ID = "esp32-devkit-v2";
```

## ตัวอย่าง Function สำหรับ Publish ค่า Float

```cpp
void publishFloat(const char* topic, float value, uint8_t decimals = 2, bool retained = false) {
  char payload[24];
  dtostrf(value, 0, decimals, payload);
  mqtt.publish(topic, payload, retained);
}
```

## ตัวอย่าง Callback สำหรับ MQTT Command

```cpp
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == "esp32/devkit/cmd/relay1") {
    if (message == "ON") {
      relay1.on();
    } else if (message == "OFF") {
      relay1.off();
    } else if (message == "TOGGLE") {
      relay1.toggle();
    }

    mqtt.publish("esp32/devkit/io/relay1", relay1.getState() ? "ON" : "OFF", true);
  }

  if (String(topic) == "esp32/devkit/cmd/pzem/reset_energy") {
    if (message == "RESET") {
      bool ok = pzem.resetEnergy();
      mqtt.publish("esp32/devkit/pzem/reset_energy/result", ok ? "OK" : "FAILED", false);
    }
  }
}
```

## ตัวอย่าง Logic ใน loop

```cpp
void loop() {
  ensureWiFi();
  ensureMqtt();
  mqtt.loop();

  sw1.update();
  iso1.update();

  if (sw1.wasPressed()) {
    mqtt.publish("esp32/devkit/io/sw1", "PRESSED", false);
  }

  if (sw1.wasReleased()) {
    mqtt.publish("esp32/devkit/io/sw1", "RELEASED", false);
  }

  if (iso1.wasActivated()) {
    mqtt.publish("esp32/devkit/io/iso1", "ACTIVE", true);
  }

  if (iso1.wasDeactivated()) {
    mqtt.publish("esp32/devkit/io/iso1", "INACTIVE", true);
  }

  if (millis() - lastDs18b20Publish >= 1000) {
    lastDs18b20Publish = millis();
    ds18b20.requestTemperatures();
    float tempC = ds18b20.getTempCByIndex(0);

    if (tempC != DEVICE_DISCONNECTED_C) {
      publishFloat("esp32/devkit/ds18b20/temperature", tempC, 2, false);
    }
  }

  if (millis() - lastModbusPublish >= 3000) {
    lastModbusPublish = millis();

    if (xymd.update()) {
      publishFloat("esp32/devkit/xymd/temperature", xymd.getTemperature(), 1, false);
      publishFloat("esp32/devkit/xymd/humidity", xymd.getHumidity(), 1, false);
    }

    if (pzem.update() && pzem.isDataValid()) {
      mqtt.publish("esp32/devkit/pzem/json", pzem.toJSON().c_str(), false);
      publishFloat("esp32/devkit/pzem/voltage", pzem.getVoltage(), 2, false);
      publishFloat("esp32/devkit/pzem/current", pzem.getCurrent(), 3, false);
      publishFloat("esp32/devkit/pzem/power", pzem.getPower(), 2, false);
      publishFloat("esp32/devkit/pzem/energy", pzem.getEnergy(), 3, true);
    }
  }
}
```

## ขั้นตอนทดลอง

1. เตรียม MQTT broker และจด host, port, username, password
2. สร้างโปรเจกต์ PlatformIO หรือใช้โปรเจกต์เดิม
3. เพิ่ม `knolleary/PubSubClient` ใน `platformio.ini`
4. สั่ง AI สร้าง `main.cpp` โดยใช้ prompt ด้านล่าง
5. ใส่ Wi-Fi และ MQTT config จริง
6. Upload ตอน switch อยู่โหมด RS232
7. สลับ switch เป็น RS485
8. Reset ESP32 ถ้าจำเป็น
9. เปิด MQTT client เพื่อตรวจ topic
10. ทดสอบ command topic เช่น relay ON/OFF

## ทดสอบด้วย Mosquitto

Subscribe:

```powershell
mosquitto_sub -h 192.168.1.10 -t "esp32/devkit/#" -v
```

Publish command:

```powershell
mosquitto_pub -h 192.168.1.10 -t "esp32/devkit/cmd/relay1" -m "TOGGLE"
```

Reset PZEM energy:

```powershell
mosquitto_pub -h 192.168.1.10 -t "esp32/devkit/cmd/pzem/reset_energy" -m "RESET"
```

## ข้อควรระวัง

- อย่า hardcode Wi-Fi/MQTT password ลง git public repository
- ถ้าใช้ UART0 กับ RS485 แล้ว Serial Monitor ไม่เห็นข้อมูล ให้ตรวจตำแหน่ง switch
- ถ้า MQTT reconnect บ่อย ให้ตรวจ Wi-Fi RSSI และ broker timeout
- อย่า publish ถี่เกินจำเป็น ควรใช้ `millis()` แยก interval
- ถ้า PZEM อยู่ simulation mode ให้ publish สถานะนี้ด้วย เพื่อไม่ให้ dashboard เข้าใจว่าเป็นค่าจริง
- คำสั่ง reset energy ต้องมี payload เฉพาะ เช่น `RESET` เพื่อป้องกันการลบค่าพลังงานโดยไม่ตั้งใจ

## Prompt สำหรับสั่ง AI

```text
ช่วยสร้างโปรแกรม ESP32 DevKit V2 ด้วย PlatformIO Arduino framework สำหรับเชื่อม Wi-Fi และ MQTT โดยอ้างอิง blueprint_v2026.md และ Lesson11.md นี้ ใช้ PubSubClient ส่งข้อมูล DS18B20 GPIO14, XY-MD03 ผ่าน Serial/UART0, PZEM-016 ผ่าน Serial/UART0, DevSwitch GPIO34, DevIsoInput GPIO33 และ DevRelay GPIO17 active low ห้ามย้าย RS485 ไป Serial2 ใช้ millis แยก interval ไม่ใช้ delay ยาว สร้าง topic ตามตาราง lesson นี้ และ subscribe คำสั่ง esp32/devkit/cmd/relay1 กับ esp32/devkit/cmd/pzem/reset_energy พร้อมตรวจ isDataValid, isSimulationMode และ DEVICE_DISCONNECTED_C ให้ถูกต้อง
```

## Prompt แบบให้ AI ทำทีละขั้น

```text
ขั้นที่ 1: ช่วยสร้างเฉพาะโครง Wi-Fi + MQTT reconnect ด้วย PubSubClient สำหรับ ESP32 PlatformIO monitor 9600 ยังไม่ต้องอ่าน sensor
```

```text
ขั้นที่ 2: เพิ่ม DevSwitch GPIO34, DevIsoInput GPIO33 และ DevRelay GPIO17 active low แล้ว publish event ไป MQTT เมื่อเกิด wasPressed, wasReleased, wasActivated, wasDeactivated
```

```text
ขั้นที่ 3: เพิ่ม DS18B20 GPIO14 ด้วย OneWire/DallasTemperature publish temperature ทุก 1 วินาที และตรวจ DEVICE_DISCONNECTED_C
```

```text
ขั้นที่ 4: เพิ่ม XY-MD03 ด้วย DevXYMDSensor ผ่าน Serial/UART0 อ่านทุก 3 วินาที publish temperature/humidity ห้ามใช้ Serial2
```

```text
ขั้นที่ 5: เพิ่ม PZEM-016 ด้วย DevPZEM ผ่าน Serial/UART0 อ่านทุก 3 วินาที publish pzem.toJSON และค่าแยก voltage/current/power/energy ตรวจ isDataValid และ isSimulationMode
```

```text
ขั้นที่ 6: เพิ่ม MQTT command callback สำหรับ relay ON/OFF/TOGGLE และ PZEM reset energy ด้วย payload RESET พร้อม publish result กลับ
```

## คำถามทบทวน

- ทำไม MQTT reconnect ต้องอยู่ใน `loop()`?
- topic แบบ JSON และ topic แบบแยกค่าเหมาะกับงานต่างกันอย่างไร?
- ทำไมต้องระวังคำสั่ง reset energy ผ่าน MQTT?
- ถ้า sensor ใช้ UART0/RS485 แล้ว upload ไม่ผ่าน ควรตรวจอะไร?

