/*
I am facing two methods for implementing this network. First is by using global flags within the method
to make desicions on state transitions. Another is to pass the state between each function which is much
safer.

This file is trying to implement the safe way and passes state transitions between functions. One Identifier
that it utlizes is returning integer values related to the ID of the incoming transmitter ID. The ID has to
be above 1. Right now for the receiver to identify whether it received a packet RTS or regular RTS to notify
receiver a transmitter is in its network, a value of the ID multiplied by ten is done. since there aren't many
transmitters in the network for early stages of development. This is a easy fix. Later it may be changed to
scale.

*/

/*
File Notes:
  - globalFlag is used for entering different states/functions and used to exit while loops
  - right now Network class contains all of the rover and base send/rec funcitonality
                  Would like to create inheritance classes in the future named Base
                  and Rover respectively to be child classes and inherit Network.
*/
#include <SD.h>
#include <CRC32.h>

/*
Rover handshake
    nameschema->RTSGPS
    nameschema->RTSPACKET
    nameevaluate->CTSGPS
    nameevaluate->CTSPACKET

base handshake
    nameevaluate->RTSPACKET
    nameevalutate->RTSGPS
    nameschema->CTS_PACKET
    nameschema->CTSGPS

both need to inherit name schema and nameevaluate. Only difference is the order at which they are evaluated. As in
Rover sends then receives/evaluates message. Base recieves/evaluates message then sends. So creating sub classes to
populate those values will make it more robust.

RTSWAKE
RTSPACKET

These do two different things
    RTSWAKE just sends a wake message but still runs through handshake
    RTSPACKET handshakes then sends a message

*/
class Network {
protected:
  Network(HardwareSerial &radioPort);
  HardwareSerial &radio;

protected:
  void RTS(bool handshakeFlag, int ID);
  void CTS(bool handshakeFlag, int ID);
  void sendPACKET(char packet[], int ID);
  bool backoff(unsigned long tStart);
  int m_parseIncomingMssg();

private:
  void m_sendMessage(char msg[]);
  void m_readIncomingPacket();
  int  m_messageRTSCTSHandler(char message[]);//Message Handler parses and returns a vector message
  int  m_messagePACKETHandler(char message[]);
  void m_append(char s[], char c);
  void m_appendStr(char s1[], char s2[]);
  bool m_readIn(char buf[], char chr);
  void m_loadPACKET(char message[]);
  long int backoffTime = 20000; //If a message isn't received for 20 seconds
                               //Backoff is envoked
  const int REC_BUFFER = 250; //only accepts 250 bytes per message
  char packetMessage[250];
  
};

Network::Network(HardwareSerial &radioPort)
  : radio(radioPort)
  {}

class Transmitter : public Network {
public:
  Transmitter(HardwareSerial &radioPort);
  void handshakeRTS(int ID, char packet[], bool sendPacket);

private:
  bool m_transmitterWaitForResponse();
  void m_createPACKET(char message[]);
  void m_loadPACKET(char message[]);

  char m_packet[250];
  int transmitterID;
};
Transmitter::Transmitter(HardwareSerial &radioPort)
  : Network(radioPort)
  {}
void Transmitter::handshakeRTS(int ID, char packet[], bool sendPacket){
  Serial.println("In handshakeRTS");
  bool handshakeFlag = false;
  bool backoffFlag = false;
  //set class values
  transmitterID = ID;

  while(handshakeFlag == false){
    RTS(handshakeFlag, ID);
    handshakeFlag = m_transmitterWaitForResponse();
  }

  if(sendPacket == true)
    sendPACKET(packet, transmitterID);
}
bool Transmitter::m_transmitterWaitForResponse(){
  Serial.println("In waitForResponse");
  unsigned long timeStart = millis();
  bool backoffFlag = false;
  bool handshakeFlag = false;
  int ID = 0;

  while(handshakeFlag == false) {
    if(radio.available())
      ID = m_parseIncomingMssg();

    handshakeFlag = ID;

    backoffFlag = backoff(timeStart);
    if(backoffFlag == true)
      return false;
  }

  return true;
}
class Receiver : public Network {
public:
  Receiver(HardwareSerial &radioPort);
  int handshakeCTS();

private:
  void waitForPacketWithID(int ID);
  int recWaitForResponse();

};
Receiver::Receiver(HardwareSerial &radioPort)
  : Network(radioPort)
  {}
int Receiver::handshakeCTS(){
  Serial.println("In handshakeCTS");
  bool handshakeFlag = false;
  int transmitterID;

  transmitterID = recWaitForResponse();
  CTS(handshakeFlag, transmitterID);
  //CTS/RTS messages will return ID correlated to the rover ID
  //Packet messages will return a ID multiplied by ten. The reason
  //Is for identifying the type of message for it to be handled accordingly.

  if(transmitterID >= 10)
    waitForPacketWithID(transmitterID);

  return transmitterID;
}
int Receiver::recWaitForResponse(){
  Serial.println("In waitForResponse");
  bool handshakeFlag = false;
  int ID = 0;
  while(handshakeFlag == false) {
    if(radio.available())
      ID = m_parseIncomingMssg();

    handshakeFlag = ID;
  }

  return ID;
}
void Receiver::waitForPacketWithID(int ID){
  bool packetwaitFlag = false;
  while(packetwaitFlag == false){
    if(radio.available())
      m_parseIncomingMssg();
  }
}
class Base : public Receiver {
public:
  Base(HardwareSerial &radioPort, int maxNumRovers);
  void checkIncomingMessages();

private:
  void queueID();
  void enqueueID();
  int numRovers;
};
Base::Base(HardwareSerial &radioPort, int maxNumRovers)
  : Receiver(radioPort)
  , numRovers{maxNumRovers}
  {}
void Base::checkIncomingMessages(){
  while(true){
    handshakeCTS();
    Serial.println("=====================");
    Serial.println("End of a transmission");
    Serial.println("=====================");
  }
}

class Rover : protected Transmitter {
public:
  Rover(HardwareSerial &radioPort, int ID);
  void alertBaseUponWake();
  void sendBasePacket(char packet[]);

private:
  int roverID;
};
Rover::Rover(HardwareSerial &radioPort, int ID)
  : Transmitter(radioPort)
  , roverID{ID}
  {}
void Rover::alertBaseUponWake(){
  handshakeRTS(roverID, "", false);
}
void Rover::sendBasePacket(char packet[]){
  handshakeRTS(roverID, packet, true);
}
void Network::m_loadPACKET(char message[]){
  memset(packetMessage, '\0', sizeof(packetMessage));
  for(int i = 0; i < strlen(message); i++)
    packetMessage[i] = message[i];
}
void Network::RTS(bool handshakeFlag, int ID){
  Serial.println("IN RTS_GPS");
  if(handshakeFlag == false){
    char message[10];
    int pass = sprintf(message, "RTS_%d_", ID);
    for(int i = 0; i < 5; i++)
      m_sendMessage(message);
  }
}
void Network::CTS(bool handshakeFlag, int ID){
  Serial.println("In CTS GPS");
  if(handshakeFlag == false){
    char message[10];
    int pass = sprintf(message, "CTS_%d_", ID);
    for(int i = 0; i < 5; i++)
      m_sendMessage(message);
  }
}
bool Network::backoff(unsigned long tStart){
  unsigned long tCurrent = millis();
  if(tCurrent - tStart > backoffTime){
    Serial.println("Time Backoff Ocurred");
    return true;
  }
  return false;
}

int Network::m_parseIncomingMssg() {
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
        int IDandAction = 0;

        IDandAction = 10 * m_messagePACKETHandler(messageIn);
        if(IDandAction != 0)
          IDandAction = m_messageRTSCTSHandler(messageIn);

        memset(messageIn, '\0', sizeof(messageIn));
        return IDandAction;
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

  memset(messageIn, '\0', sizeof(messageIn));
  return 0;
}
int Network::m_messageRTSCTSHandler(char message[]){
  bool RTSFLAG = false;
  bool CTSFLAG = false;

  Serial.print("Message: ");
  Serial.println(message);

  char messageTemp[REC_BUFFER];
  for(int i = 0; i < strlen(message); i++)
    messageTemp[i] = message[i];

  char *token = strtok(messageTemp, "_");
  while(token != NULL){
      Serial.print("token: ");
      Serial.println(token);
    if(strcmp(token, "*CTS") == 0){
      Serial.println("Recieved CTS GPS ");
      CTSFLAG = true;
    }else if(strcmp(token, "*RTS") == 0){
      Serial.println("Recieved RTS GPS ");
      RTSFLAG = true;
    }else if(atoi(token) >= 1 && atoi(token) <= 10){//Rovers with ID's between 1-10 where number correlates to # of rovers in field
      if(RTSFLAG == true || CTSFLAG == true)
        return atoi(token);
    }
    token = strtok(NULL, "_");
  }

  return 0;
}
int Network::m_messagePACKETHandler(char message[]){
  bool PACKETFLAG = false;

  Serial.print("Message: ");
  Serial.println(message);

  char messageTemp[REC_BUFFER];
  for(int i = 0; i < strlen(message); i++)
    messageTemp[i] = message[i];

  char *token = strtok(messageTemp, "_");
  while(token != NULL){
      Serial.print("token: ");
      Serial.println(token);
    if(strcmp(token, "*PACKET") == 0){
      Serial.println("Recieved Packet ");
      PACKETFLAG = true;
    }else if(atoi(token) >= 1 && atoi(token) <= 10 && PACKETFLAG == true){//Rovers with ID's between 1-10 where number correlates to # of rovers in field
      int ID = atoi(token);
      token = strtok(NULL, "_");
      Serial.print("This is the sent packet: ");
      Serial.println(token);
      Serial.print("With token ID: ");
      Serial.println(ID);
      return atoi(token);
    }
    token = strtok(NULL, "_");
  }

  return 0;
}
void Network::sendPACKET(char packet[], int ID){
  Serial.println("IN sendPacket");
  char message[250];
  int pass = sprintf(message, "PACKET_%d_%s_", ID, packet);
  m_sendMessage(message);
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
