// control debug serial prints
#define DEBUG // comment this to stop debug output

#include "helmet-signal-helmet.h"

// led info
#define NUM_LEDS 40
#define BAR_HALF 4 // half the width of the center section
#define POLL_FREQ_IN_LEDS 2 // check radio after changing n LEDs
#define DATA_PIN 9
#define LDR_PIN A6

// LED array
CRGB leds[NUM_LEDS];
const int center = NUM_LEDS / 2;
const int leftMax = center - BAR_HALF - 1;
const int rightMin = center + BAR_HALF;
int leftLeds[center - BAR_HALF];
int rightLeds[center - BAR_HALF];

// byte to receive input state via radio
byte state = 0; // [ - - - - - left right brake ]

// radio configuration
RF24 radio(7,8);
RF24Network network(radio);
const uint16_t base_node = 00; // octal address
const uint16_t helmet_node = 01;

struct payload_t {
  unsigned long ms;
  unsigned long counter = 0;
  int netid;
  byte state; // [ - - - - - left right brake ]
};

unsigned long previousMillis = 0;
unsigned long brakeMillis = 0;

const long turnInterval = 20;
const long brakeFlashInterval = 50;
const long brakeCylonInterval = 20;
const long brakeCylonWait = 4000;

const int brakeFlashes = 8;
const int brakeCylonCycles = 4;

int yellowBrightness;
int redBrightness;
int oldBrightness[9] = {60, 60, 60, 60, 60, 60, 60, 60, 60};

const int brakeBit = 1;
const int rightBit = 2;
const int leftBit = 4;

int brakeState = LOW;
int brakeCylonState = LOW;
int leftState = LOW;
int rightState = LOW;

void setup() {
  Serial.begin(9600);
  DEBUG_PRINTLN("resetting");
  
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);

  DEBUG_PRINT("leftMax: ");
  DEBUG_PRINTLN(leftMax);
  DEBUG_PRINT("rightMin: ");
  DEBUG_PRINTLN(rightMin);

  // store LED positions
  DEBUG_PRINTLN("initialize leds arrays");
  for (int i=0; i < center - BAR_HALF; i++) {
    leftLeds[i] = leftMax - i;
    rightLeds[i] = rightMin + i;

    DEBUG_PRINT("left ");
    DEBUG_PRINTLN(leftLeds[i]);
    DEBUG_PRINT("right ");
    DEBUG_PRINTLN(rightLeds[i]);
  }

  DEBUG_PRINTLN("starting SPI...");
  SPI.begin();
  DEBUG_PRINTLN("starting radio...");
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  DEBUG_PRINTLN("starting network...");
  network.begin(/*channel*/ 90, /*node address*/ helmet_node);
}

void loop() {
  setBrightness();
  getState();
  mainloop();
}

int setBrightness() {
  int sensorValue = analogRead(LDR_PIN);
  int newBrightness = sensorValue / 1024.0 * 127;
  DEBUG_PRINT(sensorValue);
  DEBUG_PRINT(" --> ");
  DEBUG_PRINT(newBrightness);

  if (newBrightness < 20)
    newBrightness = 20;
  if (newBrightness > 127)
    newBrightness = 127;

  // average last 10 readings
  int lastBrightness = newBrightness;
  for (int i = 0; i < 9; i++) {
    newBrightness += oldBrightness[i];
    oldBrightness[i] = lastBrightness;
  }
  newBrightness /= 10;
  DEBUG_PRINT(" --> ");
  DEBUG_PRINTLN(newBrightness);

  yellowBrightness = newBrightness;
  redBrightness = newBrightness * 2;
  lastBrightness = newBrightness;

  LEDS.setBrightness(yellowBrightness);
}

void getState() {
  network.update();

  while (network.available()) {
    RF24NetworkHeader header;
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    
    if (payload.netid == NETID) {
      state = payload.state;

      DEBUG_PRINT("Received packet #");
      DEBUG_PRINT(payload.counter);
      DEBUG_PRINT(" at ");
      DEBUG_PRINT(payload.ms);
      DEBUG_PRINT(" -- ");
      DEBUG_PRINTLN(state);
    }
  }
}

void mainloop() {
  unsigned long currentMillis = millis();
  
  if (state & brakeBit) {
    LEDS.setBrightness(redBrightness);
    brake();
  }
  else {
    if (brakeState == HIGH) {
      brakeState = LOW;
      LEDS.setBrightness(yellowBrightness);
      setAll(CRGB::Yellow, false);
    }
  }

  if (currentMillis - previousMillis >= turnInterval) {
    // save the time of the last blink
    previousMillis = currentMillis;

    if (state & leftBit) {
      // flash left
      DEBUG_PRINTLN("received LEFT state");
      left();
    }
    else if (state & rightBit) {
      // flash right
      DEBUG_PRINTLN("received RIGHT state");
      right();
    }
    else if (state & brakeBit) {
      DEBUG_PRINTLN("received BRAKE state");
      brake();
    }
    else {
      setAll(CRGB::Yellow, false);
    }
    
    FastLED.show();
  }
}

void doSignal(int side[], int *signalState) {
  DEBUG_PRINT("blink ");
  for (int i = 0; i < center - BAR_HALF; i++) {
    DEBUG_PRINT(side[i]);
    DEBUG_PRINT(" ");

    if (i % POLL_FREQ_IN_LEDS == 0)
      getState();
    if (state & !(leftBit | rightBit))
      break;

    if (*signalState == HIGH) {
      if (brakeState == LOW) {
        // leave if brake goes high
        if (state & brakeBit) {
          return;
        }
        leds[side[i]] = CRGB::Black;
      }
      else {
        leds[side[i]] = CRGB::Red;
      }
    }
    else {
      leds[side[i]] = CRGB::Yellow;
    }
    
    FastLED.show();
    delay(turnInterval);
  }
  DEBUG_PRINTLN();
}

void setAll(CRGB::HTMLColorCode color, bool includeCenter) {
//  DEBUG_PRINTLN("all");
  for (int i = 0; i < NUM_LEDS; i++) {
    if (!includeCenter && i > leftMax && i < rightMin)
      leds[i] = CRGB::Red;
    else
      leds[i] = color;
  }
  FastLED.show();
}

void left() {
  DEBUG_PRINTLN("left");
  leftState = !leftState;
    
  doSignal(leftLeds, &leftState);
}

void right() {
  DEBUG_PRINTLN("right");
  rightState = !rightState;
  
  doSignal(rightLeds, &rightState);
}

void brake() {
  DEBUG_PRINTLN("brake");
  
  if (brakeState == LOW) {
    // brake was just applied. flash like a mofo!
    DEBUG_PRINT("bflash ");
    brakeState = HIGH;
    brakeMillis = millis();
    
    for (int cycle = 0; cycle < brakeFlashes; cycle++) {
      DEBUG_PRINT(cycle);

      setAll(CRGB::Red, true);
      delay(brakeFlashInterval);
      setAll(CRGB::Black, true);
      delay(brakeFlashInterval);
      setAll(CRGB::Red, true);
    }
    DEBUG_PRINTLN();
  }
  else if (state & !(leftBit | rightBit)) {
    setAll(CRGB::Red, true);
  }
  
  if (millis() - brakeMillis > brakeCylonWait) {
    for (int cycle = 0; cycle < brakeCylonCycles; cycle++) {
      if (!(state & brakeBit) | state & (leftBit | rightBit)) // break on no brake or doSignal
        break;
      cylon(true);
      cylon(false);
    }
    
    setAll(CRGB::Red, true);
    brakeMillis = millis();
  }
}

void fadeall() {
  for(int i = 0; i < NUM_LEDS; i++) {
    if (i <= leftMax || i >= rightMin)
      leds[i].nscale8(200);
  }
}

void cylon(bool count_up) {
  if (count_up) {
    for (int i = 0; i < center - BAR_HALF; i++) {
      if (cylon_step(i) == 1)
        break;
    }
  } else {
    for (int i = center - BAR_HALF; i >= 0; i--) {
      if (cylon_step(i) == 1)
        break;
    }
  }
}

int cylon_step(int i) {
    DEBUG_PRINT("cylon ");
    DEBUG_PRINTLN(i);

    if (i % POLL_FREQ_IN_LEDS == 0)
      getState();
    if (!(state & brakeBit) | state & (leftBit | rightBit)) {
      // break on no brake or doSignal
      setAll(CRGB::Red, false);
      return 1;
    }

    leds[leftLeds[i]] = CHSV(0, 255, 255);
    leds[rightLeds[i]] = CHSV(0, 255, 255);
    FastLED.show();
    fadeall();
    delay(brakeCylonInterval);

  return 0;
}
