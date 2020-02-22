#include "Battery.h"
#include "ComController.h"
#include "Controller.h"
#include "FSController.h"
#include "FreewaveRadio.h"
#include "GNSSController.h"
#include "IMUController.h"
#include "MAX3243.h"
#include "MAX4280.h"
#include "PMController.h"
#include "RTClibExtended.h"
#include "SN74LVC2G53.h"
#include "VoltageReg.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// TODO move state variables to centralized class State, inject the state into
// all controllers for dynamic behavior changes.
// TODO JSON compression object, we can use the object to compress verbose json
// identifiers and decompress them
// TODO centrailze "MSG" headers and reference this in all all files for ensured
// consistency
/****** Test Routine ******/
#define ADVANCED true

/****** ComController ******/
#define RADIO_BAUD 115200
#define CLIENT_ADDR 1
#define SERVER_ADDR 2
#define RST 6
#define CD 10
#define IS_Z9C true
#define SPDT_SEL 14
#define FORCEOFF_N A5

/****** PMController ******/
#define VCC2_EN 13
#define MAX_CS 9
#define BAT 15

/****** FSController ******/
#define SD_CS 18  // A4
#define SD_RST 16 // A2

/****** IMUController ******/
#define ACCEL_INT A3

/****** GNSSController ******/
#define GNSS_TX 11
#define GNSS_RX 12
#define GNSS_BAUD 115200

/****** Mail ******/
StaticJsonDocument<1000> doc;

/****** FSController Init ******/
FSController fsController(SD_CS, SD_RST);

/****** ComController Init ******/
Freewave radio(RST, CD, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
MAX3243 max3243(FORCEOFF_N);
ComController *comController;

/****** PMController Init ******/
PoluluVoltageReg vcc2(VCC2_EN); // TODO rename this class to the id of the
                                // voltage regulator from the manufacturer
MAX4280 max4280(MAX_CS, &SPI);
Battery batReader(BAT);
PMController pmController(&max4280, &vcc2, &batReader, false, true);

/****** IMUController Init ******/
IMUController imuController(ACCEL_INT, 0x1F);

/****** GNSSController Init ******/
Uart Serial2(&sercom1, GNSS_RX, GNSS_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler() { Serial2.IrqHandler(); }
GNSSController *gnssController;

// Instatiate RTC Object
#define RTC_INT 5
#define RTC_WAKE_PERIOD 1
RTC_DS3231 RTC_DS;
volatile int HR = 8;
volatile int MIN = 0;
volatile int awakeFor = 20;

void clearRTCAlarm() {
  // clear any pending alarms
  RTC_DS.armAlarm(1, false);
  RTC_DS.clearAlarm(1);
  RTC_DS.alarmInterrupt(1, false);
  RTC_DS.armAlarm(2, false);
  RTC_DS.clearAlarm(2);
  RTC_DS.alarmInterrupt(2, false);
}

void initializeRTC() {
  Wire.begin(); // called in Adafruit_MMA8451::begin()
  RTC_DS.begin();
  RTC_DS.adjust(DateTime(__DATE__, __TIME__));

  // clear any pending alarms
  clearRTCAlarm();

  // Set SQW pin to OFF (in my case it was set by default to 1Hz)
  // The output of the DS3231 INT pin is connected to this pin
  // It must be connected to arduino Interrupt pin for wake-up
  RTC_DS.writeSqwPinMode(DS3231_OFF);

  // Set alarm1
  clearRTCAlarm();
}

void rtcInt() {
  detachInterrupt(digitalPinToInterrupt(RTC_INT));
  Serial.println("RTC Wake");
  attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

void setRTCAlarm() {
  DateTime now = RTC_DS.now(); // Check the current time
  MIN = (now.minute() + RTC_WAKE_PERIOD) %
        60; // wrap-around using modulo every 60 sec
  HR = (now.hour() + ((now.minute() + RTC_WAKE_PERIOD) / 60)) %
       24; // quotient of now.min+periodMin added to now.hr, wraparound every
           // 24hrs

  Serial.print("Setting Alarm 1 for: ");
  Serial.print(HR);
  Serial.print(":");
  Serial.println(MIN);

  // Set alarm1
  RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0); // set your wake-up time here
  RTC_DS.alarmInterrupt(1, true); // code to pull microprocessor out of sleep is
                                  // tied to the time -> here
  attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

bool pollFlag = false;
void advancedTest() {
  char cmd;
  char test[] = "{\"sensor\":\"gps\",\"time\":1351824120}";

  char fileName[30];
  memset(fileName, '\0', sizeof(char) * 30);

  if (Serial.available()) {
    DateTime now = RTC_DS.now();
    cmd = Serial.read();
    switch (cmd) {
    case '1':
      pmController.enableGNSS();
      break;
    case '2':
      pmController.disableGNSS();
      break;
    case '3':
      pmController.enableRadio();
      break;
    case '4':
      pmController.disableRadio();
      break;
    case '5':
      Serial1.println(
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST "
          "TEST TEST TEST TEST TEST TEST TEST TEST TEST");
      Serial.println("Writing data out...");
      break;
    case '6':
      Serial.println("RADIO -----> FEATHER M0 (A0 LOW)");
      mux.comY1();
      break;
    case '7':
      Serial.println("RADIO -----> GNSS RECEIVER (A0 HIGH)");
      mux.comY2();
      break;
    case '8':
      Serial.println("RS232 ---> OFF (Driving it LOW)");
      max3243.disable();
      break;
    case '9':
      Serial.println("RS232 ---> ON (Driving it HIGH)");
      max3243.enable();
      break;
    case 'q':
      Serial.println("Resetting the radio, DRIVING RST LOW");
      comController->resetRadio();
      break;
    case 'w':
      Serial.print("STATE of CD pin: ");
      if (comController->channelBusy())
        Serial.println("BUSY");
      else
        Serial.println("NOT BUSY");
      break;
    case 'e':
      setRTCAlarm();
      break;
    case 'r':
      Serial.print("RTC Time is: ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.println(now.second(), DEC);
      break;
    case 't':
      Serial.println("Sleeping");
      pmController.disableGNSS();
      pmController.disableRadio();
      pmController.sleep();

      Serial.begin(115200);
      pmController.enableGNSS();
      delay(2000);
      pmController.disableGNSS();
      Serial.println("Awake");
      break;
    case 'y':
      Serial.print("reading battery voltage: ");
      Serial.println(pmController.readBat());
      break;
    case 'u':
      Serial.print("reading battery voltage string: ");
      char volt[20];
      memset(volt, '\0', sizeof(char) * 20);
      pmController.readBatStr(volt);
      Serial.println(volt);
      break;
    case 'i':
      Serial.println("Turning off VCC2");
      vcc2.disable();
      break;
    case 'o':
      Serial.println("Turning on VCC2");
      vcc2.enable();
      break;
    case 'p':
      char buffer[RH_SERIAL_MAX_MESSAGE_LEN];
      memset(buffer, '\0', sizeof(char) * RH_SERIAL_MAX_MESSAGE_LEN);
      Serial.println("Turning BASE station ON");
      comController->request(doc);
      serializeJson(doc, buffer);
      Serial.print("Config Received:    ");
      Serial.println(buffer);
      pmController.enableGNSS();
      delay(500);
      break;

    case 's':
      Serial.println("Creating new directory");
      sprintf(fileName, "%.2d.%.2d.%.2d.%.2d.%.2d", now.month(), now.day(),
              now.hour(), now.minute(), now.second());
      Serial.println(fileName);
      fsController.setupWakeCycle(fileName, gnssController->getFormat());
      break;

    case 'd':
      Serial.println("Attempting to send with no connection");
      comController->request(doc);
      serializeJsonPretty(doc, Serial);
      fsController.log(doc);
      break;

    case 'a':
      Serial.println("Turning BASE station OFF");
      deserializeJson(doc, test);
      comController->upload(doc);
      pmController.disableGNSS();
      delay(500);
      break;
    case 'f':
      if (!pollFlag) {
        Serial.println("Polling...");
        pollFlag = true;
      } else {
        Serial.println("Done Polling...");
        pollFlag = false;
      }
      break;
    }
  }

  if (Serial1.available()) {
    Serial.print(Serial1.read());
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  // Place instatiation here, Serial1 is not in the same compilation unit as
  static ComController _comController(&radio, &max3243, &mux, &Serial1,
                                      RADIO_BAUD, CLIENT_ADDR, SERVER_ADDR);
  comController = &_comController;

  // TODO Serial2 initialization occurs after instantiation, we do not need to
  // make this static like the ComController
  static GNSSController _gnssController(&Serial2, GNSS_BAUD, GNSS_RX, GNSS_TX);
  gnssController = &_gnssController;

  // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Init IMUController
  imuController.init();

  // RTC INIT
  pinMode(RTC_INT, INPUT_PULLUP); // active low interrupts
  initializeRTC();

  // must be done after first call to attac1hInterrupt()
  pmController.init();
  if (fsController.init())
    Serial.println("successfully initialized SD");
}

void loop() {

  while (1) {

    if (ADVANCED)
      advancedTest();

    if (pollFlag && gnssController->poll(doc)) {
      serializeJsonPretty(doc, Serial);
      // TODO only log if we have valid data, in case data stays in the buffer
      // at sleep?
      fsController.log(doc);
    }

    // if (imuController.getFlag()) {
    //   Serial.println("accel int");
    //   imuController.setFlag();
    // }
  }
}
