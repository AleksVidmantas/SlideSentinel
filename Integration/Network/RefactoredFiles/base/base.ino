#include <SD.h>
#include <SPI.h>
#include <wiring_private.h>
#include <IridiumSBD.h>
#include <EnableInterrupt.h>
#include <MemoryFree.h>
#include <Adafruit_SleepyDog.h>
#include "SlideS_parser.h"
#include "TopLevelNetwork.h"

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  while(!Serial);
  Serial.println("Setup Init");
  serialFlush();
}

bool gameLoop = true;

void loop(){
  
  /*
  if(gameLoop == true){
    Base base(Serial1, 1);
    base.checkIncomingMessages();
    gameLoop = false;
  }

  
  /**/
  Serial1.write((byte)'T');
  /**/
  delay(1000);
  Serial.println("loop");
}

void serialFlush()
{
  while (Serial1.available() > 0)
  {
    char t = Serial1.read();
  }
}
