/*
  --------------SD Card----------------
  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

  or we can use following connections from ICSP of Arduino
  Arduino ICSP Pin Configuration:
     -------
  MISO| O   O |VCC
  SCK | O   O |MOSI
  RST | O   O |GND
     -------
  SD Card ------- Arduino
  VCC  ---------- VCC
  GND  ---------- GND
  MISO ---------- MISO
  MOSI ---------- MOSI
  SCK  ---------- SCK
  CS   ---------- 4

  -----------Keypad------------------

Keypad ---- Arduino
  1 --------- 5 COLUMN-4
  2 --------- 6 COLUMN-3
  3 --------- 7 COLUMN-2
  4 --------- 8 COLUMN-1
  5 --------- 9 ROW-4
  6 --------- 10 ROW-3
  7 --------- 11 ROW-2
  8 --------- 12 ROW-1

  -----------Bluetooth---------------
  Bluetoth -- Arduino
    TX ------ RX1
    RX ------ TX1

  -----------GSM & GPRS Module-------------
  Printer -- Arduino
    TX ------ RX2
    RX ------ TX2

  -----------Thermal Printer-------------
  Printer -- Arduino
    TX ------ RX3
    RX ------ TX3
*/
#define TINY_GSM_MODEM_SIM808

#include <SPI.h>
#include <SD.h>
//#include <SoftwareSerial.h>
#include "Adafruit_Thermal.h"
#include <Keypad.h>

// Increase RX buffer
//#define TINY_GSM_RX_BUFFER 512

// Use Hardware Serial on Mega, Leonardo, Micro
//SoftwareSerial bt(2, 3); // RX, TX for Bluetooth
//SoftwareSerial thermal(5, 6); // RX, TX for Thermal Printer
#define bt Serial1         //for Bluetooth
#define gprs Serial2       //for GPRS modem
#define thermal Serial3    //for thermal printer
Adafruit_Thermal printer(&thermal);  



// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "www";
const char user[] = "";
const char pass[] = "";

// Name of the server we want to connect to
const char server[] = "dataspyer.000webhostapp.com";
const int  port     = 80; //for http:80 https:443
const char resource[] = "/index.php";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>


TinyGsm modem(gprs);
TinyGsmClient client(modem); //http
//TinyGsmClientSecure client(modem); //https
HttpClient http(client, server, port);

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  //{'A'}, {'B'}, {'C'}, {'D'}
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {12, 11, 10, 9}; //{13,11,10,9}connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6, 5}; //{8}//connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

File myFile;

String inputString = "";         // a String to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  printer.begin();        // Init printer (same regardless of serial type)
  
  // reserve 200 bytes for the inputString:
  inputString.reserve(1000);

/****************************SD Card Initialization***********************/

  bt.print("Initializing SD card...");
  if (!SD.begin(4)) {
    bt.println("initialization failed!");
    return;
  }
  bt.println("initialization done.");

/***************************GPRS Initialization**************************/

  /*bt.println("Initializing modem...");
  modem.init();

  String modemInfo = modem.getModemInfo();
  bt.print("Modem: ");
  bt.println(modemInfo);

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");
  bt.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    bt.println(" fail");
    delay(1000);
    return;
  }
  bt.println(" OK");

  bt.print(F("Connecting to "));
  bt.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    bt.println(" fail");
    delay(1000);
    return;
  }
  bt.println(" OK");*/

}

void loop() {

  if (stringComplete) {
    char customKey = customKeypad.getKey();
    switch (customKey) {
      case 'A' :     //Press 'A' to send data to Bluetooth
        {
          bt.println(inputString);
          break;
        }
      case 'B' :    //Press 'B' to send data to SD Card
        {
          myFile = SD.open("test.txt", FILE_WRITE);
          if (myFile) {
            myFile.println(inputString);
            myFile.close();
            bt.println(F("SD card writing done"));
          }
          else
            bt.println(F("error opening test.txt")); // if the file didn't open, print an error:
          break;
        }
      case 'C' :      //Press 'C' to send data to Printer
        {
          printer.println(inputString);
          printer.feed(2);
          printer.sleep();      // Tell printer to sleep
          delay(3000L);         // Sleep for 3 seconds
          printer.wake();       // MUST wake() before printing again, even if reset
          printer.setDefault(); // Restore printer to defaults
          break;
        }
      case 'D' :    //Press 'D' to send data to Client Server
        {
          bt.println("making POST request");
          http.connectionKeepAlive();
          const String contentType = "application/x-www-form-urlencoded";
          String postData = "string=" + inputString;
          //http.post(resource, contentType, postData);
          int err = http.post(resource, contentType, postData);
          if (err != 0) {
            bt.println("failed to connect");
            delay(10000);
            return;
          }
          int status = http.responseStatusCode();
          bt.println(status);
          if (!status) {
            delay(10000);
            return;
          }
          String http_body = http.responseBody();
          bt.println("Response:");
          bt.println(http_body);
          //bt.println(String("Body length is: ") + http_body.length());
          http.stop();
          break;
        }
      case '1' :   //Press any other letter to clear the string
        {
          inputString = "";// clear the string:
          stringComplete = false;
          bt.println(F("Loop exiting"));
          break;
        }
    }
  }
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '$') {
      stringComplete = true;
    }
    else
      inputString += inChar;         // add it to the inputString:
  }
}
