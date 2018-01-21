// control debug serial prints
#define DEBUG // comment this to stop debug output

#include "helmet-signal-base.h"

// pin definitions
const int brakeIn = 5;
const int leftIn = 2;
const int rightIn = 3;

// byte to send input state via radio
byte state = 0; // [ - - - - - left right brake ]

// radio configuration
RF24 radio(7,8);

void setup() {
  Serial.begin(9600);
  DEBUG_PRINTLN("resetting");

  // brake
  pinMode(brakeIn, INPUT_PULLUP);
  
  // left turn signal
  pinMode(leftIn, INPUT_PULLUP);
  
  // right turn signal
  pinMode(rightIn, INPUT_PULLUP);
  
  // debugging LED
  pinMode(13, OUTPUT);

  radio.begin();
  
  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  // Comment out for production.
  DEBUG_PRINTLN("Setting PA Level");
  radio.setPALevel(RF24_PA_LOW);

  uint8_t addresses[][6] = {"1Node", "2Node"};
  DEBUG_PRINTLN("open write pipe");
  radio.openWritingPipe(addresses[0]);
  DEBUG_PRINTLN("write pipe open");
  DEBUG_PRINTLN("open read pipe");
  radio.openReadingPipe(1,addresses[1]);
  DEBUG_PRINTLN("read pipe open");
  radio.setAutoAck(0,false); // disable ACK
  radio.stopListening();
}

void loop() {
  state = 0;
  if (!digitalRead(brakeIn)) {
    state |= 1;
  }
  if (!digitalRead(rightIn)) {
    state |= 2;
  }
  if (!digitalRead(leftIn)) {
    state |= 4;
  }

  DEBUG_PRINT("Sending ");
  DEBUG_PRINTLN(state);

  if (!radio.write(&state, sizeof(byte))) {
    DEBUG_PRINTLN("failed");
  }
  else {
    DEBUG_PRINTLN("sent");
  }
}
