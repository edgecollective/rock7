/*
  WiFi Web Server LED Blink

  A simple web server that lets you blink an LED via the web.
  This sketch will create a new access point (with no password).
  It will then launch a new server and print out the IP address
  to the Serial monitor. From there, you can open that address in a web browser
  to turn on and off the LED on pin 13.

  If the IP address of your shield is yourAddress:
    http://yourAddress/H turns the LED on
    http://yourAddress/L turns it off

  created 25 Nov 2012
  by Tom Igoe
  adapted to WiFi AP by Adafruit
 */

#include <SPI.h>
#include <WiFi101.h>
#include "arduino_secrets.h" 

#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>


// Configuration of the flash chip pins and flash fatfs object.
// You don't normally need to change these if using a Feather/Metro
// M0 express board.
#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
                                              // If you change this be
                                              // sure to change the fatfs
                                              // object type below to match.

#define FLASH_SS       SS1                    // Flash chip SS pin.
#define FLASH_SPI_PORT SPI1                   // What SPI port is Flash on?

Adafruit_SPIFlash flash(FLASH_SS, &FLASH_SPI_PORT);     // Use hardware SPI 

// Alternatively you can define and use non-SPI pins!
// Adafruit_SPIFlash flash(FLASH_SCK, FLASH_MISO, FLASH_MOSI, FLASH_SS);

//Adafruit_W25Q16BV_FatFs fatfs(flash);
Adafruit_M0_Express_CircuitPython pythonfs(flash);


// Configuration for the file to open and read:
#define FILE_NAME      "input.html"

////// satellite
int threshold = 3;
char replybuffer[255];
int16_t timer = 10000;
int sendFlag=true;

char out_str[339]; // rockblock has max of 340 bytes it can send


///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)

// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   90
// size of buffer that stores the incoming string
#define TXT_BUF_SZ   50

File webFile;                    // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
char txt_buf[TXT_BUF_SZ] = {0};  // buffer to save text to



int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

char message_buffer[255];

void setup() {


Serial1.begin(19200); // start the connection with the rockblock
  
Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  sendCheckReply("ATE0","OK",timer);

  Serial.print("Random=");
  Serial.println(random(10));
  
// get the file for reading
// Initialize flash library and check its chip ID.
  if (!flash.begin(FLASH_TYPE)) {
    Serial.println("Error, failed to initialize flash chip!");
    while(1);
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.GetJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!pythonfs.begin()) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    Serial.println("Was the flash chip formatted with the pythonfs_format example?");
    while(1);
  }
  Serial.println("Mounted filesystem!");
  
  
  
  WiFi.setPins(10,12,11);
  //WiFi.setPins(8,7,4,2);
  //Initialize serial and wait for port to open:
  

  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // by default the local IP address of will be 192.168.1.1
  // you can override it with the following:
   WiFi.config(IPAddress(10, 0, 0, 2));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(2000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}


void loop() {
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      byte remoteMac[6];

      // a device has connected to the AP
      Serial.print("Device connected to AP, MAC address: ");
      WiFi.APClientMacAddress(remoteMac);
      Serial.print(remoteMac[5], HEX);
      Serial.print(":");
      Serial.print(remoteMac[4], HEX);
      Serial.print(":");
      Serial.print(remoteMac[3], HEX);
      Serial.print(":");
      Serial.print(remoteMac[2], HEX);
      Serial.print(":");
      Serial.print(remoteMac[1], HEX);
      Serial.print(":");
      Serial.println(remoteMac[0], HEX);
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
  
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file

                    if (StrContains(HTTP_req,"GET /H")) {
                       digitalWrite(led, HIGH);   
                    }
                    if (StrContains(HTTP_req,"GET /L")) {
                       digitalWrite(led, LOW);   
                    }

                    
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();

                        // print the received text to the Serial Monitor window
                        // if received with the incoming HTTP GET string
                        if (GetText(txt_buf, TXT_BUF_SZ)) { /// this strips the incoming text of extraneous stuff
                          Serial.println("\r\nReceived Text:");
                          Serial.println(txt_buf);
                          
                          chirpText(txt_buf,TXT_BUF_SZ); /// our satellite function
                          
                        }
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page


            char message[] = "Signal level ... ";
            client.print(message);

            //calculate signal
            
            //int level = random(5);
            int level = getSignal(timer);
           // delay(5000);
            
            client.print("is: ");
            char rand_buf[12];
            client.print( itoa(level, rand_buf, 10));
            client.print("</br>");


            if (level>=threshold) {
          
              client.print("<br><br>Message to send (340 characters):</br>");
              
                        // The FILE_READ mode will open the file for reading.
                        File dataFile = pythonfs.open(FILE_NAME, FILE_READ);
                        if (dataFile) {
                          // File was opened, now print out data character by character until at the
                          // end of the file.
                          Serial.println("Opened file, printing contents below:");
                          while (dataFile.available()) {
                            // Use the read function to read the next character.
                            // You can alternatively use other functions like readUntil, readString, etc.
                            // See the pythonfs_full_usage example for more details.
                            char c = dataFile.read();
                            Serial.print(c);
                            client.print(c);
                          }
                          dataFile.close();
                        }
                        else {
                          Serial.println("Failed to open data file! Does it exist?");
                        }

            }
            else {

              client.print("<br>(Insufficient signal to broadcast. Reload page to reevaluate signal.)<br>");
              
            }
                    client.print("<br>Click <a href=\"/H\">here</a> turn the LED on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED off<br>");

                    }
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}


// extract text from the incoming HTTP GET data string
// returns true only if text was received
// the string must start with "&txt=" and end with "&end"
// if the string is too long for the HTTP_req buffer and
// "&end" is cut off, then the function returns false
boolean GetText(char *txt, int len)
{
  boolean got_text = false;    // text received flag
  char *str_begin;             // pointer to start of text
  char *str_end;               // pointer to end of text
  int str_len = 0;
  int txt_index = 0;
  
  // get pointer to the beginning of the text
  str_begin = strstr(HTTP_req, "&txt=");
  if (str_begin != NULL) {
    str_begin = strstr(str_begin, "=");  // skip to the =
    str_begin += 1;                      // skip over the =
    str_end = strstr(str_begin, "&end");
    if (str_end != NULL) {
      str_end[0] = 0;  // terminate the string
      str_len = strlen(str_begin);

      // copy the string to the txt buffer and replace %20 with space ' '
      for (int i = 0; i < str_len; i++) {
        if (str_begin[i] != '%') {
          if (str_begin[i] == 0) {
            // end of string
            break;
          }
          else {
            txt[txt_index++] = str_begin[i];
            if (txt_index >= (len - 1)) {
              // keep the output string within bounds
              break;
            }
          }
        }
        else {
          // replace %20 with a space
          if ((str_begin[i + 1] == '2') && (str_begin[i + 2] == '0')) {
            txt[txt_index++] = ' ';
            i += 2;
            if (txt_index >= (len - 1)) {
              // keep the output string within bounds
              break;
            }
          }
        }
      }
      // terminate the string
      txt[txt_index] = 0;
      got_text = true;
    }
  }

  return got_text;
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

void chirpText(char *txt, int len) {

Serial.println("Sending to satellite ...");
Serial.println(txt);

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

  

