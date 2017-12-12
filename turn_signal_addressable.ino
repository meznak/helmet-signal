#include "FastLED.h"
#include "turn_signal_addressable.h"

// led info
#define NUM_LEDS 40
#define DATA_PIN 12

// pin definitions
#define brakeIn 5
#define leftIn 2
#define rightIn 3

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

int brakeState = LOW;
int brakeCylonState = LOW;
int leftState = LOW;
int rightState = LOW;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  Serial.println("resetting");
  
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
  LEDS.setBrightness(50);
  
  // brake
  pinMode(brakeIn, INPUT_PULLUP);
  
  // left turn signal
  pinMode(leftIn, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(leftIn), left, LOW);
  
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
    if (brakeState == LOW) {
      // brake was just applied. flash like a mofo!
      Serial.print("brake ");
      brakeState = HIGH;
      brakeMillis = millis();
      
      for (int cycle = 0; cycle < brakeFlashes; cycle++) {
        Serial.print(cycle);

        setAll(CRGB::Black);
        delay(brakeFlashInterval);
        setAll(CRGB::Red);
        delay(brakeFlashInterval);
      }
      Serial.println();
    }
  }
  else {
    if (brakeState == HIGH) {
      brakeState = LOW;
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

void signal(int side[], int *state) {
  Serial.print("blink ");
  for (int i = 0; i < NUM_LEDS / 2; i++) {
    Serial.print(side[i]);
    if (*state == HIGH) {
      if (brakeState == LOW) {
        if (!digitalRead(brakeIn)) {
          break;
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
  Serial.println();
}

void setAll(CRGB::HTMLColorCode color) {
  Serial.println("all");
  for (int i = 0; i < NUM_LEDS; i++) {
    if (brakeState == HIGH)
      leds[i] = CRGB::Red;
    else
      leds[i] = color;
  }
  FastLED.show();
}

void left() {
  Serial.println("left");
  leftState = !leftState;
    
  signal(leftLeds, &leftState);
}

void right() {
  Serial.println("right");
  rightState = !rightState;
  
  signal(rightLeds, &rightState);
}

void brake() {
  Serial.println("brake");
  setAll(CRGB::Red);
  if (millis() - brakeMillis > brakeCylonWait) {
    for (int cycle = 0; cycle < brakeCylonCycles; cycle++) {
      if (digitalRead(brakeIn) || !digitalRead(leftIn) || !digitalRead(rightIn)) // break on no brake or signal
        break;
      cylon();
    }
    
    brakeMillis = millis();
  }
  Serial.println("end brake");
}

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(200); } }

void cylon() {
  for (int i = 0; i < NUM_LEDS; i++) {
    Serial.println("cylon >");
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
    Serial.println("cylon <");
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
