/* 
 *  Check if door is opened API
 * Using Arduino Uno + Ethernet shield Enc28J60
 * author: Vanderlei Mendes
 * Project to solve a problem of my friend Gustavo (Só Portões)
 * 
 * This is the Server part wich returns JSON and it's intended to 
 * be used with another Arduino Uno + ENC28J60 as a client
 * 
 * When you send command to close door 1, it performs a check to see if the door 2 is closed
 * if the door i still opened, it will close the door 2 first and then closes the door 1  
 */

#include <EtherCard.h>

static byte mymac[] = {0x74, 0x69, 0x68, 0x2D, 0x30, 0x31};
static byte myip[] = {192, 168, 25, 16};
byte Ethernet::buffer[700];

const int ledPin = A4; //command status pin
boolean ledStatus;

const int doorPin = A5; //
int doorStatus;

//char* on = "closed";
//char* off = "opened";
char *doorStatusVerbose;
char *commandVerbose;

const int doorCommand = A3; //command to activate the door
boolean commandPending = false;

void setup()
{

  Serial.begin(57600);
  Serial.println("Conditional Door Closing");

  if (!ether.begin(sizeof Ethernet::buffer, mymac, 10))
    Serial.println("Failed to access Ethernet controller");
  else
    Serial.println("Ethernet controller initialized");

  if (!ether.staticSetup(myip))
    Serial.println("Failed to set IP address");

  Serial.println();

  pinMode(ledPin, OUTPUT); //door status led: on = door closed, off = door opened
  digitalWrite(ledPin, LOW);
  ledStatus = false;

  pinMode(doorPin, INPUT); //door sensor: HIGH = DOOR CLOSED, LOW = DOOR OPENED

  pinMode(doorCommand, OUTPUT); //relay to perfom door activation command
  digitalWrite(doorCommand, LOW);
}

void loop()
{

  doorStatus = digitalRead(doorPin); //read door condition

  if (doorStatus == LOW)
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

    if (strstr((char *)Ethernet::buffer + pos, "GET /?door=activate") != 0)
    {
      Serial.println("---------- REQUEST STARTED ----------");
      Serial.println();
      if (doorStatus == LOW)
      {
        Serial.println("Door is opened. Performing closing command...");
        ledStatus = true;
      }
      else
      {
        Serial.println("Door is already closed.");
        Serial.println();
        Serial.println("---------- REQUEST END ----------");
        Serial.println();
        ledStatus = false;
      }
    }

    /*if(strstr((char *)Ethernet::buffer + pos, "GET /?status=released") != 0) {
      Serial.println("Received RELEASED command");
      ledStatus = false;
    }*/

    if (ledStatus)
    {
      // digitalWrite(ledPin, HIGH);
      doorStatusVerbose = "opened";
      commandVerbose = "sent";
      commandPending = true;
      Serial.println("Command sent.");
    }
    else
    {
      //digitalWrite(ledPin, LOW);
      doorStatusVerbose = "closed";
      commandVerbose = "idle";
    }

    BufferFiller bfill = ether.tcpOffset();
    bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\n"
                      "Content-Type: application/json\r\nPragma: no-cache\r\n\r\n"
                      "{\"door\":\"$S\", \"command\":\"$S\"}"
                      //"<html><head><title>WebLed</title></head>"
                      //"<body>LED Status: $S "
                      //"<a href=\"/?status=$S\"><input type=\"button\" value=\"$S\"></a>"
                      //"</body></html>"
                      ),
                 doorStatusVerbose, commandVerbose, commandVerbose);
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
