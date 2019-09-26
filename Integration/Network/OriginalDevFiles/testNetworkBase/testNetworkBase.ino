#include <SD.h>
#include <SPI.h>
#include <wiring_private.h>
#include <IridiumSBD.h>
#include <EnableInterrupt.h>
#include <MemoryFree.h>
#include <Adafruit_SleepyDog.h>
#include "SlideS_parser.h"

//#define SEL A2 //Whatever pin is free on feather
//#define SERIAL2_RX 13      // Rx pin radio
//#define SERIAL2_TX 11      // Tx pin for first serial port

#define CD 13

bool responseSwitch = false;
/*
void Serial2Setup();//Sets up GPS
void muxToGps();//Switches the mux from Tx(radio) -> Rx(GPS)
void muxToFeather();//Switches the mux from Tx(radio) -> Rx(Feather)
void incomingRoverMessage();//checks incoming rover mesage
*/
void setup() {
  Serial.begin(115200);
 
  Serial1.begin(115200);
  while(!Serial);
  Serial.println("Setup Init Base");
  serialFlush();
  // Assign pins 10 & 11 SERCOM functionalit
  
}
int b = 0;
bool timertrigger = false;
bool globalFlag = false;
bool gameLoop = true;

void loop(){
  //Testing receive only
  //Test 1
  
  if(Serial1.available()){
    Serial.println("Glimpse of message");
    parseIncomingMssg();
  }

  //Test 2
  //Test 2, Send, receive/backoff->resend
  /*
  if(globalFlag == true){
    char message[18] = "Hello World!!!!";
    sendMessage(message);
    globalFlag = false;
    timertrigger = true;
  }
  
  unsigned long timeStart = millis();
  
  while(globalFlag == false){
    if(Serial1.available())
      parseIncomingMssg();
      
    if(timertrigger == true)
       backoff(timeStart);
  }

  //Test 3 CTSGPS(for practice) CTSPACKET translate packet
  /*

  if(gameLoop == true){
     //send rtspacket
     //backoff
     handshakeGPS();
     handshakePACKET();
     
     gameLoop = false;
     readIncomingPacket();
  }
  //Test 3.1 Test different timing of packets to make sure its asynchronous
  //Set delay to different times. 
/**/
}

void handshakeGPS(){
  globalFlag = false;
  Serial.println("In handshake GPS");
  while(globalFlag == false){
    waitForResponse();
    CTS_GPS();
  }
}

void handshakePACKET(){
  globalFlag = false;
  Serial.println("In handshake Packet");
  while(globalFlag == false) {
    waitForResponse();
    CTS_PACKET();
  }
}

void CTS_GPS(){
  Serial.println("In CTS GPS");
  if(globalFlag == true){
    char message[20] = "CTSGPS_ROVER_0_";
    for(int i = 0; i < 1; i++){//udp style send hella packets hope rec recieves it
      sendMessage(message);
    }
  }
}

void CTS_PACKET(){
  Serial.println("In CTS PACKET");
  if(globalFlag == true){
    char message[20] = "CTSPACKET_ROVER_0_";
     for(int i = 0; i < 1; i++){//udp style send hella packets hope rec recieves it
      sendMessage(message);
    }
  }
}

void readIncomingPacket(){
  globalFlag = false;
  Serial.println("In readIncomingPAcket");
  waitForResponse();
}

void waitForResponse(){
  //Only need this with backoff
  //unsigned long timeStart = millis(); 
  Serial.println("In wait for response");
  while(globalFlag == false){
    if(Serial1.available())
      parseIncomingMssg();

    //May need to look into reusing backoff for Base 
    //Mostly because base only responds to requests and doesnt
    //Worry about receiving a response
    //backoff(timeStart);
  }
}

void sendMessage(char msg[])
{
  Serial1.write(byte('*')); // start of text
  Serial.println(msg);
  for (int i = 0; i < strlen(msg); i++)
  {
    Serial1.write(msg[i]);
  }
  Serial1.write(byte('!')); // End of transmission, pad this with more 4's
  Serial1.write(byte('!'));
  Serial1.write(byte('!'));
  Serial1.write(byte('!'));
}

//Time backoff if nothing has been received it needs to resend message and hopefully it gets it
void backoff(unsigned long tStart){
  unsigned long tCurrent = millis();
  if(tCurrent - tStart > 15000){
    globalFlag = true;
  }
}

void parseIncomingMssg() {
  Serial.println("Inside ParseIncomeing Message");
  unsigned long internal_time_cur;
  unsigned long internal_time;
  int count = 0;
  char messageIn[250];
  memset(messageIn, '\0', sizeof(messageIn));
  internal_time_cur = millis();
  char input;
  if (PSTI(messageIn, '*')){
    while (Serial1.available()){
      input = Serial1.read();
      Serial.println(input);
      if (input == (byte)'!' || count == 250 - 1){
        Serial.println("Broke Reading message");
        break;
      }
      Serial.print("Char: ");
      Serial.println(input);
      append(messageIn, input);
      Serial.print("Alternative : ");
      Serial.println(messageIn);
    }
  }else {
    char trashBin = Serial1.read();
  }
  
  //Parses input and checks message for its type to service it respectively.
  messageType(messageIn);
 
  memset(messageIn, '\0', sizeof(messageIn));
  serialFlush();
  Serial.println("Serial Flushed");
}

//First off my original implentation of this was only to set the globalFlag, now it records Rover ID,
//May consider sending CTSGPS or PACKEt in this function
//Compares what type of message is incoming within parseIncomingMssg
void messageType(char message[]){
  bool RTSGPSFLAG = false;//if strcmp == RTSGPS
  bool RTSPACKETFLAG = false;//if strcmp == RTSPACKET
  bool PACKETFLAG = false; //if strcmp == PACKET
  Serial.print("Message from Rover inside messageType was: ");
  Serial.println(message);
  char mtemp[50];
  for(int i = 0; i < strlen(message); i++){
    mtemp[i] = message[i];
  }
  
  char *token = strtok(mtemp, "_");
  while(token != NULL){
    Serial.print("TEST------>");
    Serial.println(token);
    if(strcmp(token, "*RTSGPS") == 0){
      //set CTSGPS flag
      Serial.println("=====================");
      Serial.println("Recieved RTS GPS ");
      Serial.println("=====================");
      RTSGPSFLAG = true;
      globalFlag = true;
    }else if(strcmp(token,"*RTSPACKET") == 0){
      //This will prepare whether Base is ready for a Packet from recorded rover ID
      Serial.println("=====================");
      Serial.println("Recieved RTS PACKET ");
      Serial.println("=====================");
      RTSPACKETFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "PACKET") == 0){
      //This will set packet flag, which means hub is now servicing a large packet
      PACKETFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "ROVER") == 0){
      //we know its for rover, this conditional may be skipped.
      
    }else if(strcmp(token, "0") == 0){
      //record ID
      
      Serial.print("ROVER ID: ");
      Serial.println(token);
      if(PACKETFLAG == true){
        token = strtok(NULL, "_");
        Serial.print("Packet Message: ");
        Serial.println(token);
      }
    }
    token = strtok(NULL, "_");
  }
}
/*
void muxToGps(){
  Serial.println("Base Mux RadioTx -> GPSRx");
  //May need to instatiate wire names here so they are configured wires to
  //communicate with the associated pins
  digitalWrite(SEL, LOW);
}

void muxToFeather(){
  Serial.println("Base Mux RadioTx -> FeatherRx");
  //May need to instatiate wire names here so they are configured wires to
  //communicate with the associated pins
  digitalWrite(SEL, HIGH);
}*/

void serialFlush()
{
  while (Serial1.available() > 0)
  {
    char t = Serial1.read();
  }
}

bool PSTI(char buf[], char chr)
{
    char input;
    if (Serial1.available())
    {
        input = Serial1.read();
        if (input == chr)
        {
            append(buf, input);
            return true;
        }
        else
            return false;
    }
}
