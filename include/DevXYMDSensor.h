#ifndef DEV_XYMD_SENSOR_H
#define DEV_XYMD_SENSOR_H

#include <Arduino.h>
#include <ModbusMaster.h>

/**
 * @class DevXYMDSensor
 * @brief คลาสสำหรับจัดการเซนเซอร์อุณหภูมิและความชื้น XY-MD03 ผ่าน Modbus RTU
 * @details รองรับการอ่านค่าอุณหภูมิและความชื้นจาก XY-MD03 Sensor
 *          รองรับ Auto Direction RS485 (ไม่ต้องใช้ DE/RE pin)
 * 
 * Default Settings:
 * - Slave ID: 1 (0x01)
 * - Baud Rate: 9600
 * - Data bits: 8, Stop bit: 1, Parity: None
 * 
 * Modbus Registers:
 * - Temperature: Register 0x0001 (หารด้วย 10)
 * - Humidity: Register 0x0002 (หารด้วย 10)
 * - Function Code: 04 (Read Input Register)
 */
class DevXYMDSensor {
private:
  ModbusMaster modbus;           // Modbus Master object
  uint8_t slaveID;               // Modbus Slave ID
  HardwareSerial* serial;        // Serial port สำหรับ Modbus
  
  float temperature;             // ค่าอุณหภูมิล่าสุด (°C)
  float humidity;                // ค่าความชื้นล่าสุด (%)
  bool lastReadSuccess;          // สถานะการอ่านค่าครั้งล่าสุด
  unsigned long lastReadTime;    // เวลาที่อ่านค่าครั้งล่าสุด

  // Simulation fallback: ไม่ต่อ sensor / อ่านผิดพลาดติดกัน FAIL_LIMIT ครั้ง -> ใช้ค่าจำลอง
  // และยังลองอ่านของจริงต่อ (ช้าลงเป็น SIM_RETRY_MS) ถ้า sensor กลับมาจะสลับเป็นค่าจริงเอง
  static const uint8_t FAIL_LIMIT = 3;
  static const unsigned long SIM_RETRY_MS = 15000;  // read timeout ของ Modbus ~2 s จึงลองถี่ไม่ได้
  unsigned long intervalMs = 2000;
  unsigned long lastPoll = 0;
  uint8_t failCount = 0;
  bool simulated = true;
  bool hasValue = false;
  float simTemp = 27.0;
  float simHum = 60.0;

  // ค่าจำลอง: sine ช้า ๆ + random walk เล็กน้อย (ความชื้นสวนทางกับอุณหภูมิ)
  void nextSimulated() {
    float phase = sin(millis() / 60000.0 * TWO_PI);
    simTemp += ((28.0 + 3.0 * phase) - simTemp) * 0.3 + ((int)random(-10, 11)) / 100.0;
    simHum += ((60.0 - 8.0 * phase) - simHum) * 0.3 + ((int)random(-20, 21)) / 100.0;
    temperature = simTemp;
    humidity = simHum;
    hasValue = true;
  }
  
  // Modbus Register Addresses
  static const uint16_t REG_TEMPERATURE = 0x0001;
  static const uint16_t REG_HUMIDITY = 0x0002;

public:
  /**
   * @brief Constructor
   * @param hwSerial ตัวชี้ไปที่ HardwareSerial object (เช่น &Serial สำหรับใช้กับ USB Serial)
   * @param slaveId Modbus Slave ID (default: 1)
   */
  DevXYMDSensor(HardwareSerial* hwSerial, uint8_t slaveId = 1) {
    serial = hwSerial;
    slaveID = slaveId;
    temperature = 0.0;
    humidity = 0.0;
    lastReadSuccess = false;
    lastReadTime = 0;
  }

  /**
   * @brief เริ่มต้นการทำงาน (ใช้กับ Auto Direction RS485)
   * @param baudRate ความเร็ว Baud Rate (default: 9600)
   * @note เหมาะสำหรับใช้กับ Serial พอร์ตหลักที่มี auto direction
   */
  void begin(unsigned long baudRate = 9600) {
    // เริ่มต้น Serial (สำหรับ auto direction ไม่ต้องตั้งค่า DE/RE)
    serial->begin(baudRate, SERIAL_8N1);
    
    // เริ่มต้น Modbus
    modbus.begin(slaveID, *serial);
    lastPoll = millis() - intervalMs;  // อ่านรอบแรกทันที
  }

  /**
   * @brief เรียกใน loop() - อ่านค่าตามรอบเวลา, สลับเป็นค่าจำลองอัตโนมัติถ้าอ่านไม่ได้
   * @note ห้ามพิมพ์ลง Serial0 ระหว่างใช้งาน เพราะ Serial0 คือบัส RS485
   * @return true ถ้ามีค่าใหม่ในรอบนี้ (จริงหรือจำลอง)
   */
  bool update() {
    unsigned long wait = simulated ? SIM_RETRY_MS : intervalMs;
    if (millis() - lastPoll < wait) {
      // ช่วงรอลองของจริงใหม่: ยังคงอัปเดตค่าจำลองตาม intervalMs
      if (simulated && millis() - lastReadTime >= intervalMs) {
        lastReadTime = millis();
        nextSimulated();
        return true;
      }
      return false;
    }
    lastPoll = millis();

    uint8_t result = modbus.readInputRegisters(REG_TEMPERATURE, 2);  // FC 04: Temp, Hum
    if (result == modbus.ku8MBSuccess) {
      if (simulated) {
        simulated = false;
        hasValue = false;
      }
      failCount = 0;
      temperature = modbus.getResponseBuffer(0) / 10.0;
      humidity = modbus.getResponseBuffer(1) / 10.0;
      hasValue = true;
      lastReadSuccess = true;
      lastReadTime = millis();
      return true;
    }

    lastReadSuccess = false;
    if (failCount < 255) failCount++;
    if (failCount >= FAIL_LIMIT) {
      simulated = true;
      lastReadTime = millis();
      nextSimulated();
      return true;
    }
    return false;
  }

  /** true = ค่าที่ได้เป็นค่าจำลอง (ไม่พบ sensor) */
  bool isSimulated() const { return simulated; }

  /** true = มีค่าให้แสดงแล้ว (ค่าจริงหรือจำลอง) */
  bool hasReading() const { return hasValue; }

  /**
   * @brief ดึงค่าอุณหภูมิล่าสุด
   * @return ค่าอุณหภูมิในหน่วย °C
   */
  float getTemperature() const {
    return temperature;
  }

  /**
   * @brief ดึงค่าความชื้นล่าสุด
   * @return ค่าความชื้นในหน่วย %
   */
  float getHumidity() const {
    return humidity;
  }

  /**
   * @brief ตรวจสอบว่าการอ่านค่าครั้งล่าสุดสำเร็จหรือไม่
   * @return true ถ้าอ่านสำเร็จ, false ถ้าล้มเหลว
   */
  bool isLastReadSuccess() const {
    return lastReadSuccess;
  }

  /**
   * @brief ดึงเวลาที่อ่านค่าครั้งล่าสุด
   * @return เวลาใน milliseconds
   */
  unsigned long getLastReadTime() const {
    return lastReadTime;
  }

  /**
   * @brief แสดงข้อมูลทั้งหมดทาง Serial (ใช้ได้เฉพาะตอนไม่ได้ใช้ Serial0 เป็นบัส RS485)
   */
  void printInfo() const {
    Serial.println("=== XY-MD03 Temperature & Humidity Sensor ===");
    Serial.printf("Temperature: %.1f °C\n", temperature);
    Serial.printf("Humidity: %.1f %%\n", humidity);
    Serial.printf("Last Read: %s (%lu ms ago)\n", 
                  lastReadSuccess ? "Success" : "Failed",
                  millis() - lastReadTime);
    Serial.println("=============================================");
  }

  /**
   * @brief เปลี่ยน Slave ID
   * @param newSlaveId Slave ID ใหม่
   */
  void setSlaveID(uint8_t newSlaveId) {
    slaveID = newSlaveId;
    modbus.begin(slaveID, *serial);
  }

  /**
   * @brief ดึง Slave ID ปัจจุบัน
   * @return Slave ID
   */
  uint8_t getSlaveID() const {
    return slaveID;
  }
};

#endif // DEV_XYMD_SENSOR_H
