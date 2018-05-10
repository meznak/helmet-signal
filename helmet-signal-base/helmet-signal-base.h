#include <RF24Network.h>
#include <RF24.h>
#include "helmet-signal-base-config.h"

#ifdef DEBUG
 #define DEBUG_PRINT(x)    Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
 #define DEBUG_PRINTX(x)   Serial.print (x, HEX)
 #define DEBUG_PRINTXLN(x)   Serial.println (x, HEX)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTX(x)
 #define DEBUG_PRINTXLN(x)
#endif
