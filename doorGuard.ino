/*
   Door Guardian System
   Created by YWJamesLin@OpenNCU on 2017.02
 */

#include <stdlib.h>
#include <string.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

#include <Keypad.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <PN532_HSU.h>
#include <snep.h>
#include <NdefMessage.h>


#define production

#ifdef production
#include "definition_prod.h"
#else
#include "definition.h"
#endif

#define COUNT_LIMIT 2000

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'F', 'E', 'D', 'C'},
  {'B', '3', '6', '9'},
  {'A', '2', '5', '8'},
  {'0', '1', '4', '7'}
};
byte rowPins[ROWS] = {4, 5, 6, 7}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {0, 1, 2, 3}; //connect to the column pinouts of the keypad

Keypad_I2C kpd (makeKeymap(keys), rowPins, colPins, ROWS, COLS, KBI2CAddr, PCF8574);

LiquidCrystal_I2C lcd (LCDI2CAddr, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

PN532_HSU pn532hsu (Serial1);
SNEP nfc (pn532hsu);

char messages[4][16] = {DOORNAME, "Input:", "Find NFC device", "Receiving code"};
char openmsg[6] = "Open!";
char wrongmsg[6] = "Wrong";
char errormsg[6] = "Error";
char conerrmsg[17] = "Connection Error";
char timeoutmsg[8] = "TimeOut";

char input[17], request[50], reply[13], status[4], code[6];

int length, i, phase, time, count;
char key;
boolean quit;

EthernetClient client, logClient;

NdefMessage message;
NdefRecord tempRecord;
int messageSize, ndefSize, recordSize, payloadSize;
uint8_t ndefBuf[128];
byte tempArr[100];

void printStr (char* str) {
  int i;
  for (i = 0; str[i] != '\0'; ++ i) {
    lcd.print (str[i]);
  }
}

//init LCD with initMessage
void LCDInit (char* initMessage) {
  lcd.clear ();
  lcd.setCursor (0, 0);
  printStr (initMessage);
  lcd.setCursor (0, 1);
  delay (500);
}

void LCDInit () {
  lcd.clear ();
  lcd.setCursor (0, 0);
  delay (500);
}

// trigger the relay to open the entity
void triggerDoor (int doorN) {
  digitalWrite (relayTriggerPin[doorN], HIGH);
  delay (200);
  digitalWrite (relayTriggerPin[doorN], LOW);
  LCDInit (openmsg);
}

//get input from KeyPad
void keyInput () {
  int i;
  char key;
  length = 0;
  quit = false;

  count = 0;
  wdt_reset ();

  while (! quit) {
    ++ count;
    if (count == COUNT_LIMIT) {
      count = 0;
      wdt_reset ();
    }

    key = kpd.getKey();
    if (key) {
      switch (key) {
        // back to the main
        case 'F' :
          length = 0;
          quit = true;
          break;
          //Reset screen but not the existing string
        case 'E' :
          lcd.begin (16, 2);
          lcd.clear ();
          lcd.setCursor (0, 0);
          printStr (messages[phase]);
          lcd.setCursor (0, 1);
          for (i = 0; i < length; ++ i) {
            lcd.print (input[i]);
          }
          delay (500);
          break;
          //Delete (from left to right)
        case 'D' :
          if (length) {
            for (i = 1; i < length; ++ i) {
              input[i - 1] = input[i];
            }
            -- length;
            lcd.clear ();
            lcd.setCursor (0, 0);
            printStr (messages[phase]);
            lcd.setCursor (0, 1);
            for (i = 0; i < length; ++ i) {
              lcd.print (input[i]);
            }
          }
          break;
          //Clear all
        case 'C' :
          length = 0;
          LCDInit (messages[phase]);
          break;
          //Backspace (delete from right to left)
        case 'B' :
          if (length) {
            -- length;
            lcd.setCursor (length, 1);
            lcd.print (' ');
            lcd.setCursor (length, 1);
          }
          break;
          //Accept
        case 'A' :
          input[length] = '\0';
          quit = true;
          break;
          //Input digits
        default :
          if (length != 16) {
            input[length] = key;
            lcd.print(key);
            ++ length;
          }
          delay (50);
          break;
      }
    }
  }
}

// check whether input is a number string
int checkInput (char* input, int length) {
  for (int i = 0; i < length ; ++ i) {
    if (input[i] < 48 || input[i] > 57) {
      return 0;
    }
  }
  return 1;
}

// send entityUUID and receive code
void beamData () {
  int j;
  length = 0;

  count = 0;
  wdt_reset ();

  message = NdefMessage ();
  message.addMimeMediaRecord (mimeType, entityUUID);
  messageSize = message.getEncodedSize ();
  message.encode (ndefBuf);
  if (nfc.write (ndefBuf, messageSize, 5000) > 0) {
    phase = 3;
    LCDInit (messages[phase]);
    for (j = 0; j < 5; ++ j) {
      wdt_reset ();
      ndefSize = nfc.read (ndefBuf, messageSize, 2500);
      if (ndefSize > 0) {
        message = NdefMessage (ndefBuf, ndefSize);
        recordSize = message.getRecordCount ();
        for (i = 0; i < recordSize; ++ i) {
          tempRecord = message.getRecord (i);
          payloadSize = tempRecord.getPayloadLength ();
          if (payloadSize > 0) {
            tempRecord.getPayload (tempArr);
            strncpy (input, (const char *)(tempArr + payloadSize - 5), 5);
            if (checkInput (input, 5)) {
              length = 5;
              break;
            }
          }
        }
      }
      if (length == 5) {
        break;
      }
    }
  }
}

//connect to the server to verify the input
void verify () {
  count = 0;
  wdt_reset ();

  input [length] = '\0';
  client.connect (byteSrv, 80);
  if ( ! client.connected ()) {
    LCDInit (conerrmsg);
  } else {
    while (client.available ()) { client.read (); }
    snprintf (code, sizeof(code), "%s", input);
    snprintf (request, sizeof (request), "GET %s%s HTTP/1.1", query, code);
    client.println (request);
    snprintf (request, sizeof (request), "Host: %s", serverStr);
    client.println (request);

    client.println ("Connection: close");
    client.println ();
    client.flush ();
    delay (500);

    time = 0;
    while ( ! client.available () && time != 3) {
      delay (250);
      ++ time;
    }
    if ( ! client.available ()) {
      LCDInit (timeoutmsg);
    } else {
      length = 0;
      while (client.available () && length != 12) {
        reply[length] = client.read ();
        ++ length;
      }
      reply[length] = '\0';
      if (strstr (reply, "204") != NULL) {
        triggerDoor (0);
      } else if (strstr (reply, "404") != NULL) {
        LCDInit (wrongmsg);
      } else {
        LCDInit (errormsg);
      }
      client.stop ();
      length = 0;
    }
  }
}

void setup () {
  //initialize KeyPad, LCD and backlight, Ethernet
  Wire.begin ();
  kpd.begin ();
  lcd.begin (16, 2);
  lcd.backlight ();
  Ethernet.begin (mac, byteIP, byteDNS, byteGW, byteSN);

  //initalize the relay pin
  for (i = 0; i < relayNum; ++ i) {
    pinMode (relayTriggerPin[i], OUTPUT);
  }

  //start main phase
  phase = 0;
  LCDInit (messages[phase]);

  wdt_enable (WDTO_8S);
  count = 0;
}

void loop () {
  ++ count;
  if (count == COUNT_LIMIT) {
    count = 0;
    wdt_reset ();
  }

  key = kpd.getKey ();
  switch (key) {
    case 'C' :
      count = 0;
      phase = 2;
      LCDInit (messages[phase]);
      beamData ();
      break;
    case 'F' :
      count = 0;
      phase = 1;
      LCDInit (messages[phase]);
      keyInput ();
      break;
    case 'E' :
      lcd.begin (16, 2);
      lcd.clear ();
      lcd.setCursor (0, 0);
      printStr (messages[phase]);
      break;
  }
  if (key == 'C' || key == 'F') {
    if (length) { verify (); }
    phase = 0;
    LCDInit (messages[phase]);
    
    count = 0;
    wdt_reset ();
  }
}
