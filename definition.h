//Predefinition
#define relayTriggerPin 25252
#define LCDI2CAddr 0x27
#define KBI2CAddr 0x38

//Password to setup IP
char pwd[] = "25252";


//Ethernet Config 
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
char ipStr[] = "10.0.10.100";
char dnsStr[] = "8.8.8.8";
char gwStr[] = "10.0.10.254";
char snStr[] = "255.255.255.0";

char srvStr[] = "";
char query[] = "/query/love/arrow/xxxxx/";
