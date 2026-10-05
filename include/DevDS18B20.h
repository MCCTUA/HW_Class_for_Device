#ifndef DEV_DS18B20_H
#define DEV_DS18B20_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * @class DevDS18B20
 * @brief อ่านอุณหภูมิจาก DS18B20 (1-Wire) แบบ non-blocking + simulation fallback อัตโนมัติ
 * @details ต้องมี pull-up 4.7 kΩ ระหว่างขา DATA กับ 3V3
 *          - อ่านได้ปกติ: isSimulated() == false
 *          - ไม่ต่อ sensor / อ่านผิดพลาดติดกัน FAIL_LIMIT ครั้ง: สลับเป็นค่าจำลอง (isSimulated() == true)
 *          - ยังคงลองอ่านของจริงต่อเนื่อง ถ้า sensor กลับมาจะสลับกลับเป็นค่าจริงเอง
 */
class DevDS18B20 {
public:
  static constexpr uint8_t HISTORY_SIZE = 60;

private:
  static constexpr uint8_t FAIL_LIMIT = 3;
  static constexpr unsigned long CONVERT_MS = 800;  // 12-bit ใช้ ~750 ms

  OneWire oneWire;
  DallasTemperature sensors;
  unsigned long intervalMs;
  unsigned long historyIntervalMs;
  unsigned long lastRequest = 0;
  unsigned long lastHistory = 0;
  bool converting = false;
  uint8_t failCount = 0;
  bool simulated = true;  // เริ่มต้นเป็น sim จนกว่าจะอ่านของจริงได้
  bool hasValue = false;
  float temp = 0;
  float minTemp = 0;
  float maxTemp = 0;
  float simTemp = 27.0;
  float history[HISTORY_SIZE];
  uint8_t histCount = 0;
  uint8_t histHead = 0;  // ตำแหน่งที่จะเขียนถัดไป

  // ค่าจำลอง: sine ช้า ๆ รอบ 28 °C (±3) + random walk เล็กน้อย
  float nextSimulated() {
    float base = 28.0 + 3.0 * sin(millis() / 60000.0 * TWO_PI);
    simTemp += (base - simTemp) * 0.3 + ((int)random(-10, 11)) / 100.0;
    return simTemp;
  }

  void store(float t) {
    temp = t;
    if (!hasValue) {
      minTemp = maxTemp = t;
      hasValue = true;
    } else {
      if (t < minTemp) minTemp = t;
      if (t > maxTemp) maxTemp = t;
    }
    if (millis() - lastHistory >= historyIntervalMs || histCount == 0) {
      lastHistory = millis();
      history[histHead] = t;
      histHead = (histHead + 1) % HISTORY_SIZE;
      if (histCount < HISTORY_SIZE) histCount++;
    }
  }

  void onReadFail() {
    if (failCount < 255) failCount++;
    if (failCount >= FAIL_LIMIT) {
      simulated = true;
      store(nextSimulated());
    }
  }

public:
  /**
   * @param pin ขา DATA ของ DS18B20
   * @param intervalMs ระยะเวลาระหว่างการอ่านแต่ละครั้ง
   * @param historyIntervalMs ระยะเวลาระหว่างจุดของกราฟประวัติ
   */
  DevDS18B20(uint8_t pin, unsigned long intervalMs = 2000, unsigned long historyIntervalMs = 5000)
    : oneWire(pin), sensors(&oneWire), intervalMs(intervalMs), historyIntervalMs(historyIntervalMs) {}

  void begin() {
    sensors.begin();
    sensors.setResolution(12);
    sensors.setWaitForConversion(false);  // ห้ามบล็อก loop
    lastRequest = millis() - intervalMs;   // อ่านรอบแรกทันที
  }

  /** เรียกใน loop() */
  void update() {
    unsigned long now = millis();
    if (!converting) {
      if (now - lastRequest < intervalMs) return;
      lastRequest = now;
      if (sensors.getDeviceCount() == 0) sensors.begin();  // ลอง detect ใหม่ (รองรับเสียบทีหลัง)
      if (sensors.getDeviceCount() == 0) {
        onReadFail();
        return;
      }
      sensors.requestTemperatures();
      converting = true;
      return;
    }
    if (now - lastRequest < CONVERT_MS) return;
    converting = false;
    float t = sensors.getTempCByIndex(0);
    // DEVICE_DISCONNECTED_C = -127, 85.0 = ค่า power-on reset (ยังไม่ได้แปลงค่า)
    if (t == DEVICE_DISCONNECTED_C || t == 85.0f || t < -55 || t > 125) {
      onReadFail();
      return;
    }
    failCount = 0;
    if (simulated) {  // เพิ่งกลับมาใช้ค่าจริง: เริ่ม min/max และกราฟใหม่
      simulated = false;
      hasValue = false;
      histCount = histHead = 0;
    }
    store(t);
  }

  bool isSimulated() const { return simulated; }
  bool hasReading() const { return hasValue; }
  float getTemp() const { return temp; }
  float getMin() const { return minTemp; }
  float getMax() const { return maxTemp; }
  uint8_t getHistoryCount() const { return histCount; }

  /** ดึงประวัติเรียงจากเก่า -> ใหม่ (i = 0 คือเก่าสุด) */
  float getHistory(uint8_t i) const {
    uint8_t start = (histHead + HISTORY_SIZE - histCount) % HISTORY_SIZE;
    return history[(start + i) % HISTORY_SIZE];
  }
};

#endif // DEV_DS18B20_H
