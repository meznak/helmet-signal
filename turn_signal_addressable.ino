#include "FastLED.h"
#include "turn_signal_addressable.h"

// control debug serial prints
//#define DEBUG // comment this to stop debug output
#ifdef DEBUG
 #define DEBUG_PRINT(x)    Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
#endif

// led info
#define NUM_LEDS 40
#define DATA_PIN 12

// pin definitions
const int brakeIn = 5;
const int leftIn = 2;
const int rightIn = 3;

// LED array
CRGB leds[NUM_LEDS];
int leftLeds[NUM_LEDS / 2];
int rightLeds[NUM_LEDS / 2];

unsigned long previousMillis = 0;
unsigned long brakeMillis = 0;

const long turnInterval = 30;
const long brakeFlashInterval = 50;
const long brakeCylonInterval = 20;
const long brakeCylonWait = 4000;

const int brakeFlashes = 8;
const int brakeCylonCycles = 4;

const int yellowBrightness = 60;
const int redBrightness = 100;

int brakeState = LOW;
int brakeCylonState = LOW;
int leftState = LOW;
int rightState = LOW;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  DEBUG_PRINTLN("resetting");
  
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
  LEDS.setBrightness(yellowBrightness);
  
  // brake
  pinMode(brakeIn, INPUT_PULLUP);
  
  // left turn signal
  pinMode(leftIn, INPUT_PULLUP);
  
  // right turn signal
  pinMode(rightIn, INPUT_PULLUP);
  
  // debugging LED
  pinMode(13, OUTPUT);

  for (int i=0; i < NUM_LEDS / 2; i++) {
    leftLeds[i] = NUM_LEDS / 2 + i;
  }
  for (int i=NUM_LEDS / 2 - 1; i >= 0; i--) {
    rightLeds[i] = NUM_LEDS / 2 - 1 - i;
  }
}

// the loop function runs over and over again forever
void loop() {
  unsigned long currentMillis = millis();
  
  // inputs are active-low!!!
  if (!digitalRead(brakeIn)) {
    LEDS.setBrightness(redBrightness);
    brake();
  }
  else {
    if (brakeState == HIGH) {
      brakeState = LOW;
      LEDS.setBrightness(yellowBrightness);
      setAll(CRGB::Yellow);
    }
  }

  if (currentMillis - previousMillis >= turnInterval) {
    // save the time of the last blink
    previousMillis = currentMillis;

    if (!digitalRead(leftIn)) {
      // flash left
      left();
    }
    else if (!digitalRead(rightIn)) {
      // flash right
      right();
    }
    else if (!digitalRead(brakeIn)) {
      brake();
    }
    else {
      setAll(CRGB::Yellow);
    }
    
    FastLED.show();
  }
}

void signal(int side[], int *state, const int *pin) {
  DEBUG_PRINT("blink ");
  for (int i = 0; i < NUM_LEDS / 2; i++) {
    DEBUG_PRINT(side[i]);

    if (digitalRead(*pin))
      break;

    if (*state == HIGH) {
      if (brakeState == LOW) {
        if (!digitalRead(brakeIn)) {
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

void setAll(CRGB::HTMLColorCode color) {
  DEBUG_PRINTLN("all");
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void left() {
  DEBUG_PRINTLN("left");
  leftState = !leftState;
    
  signal(leftLeds, &leftState, &leftIn);
}

void right() {
  DEBUG_PRINTLN("right");
  rightState = !rightState;
  
  signal(rightLeds, &rightState, &rightIn);
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

      setAll(CRGB::Red);
      delay(brakeFlashInterval);
      setAll(CRGB::Black);
      delay(brakeFlashInterval);
      setAll(CRGB::Red);
    }
    DEBUG_PRINTLN();
  }
  else if (digitalRead(leftIn) && digitalRead(rightIn)) {
    setAll(CRGB::Red);
  }
  
  if (millis() - brakeMillis > brakeCylonWait) {
    for (int cycle = 0; cycle < brakeCylonCycles; cycle++) {
      if (digitalRead(brakeIn) || !digitalRead(leftIn) || !digitalRead(rightIn)) // break on no brake or signal
        break;
      cylon();
    }
    
    setAll(CRGB::Red);
    brakeMillis = millis();
  }
  DEBUG_PRINTLN("end brake");
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(200); } }

void cylon() {
  for (int i = 0; i < NUM_LEDS; i++) {
    DEBUG_PRINTLN("cylon >");
    if (digitalRead(brakeIn) || !digitalRead(leftIn) || !digitalRead(rightIn)) { // break on no brake or signal
      setAll(CRGB::Red);
      break;
    }
    leds[i] = CHSV(0, 255, 255);
    FastLED.show();
    fadeall();
    delay(brakeCylonInterval);
  }
  
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    DEBUG_PRINTLN("cylon <");
    if (digitalRead(brakeIn) || !digitalRead(leftIn) || !digitalRead(rightIn)) { // break on no brake or signal
      setAll(CRGB::Red);
      break;
    }
    leds[i] = CHSV(0, 255, 255);
    FastLED.show();
    fadeall();
    delay(brakeCylonInterval);
  }
}
