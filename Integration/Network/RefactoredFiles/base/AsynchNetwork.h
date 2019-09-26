/*
File Notes:
  - globalFlag is used for entering different states/functions and used to exit while loops
  - right now Network class contains all of the rover and base send/rec funcitonality
                  Would like to create inheritance classes in the future named Base
                  and Rover respectively to be child classes and inherit Network.
*/
#include <SD.h>
#include <CRC32.h>

class Network {
public:
  Network(HardwareSerial &radioPort, char ID[]);
  void RoverHandshakeGPS();
  void BaseHandshakeGPS();
  void RoverHandshakePACKET();
  void BaseHandshakePACKET();
  void loadPACKET(char message[]);

private:
  void m_RTS_GPS();
  void m_CTS_GPS();
  void m_RTS_PACKET();
  void m_CTS_PACKET();
  void m_RoverWaitForResponse();
  void m_BaseWaitForResponse();
  void m_sendPACKET();
  void m_backoff(unsigned long tStart);
  void m_sendMessage(char msg[]);
  void m_readIncomingPacket();
  void m_parseIncomingMssg();
  void m_messageHandler(char message[]);
  void m_append(char s[], char c);
  void m_appendStr(char s1[], char s2[]);
  bool m_readIn(char buf[], char chr);

  char roverID[3];//ID number for Rover
  bool globalFlag = false;
  bool backoffFlag = false;
  long int backoffTime = 20000; //If a message isn't received for 20 seconds
                                //Backoff is envoked
  HardwareSerial &radio;
  const int REC_BUFFER = 250; //only accepts 250 bytes per message
  char packetMessage[250];
};

Network::Network(HardwareSerial &radioPort, char ID[])
  : radio(radioPort), roverID{} {
  for(int i = 0; i < strlen(ID); i++)
    this->roverID[i] = ID[i];
}

void Network::loadPACKET(char message[]){
  memset(packetMessage, '\0', sizeof(packetMessage));
  for(int i = 0; i < strlen(message); i++)
    packetMessage[i] = message[i];
}

void Network::RoverHandshakeGPS(){
  //These two globalFlags are set to keep track of the state of messages sent/received
  globalFlag = false;
  backoffFlag = false;//Backoff occurs when a CTS message is not received from the receiver so a new message must be sent
  Serial.println("In RoverHandshakeGPS");
  while(globalFlag == false){
    m_RTS_GPS();
    m_RoverWaitForResponse();

    if(backoffFlag == true)
      globalFlag = false;
  }
}

void Network::BaseHandshakeGPS(){
  globalFlag = false;
  Serial.println("In BaskHandshakeGPS");
  while(globalFlag == false){
    m_BaseWaitForResponse();
    m_CTS_GPS();
  }
}

void Network::RoverHandshakePACKET(){
  globalFlag = false;
  backoffFlag = false;
  Serial.println("In RoverHandshakePACKET");
  while(globalFlag == false){
    m_RTS_PACKET();
    m_RoverWaitForResponse();

    if(backoffFlag == true)
      globalFlag = false;
  }
  //Once this is while loop exits, the packet is ready to be sent
  m_sendPACKET();
}

void Network::BaseHandshakePACKET(){
  globalFlag = false;
  Serial.println("In BaseHandshakePACKET");
  while(globalFlag == false){
    m_BaseWaitForResponse();
    m_CTS_PACKET();
  }
  m_readIncomingPacket();
}

void Network::m_RTS_GPS(){
  Serial.println("IN RTS_GPS");

  if(globalFlag == false){
    char message[30] = "RTSGPS_ROVER_";
    //Note roverID[0] was a quick fix to send a char in that function instead of a c str. only accepts single digit numbers between 0-9
    m_appendStr(message, roverID);
    m_append(message, '_');
    //In original version, sendMessage was encapsulated in a for loop to send the message
    //Multiple times UDP style. But it worked with sending the message once so thats
    //What is implemented below.
    m_sendMessage(message);
    backoffFlag = false;
  }
}

void Network::m_CTS_GPS(){
  Serial.println("In CTS GPS");

  if(globalFlag == true){
    char message[20] = "CTSGPS_ROVER_";
    m_appendStr(message, roverID);
    m_append(message, '_');
    for(int i = 0; i < 5; i++){
      m_sendMessage(message);
      delay(10);
    }
  }
}

void Network::m_RTS_PACKET(){
  Serial.println("In RTS_PACKET");

  if(globalFlag == false){
    char message[30] = "RTSPACKET_ROVER_";
    m_appendStr(message, roverID);
    m_append(message, '_');

    m_sendMessage(message);
    backoffFlag = false;
  }
}

void Network::m_CTS_PACKET(){
  Serial.println("In CTS_PACKET");

  if(globalFlag == true){
    char message[20] = "CTSPACKET_ROVER_";
    m_appendStr(message, roverID);
    m_append(message, '_');
    m_sendMessage(message);
  }
}

void Network::m_readIncomingPacket(){
  globalFlag = false;
  Serial.println("In reading packet");
  m_BaseWaitForResponse();
}

void Network::m_RoverWaitForResponse(){
  unsigned long timeStart = millis();
  Serial.println("In waitForResponse");
  while(globalFlag == false) {
    if(Serial1.available())
      m_parseIncomingMssg();

    m_backoff(timeStart);
  }
}

void Network::m_BaseWaitForResponse(){
  unsigned long timeStart = millis();
  Serial.println("In waitForResponse");
  while(globalFlag == false) {
    if(Serial1.available())
      m_parseIncomingMssg();
  }
}

void Network::m_sendPACKET(){
  char message[250] = "PACKET_ROVER_0_";
  m_appendStr(message, packetMessage);
  m_append(message, '_');
  m_sendMessage(message);
}

void Network::m_backoff(unsigned long tStart){
  unsigned long tCurrent = millis();
  if(tCurrent - tStart > backoffTime){
    globalFlag = true;
    backoffFlag = true;
    Serial.println("Time Backoff Ocurred");
  }
}

void Network::m_sendMessage(char msg[]){
  radio.write(byte('*'));
  for(int i = 0; i < strlen(msg); i++)
    radio.write(msg[i]);

  radio.write(byte('!'));
  radio.write(byte('!'));
  radio.write(byte('!'));
  radio.write(byte('!'));

  Serial.print("Sending this message: ");
  Serial.println(msg);
}

void Network::m_parseIncomingMssg() {
  Serial.println("Inside parseIncomingMssg");
  char messageIn[REC_BUFFER];
  char input;
  int count = 0;
  bool receivingMessage = false;
  if(m_readIn(messageIn, '*')){
    receivingMessage = true;
    while(radio.available()){
      input = radio.read();
      if(input == (byte)'!' || count == 250 - 1){
        break;
      }
      count++;
      m_append(messageIn, input);
      Serial.print("Char: ");
      Serial.println(input);
      Serial.print("Constructed Message: ");
      Serial.println(messageIn);
    }
  }else {//Basically flushes radio of messages that don't follow message format
    char trashBin = radio.read();
  }

  if(receivingMessage == true){
    Serial.print("Hopefully Whole Message rec: ");
    Serial.println(messageIn);
    m_messageHandler(messageIn);
  }

  memset(messageIn, '\0', sizeof(messageIn));
  //serialFlush();//May not need this since trashBin catches unwarranted radio input
}

void Network::m_messageHandler(char message[]){
  //Flags triggered to decide what action to take with incoming message
  bool CTSGPSFLAG = false;
  bool CTSPACKETFLAG = false;
  bool RTSGPSFLAG = false;
  bool RTSPACKETFLAG = false;
  bool PACKETFLAG = false;

  Serial.print("Message: ");
  Serial.println(message);

  char messageTemp[REC_BUFFER];
  for(int i = 0; i < strlen(message); i++)
    messageTemp[i] = message[i];

  char *token = strtok(messageTemp, "_");
  while(token != NULL){
      Serial.print("TEST------>");
      Serial.println(token);
    if(strcmp(token, "*CTSGPS") == 0){
      Serial.println("Recieved CTS GPS ");
      CTSGPSFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "*RTSGPS") == 0){
      Serial.println("Recieved RTS GPS ");
      RTSGPSFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "*CTSPACKET") == 0){
      Serial.println("Recieved CTS PACKET ");
      CTSPACKETFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "*RTSPACKET") == 0){
      Serial.println("Recieved RTS PACKET ");
      RTSPACKETFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "*PACKET") == 0){
      PACKETFLAG = true;
      globalFlag = true;
    }else if(strcmp(token, "ROVER") == 0){
      //Nothing to do if it sees its rover yet
    }else if(strcmp(token, roverID) == 0){
      Serial.print("Rover ID: ");
      Serial.println(token);
      if(CTSPACKETFLAG == true){
        //State is triggered for when Sender receives a CTSPACKET message
        //This state will exit all the way back to RoverHandshakePACKET then sendPACKET
        //Can take furthur action to take advantage of the current state
        token = strtok(NULL, "_");
        Serial.print("Packet Message: ");
        Serial.println(token);
      }else if(PACKETFLAG == true){
        //This state is triggered when Receiver Sent a CTSPACKET Message and current message is the packet
        token = strtok(NULL, "_");
        Serial.print("Packet Message: ");
        Serial.println(token);
      }
    }
    token = strtok(NULL, "_");
  }
}

bool Network::m_readIn(char buf[], char chr){
  char input;
  if(radio.available()){
    input = radio.read();
    if(input == chr){
      append(buf, chr);
      return true;
    }

    else
      return false;
  }
}

void Network::m_append(char s[], char c){
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

void Network::m_appendStr(char s1[], char s2[]){
  int len = strlen(s1);
  for(int i = 0; i < strlen(s2); i++){
    s1[len + i] = s2[i];
    s1[len + i + 1] = '\0';
  }
}
