/*
 * this is an example of the data definition, please just modify the value to suit what you need
 */
//Predefinition
#define LCDI2CAddr 0x3f
#define KBI2CAddr 0x38
#define DOORNAME "Door Guard"

//  Ethernet Config 
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xEF, 0xED
};

//Predefinition of server info.
byte byteIP[4] = {10, 0, 10, 100};
byte byteDNS[4] = {8, 8, 8, 8};
byte byteGW[4] = {10, 0, 10, 254};
byte byteSN[4] = {255, 255, 255, 0};
byte byteSrv[4] = {25, 2, 5, 2};
char serverStr[] = "25.2.5.2";

char query[] = "/love/arrow/shoot/";
char mimeType[] = "word/something/xopowo";
char entityUUID[] = "yosoro-bubu-zura-yohane";

//multidoor feature door relay pin
const int relayNum = 1;
int relayTriggerPin[relayNum] = {9};
