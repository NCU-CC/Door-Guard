/*
   Door Guardian System
   Created by YWJamesLin@OpenNCU on 2017.02
 */

#include <stdlib.h>
#include <string.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>

#include <Keypad.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>


#define production

#ifdef production
#include "definition_prod.h"
#else
#include "definition.h"
#endif

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

char messages[2][16] = {"Door Guard", "Input:"};
char openmsg[6] = "Open!";
char wrongmsg[6] = "Wrong";
char errormsg[6] = "Error";
char conerrmsg[17] = "Connection Error";
char timeoutmsg[8] = "TimeOut";

char input[17], request[50], reply[13], status[4], code[6];

int length, i, phase, time, inpLen;
char key;
boolean quit;

EthernetClient client, logClient;

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

void triggerDoor (int doorN) {
  if (doorN > -1) {
    digitalWrite (relayTriggerPin[doorN], HIGH);
    delay (200);
    digitalWrite (relayTriggerPin[doorN], LOW);
  }
  LCDInit (openmsg);
}

//get input from KeyPad
void keyInput () {
  int i;
  length = 0;
  quit = false;
  while (! quit) {
    key = kpd.getKey();
    if (key) {
      switch (key) {
        // back to the main
        case 'F' :
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
  quit = false;
}

void verify () {
  input [length] = '\0';
  inpLen = length;
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
        if (inpLen == 5) {
          triggerDoor (0);
        } else {
          triggerDoor (input[5] - 49);
        }
        snprintf (status, sizeof (status), "204");
      } else if (strstr (reply, "404") != NULL) {
        LCDInit (wrongmsg);
        snprintf (status, sizeof (status), "404");
      } else if (strstr (reply, "500")) {
        LCDInit (errormsg);
        snprintf (status, sizeof (status), "500");
      } else {
        LCDInit (errormsg);
        snprintf (status, sizeof (status), "999");
      }
      client.stop ();
      length = 0;

      if (strcmp (status, "999") != 0) {
        logClient.connect (byteSrv, 7070);

        logClient.print ("GET /entityLog/");
        logClient.print (status);
        logClient.print ("/");
        logClient.print (code);
        logClient.println (" HTTP/1.1");
        logClient.println ();
        logClient.println ("Connection: close");
        logClient.println ();
        logClient.flush ();
        logClient.stop ();
      }
    }
  }
}

void setup() {
  //initialize KeyPad, LCD and backlight, Ethernet
  Wire.begin ();
  kpd.begin (makeKeymap (keys));
  lcd.begin (16, 2);
  lcd.backlight ();
  Ethernet.begin (mac, byteIP, byteDNS, byteGW, byteSN);

  //initalize the relay pin
  for (i = 0; i < relayNum; ++ i) {
    pinMode (relayTriggerPin[i], OUTPUT);
  }

  //start main phase
  phase = 1;
  LCDInit (messages[phase]);
}

void loop() {
  keyInput ();

  //verify input and back to the main phase
  if (length) {
    verify ();
    phase = 1;
    LCDInit (messages[phase]);
  }
}
