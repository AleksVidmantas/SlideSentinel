#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "sbp.h"
#include "navigation.h"
#include "Adafruit_MMA8451.h"
#include "RTClibExtended.h"
#include "wiring_private.h" // Pin peripheral
#include "MAX4280.h"
#include "MAX3243.h"
#include "FreewaveRadio.h"
#include "VoltageReg.h"
#include "SN74LVC2G53.h"
#include "PMController.h"
#include "ComController.h"
#include "Battery.h"
#include "Tutorial.h"


// Test Toggle
#define ADVANCED true

// COMMUNICATION CONTROLLER
#define RADIO_BAUD 115200
#define CLIENT_ADDR 1
#define SERVER_ADDR 2
#define RST 6
#define CD 10
#define IS_Z9C true
#define SPDT_SEL 14
#define FORCEOFF_N A5

/*
 * State of the SBP message parser.
 * Must be statically allocated.
 */
sbp_state_t sbp_state;

/* SBP structs that messages from Piksi will feed. */
msg_pos_llh_t pos_llh;
msg_baseline_ned_t baseline_ned;
msg_vel_ned_t vel_ned;
msg_dops_t dops;
msg_gps_time_t gps_time;

/*
 * SBP callback nodes must be statically allocated. Each message ID / callback
 * pair must have a unique sbp_msg_callbacks_node_t associated with it.
 */
sbp_msg_callbacks_node_t pos_llh_node;
sbp_msg_callbacks_node_t baseline_ned_node;
sbp_msg_callbacks_node_t vel_ned_node;
sbp_msg_callbacks_node_t dops_node;
sbp_msg_callbacks_node_t gps_time_node;

/*
 * Callback functions to interpret SBP messages.
 * Every message ID has a callback associated with it to
 * receive and interpret the message payload.
 */
void sbp_pos_llh_callback(u16 sender_id, u8 len, u8 msg[], void *context)
{
  pos_llh = *(msg_pos_llh_t *)msg;
}
void sbp_baseline_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context)
{
  baseline_ned = *(msg_baseline_ned_t *)msg;
}
void sbp_vel_ned_callback(u16 sender_id, u8 len, u8 msg[], void *context)
{
  vel_ned = *(msg_vel_ned_t *)msg;
}
void sbp_dops_callback(u16 sender_id, u8 len, u8 msg[], void *context)
{
  dops = *(msg_dops_t *)msg;
}
void sbp_gps_time_callback(u16 sender_id, u8 len, u8 msg[], void *context)
{
  gps_time = *(msg_gps_time_t *)msg;
}

/*
 * Set up SwiftNav Binary Protocol (SBP) nodes; the sbp_process function will
 * search through these to find the callback for a particular message ID.
 *
 * Example: sbp_pos_llh_callback is registered with sbp_state, and is associated
 * with both a unique sbp_msg_callbacks_node_t and the message ID SBP_POS_LLH.
 * When a valid SBP message with the ID SBP_POS_LLH comes through the UART, written
 * to the FIFO, and then parsed by sbp_process, sbp_pos_llh_callback is called
 * with the data carried by that message.
 */
void sbp_setup(void)
{
  /* SBP parser state must be initialized before sbp_process is called. */
  sbp_state_init(&sbp_state);

  /* Register a node and callback, and associate them with a specific message ID. */
  sbp_register_callback(&sbp_state, SBP_MSG_GPS_TIME, &sbp_gps_time_callback,
                        NULL, &gps_time_node);
  sbp_register_callback(&sbp_state, SBP_MSG_POS_LLH, &sbp_pos_llh_callback,
                        NULL, &pos_llh_node);
  sbp_register_callback(&sbp_state, SBP_MSG_BASELINE_NED, &sbp_baseline_ned_callback,
                        NULL, &baseline_ned_node);
  sbp_register_callback(&sbp_state, SBP_MSG_VEL_NED, &sbp_vel_ned_callback,
                        NULL, &vel_ned_node);
  sbp_register_callback(&sbp_state, SBP_MSG_DOPS, &sbp_dops_callback,
                        NULL, &dops_node);
}

Freewave radio(RST, CD, IS_Z9C);
SN74LVC2G53 mux(SPDT_SEL, -1);
MAX3243 max3243(FORCEOFF_N);
ComController *comController;

// Make sure you change the com level to TTL on the receiver!!!
// POWER MANAGEMENT CONTROLLER
#define SD_CS 18
#define VCC2_EN 13
#define MAX_CS 9
#define BAT 15

PoluluVoltageReg vcc2(VCC2_EN);
MAX4280 max4280(MAX_CS, &SPI);
Battery batReader(BAT);
PMController pmController(&max4280, &vcc2, &batReader, false, true);

// GLOBAL DOCUMENT
StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> doc;

// Instatiate ACCELEROMETER Object
#define ACCEL_INT A3
Adafruit_MMA8451 mma;

// Instatiate RTC Object
#define RTC_INT 5
RTC_DS3231 RTC_DS;
volatile int HR = 8; //  These should not be volatile
volatile int MIN = 0;
volatile int awakeFor = 20;
#define RTC_WAKE_PERIOD 1 // Interval to wake and take sample in Min, reset alarm based on this period (Bo - 5 min), 15 min

// GNSS Serial Init
#define SERIAL2_TX 11
#define SERIAL2_RX 12
#define GNSS_BAUD 115200
Uart Serial2(&sercom1, SERIAL2_RX, SERIAL2_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);

void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

void Serial2Setup(int baudrate)
{
    Serial2.begin(baudrate);
    // Assign pins 11,12 SERCOM functionality, internal function
    pinPeripheral(SERIAL2_TX, PIO_SERCOM); //Private functions for serial communication
    pinPeripheral(SERIAL2_RX, PIO_SERCOM);
}

void mmaSetupSlideSentinel()
{
    if (!mma.begin())
    {
        Serial.println("Unable to find MMA8451");
        while (1)
            ;
    }

    mma.setRange(MMA8451_RANGE_2_G);
    mma.setDataRate(MMA8451_DATARATE_6_25HZ);
}

void configInterrupts(Adafruit_MMA8451 device)
{
    uint8_t dataToWrite = 0;
    // MMA8451_REG_CTRL_REG2
    // sysatem control register 2

    //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
    //dataToWrite |= 0x40;    // FIFO interrupt
    //dataToWrite |= 0x20;    // Transient interrupt - enabled
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse interrupt
    //dataToWrite |= 0x04;    // Freefall interrupt
    //dataToWrite |= 0x01;    // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO

    // MMA8451_REG_CTRL_REG3
    // Interrupt control register
    dataToWrite |= 0x80; // FIFO gate option for wake/sleep transition, default 0, Asserting this allows the accelerometer to collect data the moment an impluse happens and preserve that data because the FIFO buffer is blocked. Thus at the end of a wake cycle the data from the initial transient wake up is still in the buffer
    dataToWrite |= 0x40; // Wake from transient interrupt enable
    //dataToWrite |= 0x20;    // Wake from orientation interrupt enable
    //dataToWrite |= 0x10;    // Wake from Pulse function enable
    //dataToWrite |= 0x08;    // Wake from freefall/motion decect interrupt
    //dataToWrite |= 0x02;    // Interrupt polarity, 1 = active high
    dataToWrite |= 0x00; // (0) Push/pull or (1) open drain interrupt, determines whether bus is driven by device, or left to hang

    device.writeRegister8_public(MMA8451_REG_CTRL_REG3, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_CTRL_REG4
    // Interrupt enable register, enables interrupts that are not commented

    //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
    //dataToWrite |= 0x40;    // FIFO interrupt
    dataToWrite |= 0x20; // Transient interrupt - enabled
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse interrupt
    //dataToWrite |= 0x04;    // Freefall interrupt
    dataToWrite |= 0x01; // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO
    device.writeRegister8_public(MMA8451_REG_CTRL_REG4, dataToWrite | 0x01);

    dataToWrite = 0;

    // MMA8451_REG_CTRL_REG5
    // Interrupt pin 1/2 configuration register, bit == 1 => interrupt to pin 1
    // see datasheet for interrupt's description, threshold int routed to pin 1
    // comment = int2, uncoment = int1

    //dataToWrite |= 0x80;    // Auto sleep/wake
    //dataToWrite |= 0x40;    // FIFO
    dataToWrite |= 0x20; // Transient, asserting this routes transients interrupts to INT1 pin
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse
    //dataToWrite |= 0x04;    // Freefall
    //dataToWrite |= 0x01;    // data ready

    device.writeRegister8_public(MMA8451_REG_CTRL_REG5, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_CFG
    //dataToWrite |= 0x10;  // Latch enable to capture accel values when interrupt occurs
    dataToWrite |= 0x08; // Z transient interrupt enable
    dataToWrite |= 0x04; // Y transient interrupt enable
    dataToWrite |= 0x02; // X transient interrupt enable
    //dataToWrite |= 0x01;    // High-pass filter bypass
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_CFG, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_THS
    // Transient interrupt threshold in units of .06g
    //Acceptable range is 1-127
    dataToWrite = 0x1F;
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_CT  0x20
    dataToWrite = 0; // value is 0-255 for numer of counts to debounce for, depends on ODR
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, dataToWrite);

    dataToWrite = 0;
}

void accelInt()
{
    detachInterrupt(digitalPinToInterrupt(ACCEL_INT));
    Serial.println("Accelerometer Wake");
    attachInterrupt(digitalPinToInterrupt(ACCEL_INT), accelInt, CHANGE);
}

void clearRTCAlarm()
{
    //clear any pending alarms
    RTC_DS.armAlarm(1, false);
    RTC_DS.clearAlarm(1);
    RTC_DS.alarmInterrupt(1, false);
    RTC_DS.armAlarm(2, false);
    RTC_DS.clearAlarm(2);
    RTC_DS.alarmInterrupt(2, false);
}

uint8_t getFixMode(msg_pos_llh_t pos_llh, char rj[])
{
    uint8_t mode = pos_llh.flags & 0b0000111;
    switch(mode)
    {
        case 0:
            sprintf(rj, "Invalid");
            break;
        case 1:
            sprintf(rj, "SPP");
            break;
        case 2:
            sprintf(rj, "DGNSS");
            break;
        case 3:
            sprintf(rj, "Float RTK");
            break;
        case 4:
            sprintf(rj, "Fixed RTK");
            break;
        case 5:
            sprintf(rj, "Dead Reckoning");
            break;
        case 6:
            sprintf(rj, "SBAS");
            break;
    }
    return mode;
}

void readGNSS()
{
    if (Serial2.available())
    {
        Serial.print((char)Serial2.read());
    }
}

bool writeData(char *file, char *data)
{
    auto sdFile = SD.open(file, FILE_WRITE);
    if (!sdFile)
        return false;

    sdFile.print(data);
    sdFile.close();
    return true;
}

void initializeRTC()
{
    if (!RTC_DS.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }
    //clear any pending alarms
    clearRTCAlarm();

    //Set SQW pin to OFF (in my case it was set by default to 1Hz)
    //The output of the DS3231 INT pin is connected to this pin
    //It must be connected to arduino Interrupt pin for wake-up
    RTC_DS.writeSqwPinMode(DS3231_OFF);

    //Set alarm1
    clearRTCAlarm();
}

void rtcInt()
{
    detachInterrupt(digitalPinToInterrupt(RTC_INT));
    Serial.println("RTC Wake");
    attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

void setRTCAlarm()
{
    DateTime now = RTC_DS.now();                                      // Check the current time
    MIN = (now.minute() + RTC_WAKE_PERIOD) % 60;                      // wrap-around using modulo every 60 sec
    HR = (now.hour() + ((now.minute() + RTC_WAKE_PERIOD) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs

    Serial.print("Setting Alarm 1 for: ");
    Serial.print(HR);
    Serial.print(":");
    Serial.println(MIN);

    // Set alarm1
    RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0); //set your wake-up time here
    RTC_DS.alarmInterrupt(1, true);                //code to pull microprocessor out of sleep is tied to the time -> here
    attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

bool setup_sd()
{
    Serial.println("Initializing SD card...");

    if (!SD.begin(SD_CS))
    {
        Serial.println("SD Initialization failed!");
        return false;
    }
    Serial.println("SD initialization complete");
    return true;
}

void advancedTest()
{
    char cmd;
    if (Serial.available())
    {
        DateTime now = RTC_DS.now();
        cmd = Serial.read();
        switch (cmd)
        {
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
            Serial1.println("TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST");
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
        case 'r':
            Serial.println("Resetting the radio, DRIVING RST LOW");
            comController->resetRadio();
            break;
        case 't':
            Serial.print("STATE of CD pin: ");
            if (comController->channelBusy())
                Serial.println("BUSY");
            else
                Serial.println("NOT BUSY");
            break;
        case 'w':
            setRTCAlarm();
            break;
        case 'j':
            Serial.print("RTC Time is: ");
            Serial.print(now.hour(), DEC);
            Serial.print(':');
            Serial.print(now.minute(), DEC);
            Serial.print(':');
            Serial.println(now.second(), DEC);
            break;
        case 's':
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
        case 'd':
            Serial.print("reading battery voltage: ");
            Serial.println(pmController.readBat());
            break;
        case 'f':
            Serial.print("reading battery voltage string: ");
            char volt[20];
            memset(volt, '\0', sizeof(char) * 20);
            pmController.readBatStr(volt);
            Serial.println(volt);
            break;
        case 'x':
            Serial.println("Turning off VCC2");
            vcc2.disable();
            break;
        case 'y':
            Serial.println("Turning on VCC2");
            vcc2.enable();
            break;
        case 'o':
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

        case 'p':
            Serial.println("Turning BASE station OFF");
            char test[] = "{\"sensor\":\"gps\",\"time\":1351824120}";
            deserializeJson(doc, test);
            comController->upload(doc);
            pmController.disableGNSS();
            delay(500);
            break;
        }
    }

    if (Serial1.available())
    {
        Serial.print(Serial1.read());
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // Place instatiation here, Serial1 is not in the same compilation unit as ComController
    static ComController _comController(&radio, &max3243, &mux, &Serial1, RADIO_BAUD, CLIENT_ADDR, SERVER_ADDR);
    comController = &_comController;

    // SPI INIT
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV8);

    // ACCELEROMETER INIT
    Serial.println("Setting up MMA");
    digitalWrite(ACCEL_INT, INPUT_PULLUP);
    mmaSetupSlideSentinel();
    configInterrupts(mma);
    attachInterrupt(digitalPinToInterrupt(ACCEL_INT), accelInt, FALLING);
    mma.readRegister8(MMA8451_REG_TRANSIENT_SRC);

    // RTC INIT
    pinMode(RTC_INT, INPUT_PULLUP); //active low interrupts
    initializeRTC();

    // must be done after first call to attac1hInterrupt()
    pmController.init();

    // GNSS INIT
    Serial2Setup(115200);

    // SD Card Initialization
    pinMode(SD_CS, OUTPUT);

    //CONSIDER a pointer to the file to be written to currently in the state machine
    setup_sd();
}

void loop()
{

    /* Use sprintf to right justify floating point prints. */
    char rj[30];
    /* Only want 1 call to SH_SendString as semihosting is quite slow.
     * sprintf everything to this array and then print using array. */
    char str[1000];
    int str_i;
    sbp_setup();

    while (1)
    {

        if (ADVANCED)
        {
            advancedTest();
        }

    /*
     * sbp_process must be called periodically in your
     * main program loop to consume the received bytes
     * from Piksi and parse the SBP messages from them.
     *
     * In this tutorial we use a FIFO structure to hold the data
     * before it is consumed by sbp_process; this helps ensure that no
     * data is lost or overwritten between calls to sbp_process. See
     * tutorial_implementation.c for the interaction between the USART
     * and the FIFO.
     *
     * sbp_process must be passed a function that conforms to the definition
     *     u32 get_bytes(u8 *buff, u32 n, void *context);
     * that provides access to the bytes received from Piksi. See fifo_read and
     * related code in tutorial_implementation.c for a reference.
     */
    
        if (Serial2.available())
            fifo_write(Serial2.read());

        s8 ret = sbp_process(&sbp_state, &fifo_read);

        /* Semihosting is slow - each loop the FIFO fills up and packets get
         * dropped, so we don't check the return value from sbp_process. It's a good
         * idea to incorporate this check into your host's code, though. 
         */
        if (ret < 0)
            printf("sbp_process error: %d\n", (int)ret);

        /* Print data from messages received from Piksi. */
        
        DO_EVERY(10000,
                 str_i = 0;
                 memset(str, 0, sizeof(str));

                 str_i += sprintf(str + str_i, "\n\n\n\n");

                 str_i += sprintf(str + str_i, "GPS Time:\n");
                 str_i += sprintf(str + str_i, "\tWeek\t\t: %6d\n", (int)gps_time.wn);
                 sprintf(rj, "%6.2f", ((float)gps_time.tow) / 1e3);
                 str_i += sprintf(str + str_i, "\tSeconds\t: %9s\n", rj);
                 str_i += sprintf(str + str_i, "\n");


                /* -------------- RTK --------------
                 * The msg_pos_llh_t struct contains an 8-bit, six digit flag member variable.
                 * This variable, the 3 least significant bits
                 * ---------------------------------
                 * | 0 | Invalid     |
                 * | 1 | SPP         |
                 * | 2 | DGNSS       |
                 * | 3 | Float RTK   |
                 * | 4 | Fixed RTK   |
                 * | 5 | Dead Reckon |
                 * | 6 | SBAS        |
                 * ---------------------------------
                 */

                 str_i += sprintf(str + str_i, "Absolute Position:\n");
                 sprintf(rj, "%4.10lf", pos_llh.lat);
                 str_i += sprintf(str + str_i, "\tLatitude\t: %17s\n", rj);
                 sprintf(rj, "%4.10lf", pos_llh.lon);
                 str_i += sprintf(str + str_i, "\tLongitude\t: %17s\n", rj);
                 sprintf(rj, "%4.10lf", pos_llh.height);
                 str_i += sprintf(str + str_i, "\tHeight\t: %17s\n", rj);
                 str_i += sprintf(str + str_i, "\tSatellites\t:     %02d\n", pos_llh.n_sats);
                 getFixMode(pos_llh, rj);
                 str_i += sprintf(str + str_i, "\tFix Mode\t: %17s\n", rj);
                 str_i += sprintf(str + str_i, "\n");

                 str_i += sprintf(str + str_i, "Baseline (mm):\n");
                 str_i += sprintf(str + str_i, "\tNorth\t\t: %6d\n", (int)baseline_ned.n);
                 str_i += sprintf(str + str_i, "\tEast\t\t: %6d\n", (int)baseline_ned.e);
                 str_i += sprintf(str + str_i, "\tDown\t\t: %6d\n", (int)baseline_ned.d);
                 str_i += sprintf(str + str_i, "\n");

                 str_i += sprintf(str + str_i, "Velocity (mm/s):\n");
                 str_i += sprintf(str + str_i, "\tNorth\t\t: %6d\n", (int)vel_ned.n);
                 str_i += sprintf(str + str_i, "\tEast\t\t: %6d\n", (int)vel_ned.e);
                 str_i += sprintf(str + str_i, "\tDown\t\t: %6d\n", (int)vel_ned.d);
                 str_i += sprintf(str + str_i, "\n");

                 str_i += sprintf(str + str_i, "Dilution of Precision:\n");
                 sprintf(rj, "%4.2f", ((float)dops.gdop / 100));
                 str_i += sprintf(str + str_i, "\tGDOP\t\t: %7s\n", rj);
                 sprintf(rj, "%4.2f", ((float)dops.hdop / 100));
                 str_i += sprintf(str + str_i, "\tHDOP\t\t: %7s\n", rj);
                 sprintf(rj, "%4.2f", ((float)dops.pdop / 100));
                 str_i += sprintf(str + str_i, "\tPDOP\t\t: %7s\n", rj);
                 sprintf(rj, "%4.2f", ((float)dops.tdop / 100));
                 str_i += sprintf(str + str_i, "\tTDOP\t\t: %7s\n", rj);
                 sprintf(rj, "%4.2f", ((float)dops.vdop / 100));
                 str_i += sprintf(str + str_i, "\tVDOP\t\t: %7s\n", rj);
                 str_i += sprintf(str + str_i, "\n");
                 Serial.print("Data: ");
                 Serial.println(str);
        );
    }
}
