/* 
 * TCP Sync Door
 * Hardware: Arduino Uno + Ethernet shield Enc28J60
 * @author: Vanderlei Mendes
 * Project to solve a problem of my friend Gustavo (Só Portões)
 * 
 * This is the Server part wich returns JSON and it's intended to 
 * be used with another Arduino Uno + ENC28J60 as a client
 * 
 * It checks the Slave Door. If it's in the same (opened/opened or closed/closed) condition 
 * as the Master door, slave door is activated as the Master Door is,
 * if i'ts in different condition (opened/closed or closed/opened), the command is performed only
 * on the Master Door achieving the same condition 
 */

#include <EtherCard.h>

static byte mymac[] = {0x74, 0x69, 0x68, 0x2D, 0x30, 0x31};
static byte myip[] = {192, 168, 25, 16};
byte Ethernet::buffer[700];

const int ledPin = A4; //command status pin
boolean doorLedStatus = false;

const int doorPin = A5; // sensor pin to check if door is opened/closed
int doorSlaveStatus;

char *doorSlaveStatusVerbose;
char *commandSlaveVerbose;

const int doorCommand = A3; //command to activate the door
boolean commandPending = false;


void setup()
{

  Serial.begin(57600);
  Serial.println("TCP Sync Door System ( Slave Unit )");

  if (!ether.begin(sizeof Ethernet::buffer, mymac, 10))
    Serial.println("Failed to access Ethernet controller");
  else
    Serial.println("Ethernet controller initialized");

  if (!ether.staticSetup(myip))
    Serial.println("Failed to set IP address");

  Serial.println();

  pinMode(ledPin, OUTPUT); //door status led: on = door closed, off = door opened
  digitalWrite(ledPin, LOW);
  doorLedStatus = false;

  pinMode(doorPin, INPUT); //door sensor: HIGH = DOOR CLOSED, LOW = DOOR OPENED

  pinMode(doorCommand, OUTPUT); //relay to perfom door activation command
  digitalWrite(doorCommand, LOW);
}

void loop()
{

  doorSlaveStatus = digitalRead(doorPin); //read door condition

  if (doorSlaveStatus == LOW)
  {
    digitalWrite(ledPin, LOW);
  }
  else
  {
    digitalWrite(ledPin, HIGH);
  }

  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos)
  {
    //check if it's opened
    if (strstr((char *)Ethernet::buffer + pos, "GET /?masterdoor=closing") != 0)
    {
      Serial.println("---------- REQUEST STARTED closing ----------");
      Serial.println();
      if (doorSlaveStatus == LOW)
      {
        Serial.println("Master Door is closing. Performing closing command on opened Slave Door");
        doorLedStatus = true;
      }
      else
      {
        Serial.println("Master Door is closing and Slave Door is already closed.");
        Serial.println();
        Serial.println("---------- REQUEST END ----------");
        Serial.println();
        doorLedStatus = false;
      }
    }

    //Check if it's closed
    if (strstr((char *)Ethernet::buffer + pos, "GET /?masterdoor=opening") != 0)
    {
      Serial.println("---------- REQUEST STARTED openning----------");
      Serial.println();
      if (doorSlaveStatus == HIGH)
      {
        Serial.println("Master Door is opening. Performing opening command on closed Slave Door");
        doorLedStatus = true;
      }
      else
      {
        Serial.println("Master Door is opening and Slave Door is already opened.");
        Serial.println();
        Serial.println("---------- REQUEST END ----------");
        Serial.println();
        doorLedStatus = false;
      }
    }    

    if (doorLedStatus)
    {     
      doorSlaveStatusVerbose = "same";
      commandSlaveVerbose = "activating";      
      commandPending = true;
      Serial.println("Command sent.");
    }
    else
    {     
      doorSlaveStatusVerbose = "different";
      commandSlaveVerbose = "idle";      
      Serial.println("command unnecessary.");
    }

    BufferFiller bfill = ether.tcpOffset();
    bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\n"
                      "Content-Type: application/json\r\nPragma: no-cache\r\n\r\n"
                      "{\"slavedoor\":\"$S\", "
                      "\"commandslavedoor\":\"$S\"}"),
                      doorSlaveStatusVerbose, commandSlaveVerbose);
    ether.httpServerReply(bfill.position());
  }

  if (commandPending)
  {
    Serial.println("1 second command initiated...");
    digitalWrite(doorCommand, HIGH);
    delay(1000);
    digitalWrite(doorCommand, LOW);
    Serial.println("1 second command terminated!");
    commandPending = false;
    Serial.println();
    Serial.println("---------- REQUEST END ----------");
    Serial.println();
  }
}
