#include <Bounce2.h>
#include "notes.h"

// Pinout set for ESP32-C3 DevkitC
// Connect with S2 Kaluga via UART:
//   C3   | Kaluga
// RX1 18 | 19 TX
// TX1 19 | 20 RX

#define YELLOW_LED 0
#define BLUE_LED 1
#define RED_LED 2
#define GREEN_LED 3

#define YELLOW_BUTTON 4
#define BLUE_BUTTON 5
#define RED_BUTTON 6
#define GREEN_BUTTON 7

// GPIO 8 is for RGD LED on board

#define BUZZER_PIN 9

#define MAX_GAME_SEQUENCE 800

Bounce debouncerRed = Bounce();
Bounce debouncerGreen = Bounce();
Bounce debouncerBlue = Bounce();
Bounce debouncerYellow = Bounce();


unsigned short gameSequence[MAX_GAME_SEQUENCE];

bool gameInProgress = false;
bool attractLEDOn = false;
bool showingSequenceToUser;
unsigned short currentDelay;
unsigned short currentSequenceLength;
unsigned short userPositionInSequence;
unsigned short n;
unsigned long count = 0;
uint8_t level = 0; // game level to be tracked on TFT display

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  debouncerRed.attach(RED_BUTTON, INPUT_PULLUP);
  debouncerRed.interval(30);
  debouncerGreen.attach(GREEN_BUTTON, INPUT_PULLUP);
  debouncerGreen.interval(30);
  debouncerBlue.attach(BLUE_BUTTON, INPUT_PULLUP);
  debouncerBlue.interval(30);
  debouncerYellow.attach(YELLOW_BUTTON, INPUT_PULLUP);
  debouncerYellow.interval(30);

  pinMode(BUZZER_PIN, OUTPUT);

  Serial1.begin(115200);
  while(!Serial1){delay(100);}

/*
  //demonstration
  uint8_t level = 0;
  do{
    Serial1.write(level++);
    delay(100);
  }while(level <= 25);
  Serial1.write(0);
  */
}

void loop() {
  if (! gameInProgress) {
    // Waiting for someone to press the green button...
    debouncerGreen.update();

    if (debouncerGreen.fell()) {
      digitalWrite(GREEN_LED, HIGH);
      
      // Create a new game sequence.
      randomSeed(analogRead(0));

      for (n = 0; n < MAX_GAME_SEQUENCE; n++) {
        gameSequence[n] = random(RED_BUTTON, YELLOW_BUTTON + 1);
      }

      currentSequenceLength = 1;
      currentDelay = 500;

      gameInProgress = true;
      count = 0;
      showingSequenceToUser = true;

      // Little delay before the game starts.
      delay(1000);
    } else {
      // Attract mode - flash the green LED.
      if (count == 50000) {
        attractLEDOn = ! attractLEDOn;
        digitalWrite(GREEN_LED, attractLEDOn ? LOW : HIGH);
        count = 0;    
      } 
      
      count++;
    }
  } else {
    // Game is in progress...
    if (showingSequenceToUser) {
      // Play the pattern out to the user
      for (n = 0; n < currentSequenceLength; n++) {
        digitalWrite(gameSequence[n] - 4, LOW);
        delay(currentDelay);
        digitalWrite(gameSequence[n] - 4, HIGH);
        delay(currentDelay);
      }

      showingSequenceToUser = false;
      userPositionInSequence = 0;
    } else {
      // Waiting for the user to repeat the sequence back
      debouncerGreen.update();
      debouncerRed.update();
      debouncerBlue.update();
      debouncerYellow.update();

      unsigned short userPressed = 0;

      if (debouncerGreen.fell()) {
        digitalWrite(GREEN_LED, LOW);
        delay(currentDelay);
        digitalWrite(GREEN_LED, HIGH);
        userPressed = GREEN_BUTTON;
      } else if (debouncerRed.fell()) {
        digitalWrite(RED_LED, LOW);
        delay(currentDelay);
        digitalWrite(RED_LED, HIGH);
        userPressed = RED_BUTTON;
      } else if (debouncerBlue.fell()) {
        digitalWrite(BLUE_LED, LOW);
        delay(currentDelay);
        digitalWrite(BLUE_LED, HIGH);
        userPressed = BLUE_BUTTON;
      } else if (debouncerYellow.fell()) {
        digitalWrite(YELLOW_LED, LOW);
        delay(currentDelay);
        digitalWrite(YELLOW_LED, HIGH);
        userPressed = YELLOW_BUTTON;
      }

      if (userPressed > 0) {
        // A button was pressed, check it against current sequence...
        if (userPressed != gameSequence[userPositionInSequence]) {
          // Failed...
          tone(BUZZER_PIN, NOTE_F3, 300);
          //
          delay(300);
          tone(BUZZER_PIN, NOTE_G3, 500);
          delay(2500);
          gameInProgress = false;
        } else {
          userPositionInSequence++;
          if (userPositionInSequence == currentSequenceLength) {
            // User has successfully repeated back the sequence so make it one longer...
            currentSequenceLength++;

            // There's no win scenario here, so just reset...
            if (currentSequenceLength >= MAX_GAME_SEQUENCE) {
              gameInProgress = false;
            }

            // Play a tone...
            tone(BUZZER_PIN, NOTE_A3, 300);
            delay(2000);

            showingSequenceToUser = true;
          }
        }
      }
    }
  }
}
