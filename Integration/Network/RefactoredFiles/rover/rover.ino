#include <SD.h>
#include <SPI.h>
#include <wiring_private.h>
#include <IridiumSBD.h>
#include <EnableInterrupt.h>
#include <MemoryFree.h>
#include <Adafruit_SleepyDog.h>
#include "SlideS_parser.h"
#include "TopLevelNetwork.h"

#define enable_pin 13
#define disable_pin 19
#define RADIOROUTE A0

void setup() {
  Serial.begin(115200);
  //Serial1.begin(115200);
  while(!Serial);
  delay(1000);
  Serial.println("Setup Init");
/*
  pinMode(enable_pin, OUTPUT);
  pinMode(disable_pin, OUTPUT);
  pinMode(RADIOROUTE, OUTPUT);
  
  digitalWrite(enable_pin, HIGH);
  delay(10);
  digitalWrite(enable_pin, LOW);

  radioToGPS();*/
  //serialFlush();
}


bool gameLoop = true;

void loop(){
  /*
  if(gameLoop == true){
    Rover rover(Serial1, 1);
    rover.alertBaseUponWake();
    for(int i = 0; i < 10; i++){
      char message[20];
      int b = sprintf(message, "Intro to networks %d", i);
      rover.sendBasePacket(message);
      gameLoop = false;
    }

  } 
  
  /**/
  Serial.println("Sending message");
  char message[8] = "Hello";
  while(true){
    sendMessage(message);
    delay(2000);
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

void radioToGPS(){
  digitalWrite(RADIOROUTE, HIGH);
}

void serialFlush()
{
  while (Serial1.available() > 0)
  {
    char t = Serial1.read();
  }
}
