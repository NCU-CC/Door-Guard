/*
  Door Guardian System
  2016.07.22 YWJamesLin@OpenNCU
*/
#include <stdlib.h>
#include <string.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>

#include "definition.h"
#include <IPTransformer.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>

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

char key;
char messages[5][10] = {"INPUT:", "IP:", "DNS:", "GATEWAY:", "SUBNET:"};
char input[17];
char reply[200];
int length, i, phase, time;
boolean quit;

IPTransformer IPT;

int newIP[4], newDNS[4], newGateway[4], newSubnet[4], newServer[4];

IPAddress *ip, *dnsAddr, *gateway, *subnet, *server;

EthernetClient client;

char request[100] = "";
char host[100] = "";

void triggerDoor () {
  digitalWrite (relayTriggerPin, HIGH);
  delay (200);
  digitalWrite (relayTriggerPin, LOW);
}

void LCDInit (char* initMessage) {
  lcd.clear ();
  lcd.setCursor (0, 0);
  lcd.println (initMessage);
  lcd.setCursor (0, 1);
  length = 0;
  delay (500);
}

void setup() {
  IPT.IPStrToInt (newIP, ipStr);
  IPT.IPStrToInt (newDNS, dnsStr);
  IPT.IPStrToInt (newGateway, gwStr);
  IPT.IPStrToInt (newSubnet, snStr);
  IPT.IPStrToInt (newServer, srvStr);

  ip = new IPAddress(newIP[0], newIP[1], newIP[2], newIP[3]);
  dnsAddr = new IPAddress (newDNS[0], newDNS[1], newDNS[2], newDNS[3]);
  gateway = new IPAddress (newGateway[0], newGateway[1], newGateway[2], newGateway[3]);
  subnet = new IPAddress (newSubnet[0], newSubnet[1], newSubnet[2], newSubnet[3]);
  server = new IPAddress (newServer[0], newServer[1], newServer[2], newServer[3]);

Wire.begin ();
  kpd.begin (makeKeymap (keys));
  Serial.begin (115200);
  Ethernet.begin (mac, *ip, *dnsAddr, *gateway, *subnet);
  lcd.begin (16, 2);
  lcd.backlight ();
  pinMode (relayTriggerPin, OUTPUT);
}

void keyInput () {
  quit = false;
  while (! quit) {
    key = kpd.getKey();
    if (key) {
      switch (key) {
        case 'F' :
          lcd.print ('.');
          input[length] = '.';
          ++ length;
          break;
        case 'E' :
          lcd.begin (16, 2);
          lcd.clear ();
          lcd.setCursor (0, 0);
          lcd.println (messages[phase]);
          lcd.setCursor (0, 1);
          for(i = 0; i < length; ++ i) {
            lcd.print (input[i]);
          }
          delay (500);
          break;
        case 'D' :
          break;
        case 'C' :
          LCDInit (messages[phase]);
          break;
        case 'B' :
          if (length) {
            length -= 1;
            lcd.setCursor (length, 1);
            lcd.print (' ');
            lcd.setCursor (length, 1);
          }
          break;
        case 'A' :
          input[length] = '\0';
          quit = true;
          break;
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

void loop() {
  // INPUT Password Phase
  phase = 0;
  LCDInit (messages[phase]);
  keyInput ();
  if (strcmp (input, pwd) == 0) {
    //Input IP phase
    phase = 1;
    quit = false;
    while (! quit) {
      LCDInit (messages[phase]);
      keyInput ();
      if (IPT.IPCheck (input)) {
        IPT.IPStrToInt (newIP, input);
        quit = true;
        for (i = 0; i < 4; ++ i) {
          if ( ! IPT.IPRangeCheck (newIP[i])) {
            quit = false;
            break;
          }
        }
      }
    }

    phase = 2;
    quit = false;
    while (! quit) {
      LCDInit (messages[phase]);
      keyInput ();
      if (IPT.IPCheck (input)) {
        IPT.IPStrToInt (newDNS, input);
        quit = true;
        for (i = 0; i < 4; ++ i) {
          if ( ! IPT.IPRangeCheck (newDNS[i])) {
            quit = false;
            break;
          }
        }
      }
    }

    phase = 3;
    quit = false;
    while (! quit) {
      LCDInit (messages[phase]);
      keyInput ();
      if (IPT.IPCheck (input)) {
        IPT.IPStrToInt (newGateway, input);
        quit = true;
        for (i = 0; i < 4; ++ i) {
          if ( ! IPT.IPRangeCheck (newGateway[i])) {
            quit = false;
            break;
          }
        }
      }
    }

    phase = 4;
    quit = false;
    while (! quit) {
      LCDInit (messages[phase]);
      keyInput ();
      if (IPT.IPCheck (input)) {
        IPT.IPStrToInt (newSubnet, input);
        quit = true;
        for (i = 0; i < 4; ++ i) {
          if ( ! IPT.IPRangeCheck (newSubnet[i])) {
            quit = false;
            break;
          }
        }
      }
    }

    free (ip);
    free (dnsAddr);
    free (gateway);
    free (subnet);

    ip = new IPAddress (newIP[0], newIP[1], newIP[2], newIP[3]);
    dnsAddr = new IPAddress (newDNS[0], newDNS[1], newDNS[2], newDNS[3]);
    gateway = new IPAddress (newGateway[0], newGateway[1], newGateway[2], newGateway[3]);
    subnet = new IPAddress (newSubnet[0], newSubnet[1], newSubnet[2], newSubnet[3]);

    Ethernet.begin (mac, *ip, *dnsAddr, *gateway, *subnet);
  } else {
    if (client.connect (*server, 80)) {
      sprintf (request, "GET %s%s HTTP/1.0", query, input);
      client.println (request);
      sprintf (host, "Host: %s", srvStr);
      client.println (host);
      client.println ("Connection: close");
      client.println ();
    }
    sprintf (reply, "");
    length = 0;
    while (client.connected () && length != 199) {
      key = client.read ();
      reply[length] = key;
      ++ length;
    }
    reply[length] = '\0';
    if (strstr (reply, "204") != NULL) {
      triggerDoor ();
    }
    client.stop ();
  }
}
