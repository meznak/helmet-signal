// control debug serial prints
#define DEBUG // comment this to stop debug output

#include "helmet-signal-base.h"

// pin definitions
const int brakeIn = 5;
const int leftIn = 2;
const int rightIn = 3;

// radio configuration
RF24 radio(7,8);
RF24Network network(radio);

const uint16_t base_node = 00; // octal address
const uint16_t helmet_node = 01;

const unsigned long interval = 20;

unsigned long last_sent = 0;
unsigned long packets_sent = 0;

struct payload_t {
  unsigned long ms;
  unsigned long counter;
  int netid;
  byte state; // [ - - - - - left right brake ]
};

void setup() {
  Serial.begin(9600);
  DEBUG_PRINTLN("resetting");

  // brake
  pinMode(brakeIn, INPUT_PULLUP);
  
  // left turn signal
  pinMode(leftIn, INPUT_PULLUP);
  
  // right turn signal
  pinMode(rightIn, INPUT_PULLUP);

  SPI.begin();
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  network.begin(/*channel*/ 90, /*node address*/ base_node);
}

void loop() {
  byte state = 0;
  if (!digitalRead(brakeIn)) {
    state |= 1;
  }
  if (!digitalRead(rightIn)) {
    state |= 2;
  }
  if (!digitalRead(leftIn)) {
    state |= 4;
  }
  network.update();

  unsigned long now = millis();
  if (now - last_sent >= interval) {
    last_sent = now;

    DEBUG_PRINT("Sending...");

    DEBUG_PRINTLN(state);

    payload_t payload = { millis(), packets_sent++, NETID, state };

    RF24NetworkHeader header(/*to node*/ helmet_node);
    bool ok = network.write(header, &payload, sizeof(payload));

    if (ok)
      DEBUG_PRINTLN("ok");
    else
      DEBUG_PRINTLN("failed");
  }
}
