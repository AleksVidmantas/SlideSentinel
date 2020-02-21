#ifndef _IMUCONTROLLER_H_
#define _IMUCONTROLLER_H_

#include <Arduino.h>
#include "Adafruit_MMA8451.h"
#include "ArduinoJson.h"
#include "Controller.h"

// State: m_sensitivity

class IMUController : public Controller {
private:
  Adafruit_MMA8451 m_dev;
  uint8_t m_sensitivity;
  static uint8_t m_pin;
  static volatile bool m_flag;
  const char* IMU_WAKE;
  bool m_getFlag();
  void m_setFlag();

public:
  IMUController(uint8_t pin, uint8_t sensitivity);
  bool init();
  void getWakeStatus(JsonDocument &doc);
  static void IMU_ISR(); 
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _IMUCONTROLLER_H_