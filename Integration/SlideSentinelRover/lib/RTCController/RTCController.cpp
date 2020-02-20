#include "RTCController.h"
#include "Console.h"

uint8_t RTCController::m_interruptPin;
volatile bool RTCController::m_flag = false;

RTCController::RTCController(RTC_DS3231 * RTC_DS, uint8_t intPin, uint8_t wakeTime, uint8_t sleepTime)
    : Controller("RTC")
    , m_RTC_DS(RTC_DS)
    , m_wakeTime(wakeTime)
    , m_sleepTime(sleepTime)
    {
        m_interruptPin = intPin;
    }

void RTCController::m_clearRTCAlarm(){
    // Clears pending alarms
    m_RTC_DS->armAlarm(1, false);
    m_RTC_DS->clearAlarm(1);
    m_RTC_DS->alarmInterrupt(1, false);
    m_RTC_DS->armAlarm(2, false);
    m_RTC_DS->clearAlarm(2);
    m_RTC_DS->alarmInterrupt(2, false);
}

void RTCController::m_RTC_ISR(){
    detachInterrupt(digitalPinToInterrupt(m_interruptPin));
    m_flag = true;
    attachInterrupt(digitalPinToInterrupt(m_interruptPin), m_RTC_ISR, FALLING);
}

void RTCController::setFlag(){
    m_flag = false;
}

bool RTCController::getFlag(){
    return m_flag;
}

void RTCController::init(){
    pinMode(m_interruptPin, INPUT_PULLUP);
    if (!m_RTC_DS->begin())
    {
        console.debug("Couldn't find RTC");
        while (1)
            ;
    }

    //clear any pending alarms
    m_clearRTCAlarm();

    //Set SQW pin to OFF (in my case it was set by default to 1Hz)
    //The output of the DS3231 INT pin is connected to this pin
    //It must be connected to arduino Interrupt pin for wake-up
    m_RTC_DS->writeSqwPinMode(DS3231_OFF);

    //Set alarm1
    m_clearRTCAlarm();
}

void RTCController::setRTCAlarm(){
    DateTime now = getDate();
    uint8_t m_MIN = (now.minute() + m_wakeTime) % 60;
    uint8_t m_HR = (now.hour() + ((now.minute() + m_wakeTime) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs

    console.debug("Setting Alarm 1 for: ");
    console.debugInt(m_HR);
    console.debug(":");
    console.debugInt(m_MIN);

    // Set alarm1S
    m_RTC_DS->setAlarm(ALM1_MATCH_HOURS, m_MIN, m_HR, 0); //set your wake-up time here
    m_RTC_DS->alarmInterrupt(1, true);                //code to pull microprocessor out of sleep is tied to the time -> here
    attachInterrupt(digitalPinToInterrupt(m_interruptPin), m_RTC_ISR, FALLING);
}

DateTime RTCController::getDate(){
    return m_RTC_DS->now();
}

void RTCController::update(JsonDocument &doc) {}

void RTCController::status(uint8_t verbosity, JsonDocument &doc) {}