/*
  SerialPassthrough sketch

  Some boards, like the Arduino 101, the MKR1000, Zero, or the Micro,
  have one hardware serial port attached to Digital pins 0-1, and a
  separate USB serial port attached to the IDE Serial Monitor.
  This means that the "serial passthrough" which is possible with
  the Arduino UNO (commonly used to interact with devices/shields that
  require configuration via serial AT commands) will not work by default.

  This sketch allows you to  emulate the serial passthrough behaviour.
  Any text you type in the IDE Serial monitor will be written
  out to the serial port on Digital pins 0 and 1, and vice-versa.

  On the 101, MKR1000, Zero, and Micro, "Serial" refers to the USB Serial port
  attached to the Serial Monitor, and "Serial1" refers to the hardware
  serial port attached to pins 0 and 1. This sketch will emulate Serial passthrough
  using those two Serial ports on the boards mentioned above,
  but you can change these names to connect any two serial ports on a board
  that has multiple ports.

   Created 23 May 2016
   by Erik Nyquist
*/

char replybuffer[255];
int16_t timer = 10000;
int sendFlag=true;

char out_str[339]; // rockblock has max of 340 bytes it can send

void setup() {
  Serial.begin(9600);
  Serial1.begin(19200);

while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

//turn off echo

  sendCheckReply("ATE0","OK",timer);


  if (sendCheckReply("AT","OK",timer)) {

  Serial.println("Got it.");
  
}

delay(2000);


}

void loop() {

// get signal level

int level = getSignal(timer);
Serial.print("\nsignal=");
Serial.println(level);
Serial.println();

char str[] = "AT+SBDWT=";
strcpy(out_str,str);

char send_string [] = "Hello";

Serial.print("send string=");
Serial.println(send_string);
Serial.print("string length=");
Serial.println(strlen(send_string));

Serial.print("rock string=");
strcat(out_str,send_string);
Serial.println(out_str);


if ((level>=3)&&(sendFlag==true)) {
//if (level>=3) {

Serial.println("Signal quality sufficient! Sending ...");
  sendCheckReply("AT&K0","OK",timer);
  sendCheckReply("AT+SBDWT=Hello World","OK",timer);
  sendCheckReply("AT+SBDIX","OK",timer*2);
  sendFlag=false;
}

delay(5000);

}

int getSignal(uint16_t timeout) {
if (! getReply("AT+CSQ",timeout))
return -1;

if (strcmp(replybuffer,"+CSQ:0")==0) return 0;
else if (strcmp(replybuffer,"+CSQ:1")==0) return 1;
else if (strcmp(replybuffer,"+CSQ:2")==0) return 2;
else if (strcmp(replybuffer,"+CSQ:3")==0) return 3;
else if (strcmp(replybuffer,"+CSQ:4")==0) return 4;
else if (strcmp(replybuffer,"+CSQ:5")==0) return 5;
else if (strcmp(replybuffer,"+CSQ:5")==6) return 6;
else return 9;

}

boolean sendCheckReply(char *send, char *reply, uint16_t timeout) {
  if (! getReply(send, timeout) )
    return false;

  return (strcmp(replybuffer, reply) == 0);
}

uint8_t getReply(char *send, uint16_t timeout) {
  flushInput();


  Serial.print(F("\t---> ")); 
  Serial.println(send);

  Serial1.println(send);

  uint8_t l = readline(timeout,false);

  Serial.print(F("\t<--- ")); Serial.println(replybuffer);

  return l;
}

uint8_t readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= 254) {
      //DEBUG_PRINTLN(F("SPACE"));
      break;
    }

    while(Serial1.available()) {
      char c =  Serial1.read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      //DEBUG_PRINT(c, HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);
      replyidx++;
    }

    if (timeout == 0) {
      //DEBUG_PRINTLN(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

void flushInput() {
    // Read all available serial input to flush pending data.
    uint16_t timeoutloop = 0;
    while (timeoutloop++ < 40) {
        while(Serial1.available()) {
            Serial1.read();
            timeoutloop = 0;  // If char was received reset the timer
        }
        delay(1);
    }
}
