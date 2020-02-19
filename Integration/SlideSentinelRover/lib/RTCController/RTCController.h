#ifndef _RTCCONTROLLER_H_
#define _RTCCONTROLLER_H_

#include <Arduino.h>
#include "Controller.h"
#include "RTClibExtended.h"
#include "ArduinoJson.h"

class RTCController : public Controller {
    private:
        RTC_DS3231 *m_RTC_DS;
        static uint8_t m_interruptPin;
        static volatile bool m_flag; 
        uint8_t m_wakeTime;
        uint8_t m_sleepTime;

        void m_initializeRTC();
        void m_clearRTCAlarm();
        static void m_RTC_ISR();

    public:
        RTCController(RTC_DS3231 *RTC_DS, uint8_t intPin, uint8_t wakeTime, uint8_t sleepTime); 
        void update(JsonDocument &doc); // runtime polymorphism
        void status(uint8_t verbosity, JsonDocument &doc);
        void setRTCAlarm();
        void setFlag();
        bool getFlag();
        void init();
        DateTime getDate();
};

#endif // Timing Controller