#include <Bounce2.h>
#include "tones.h"
#include "I2S.h"
#include "start_1.c"
#include "lvl_up_1.c"
#include "fail_1.c"

// Pinout set for ESP32-C3 DevkitC
// Connect with S2 Kaluga via UART:
//   C3   | Kaluga
// RX1 18 | 19 TX
// TX1 19 | 20 RX

#define RED_LED 4
#define GREEN_LED 5
#define BLUE_LED 6
#define YELLOW_LED 7

#define RED_BUTTON 10
#define GREEN_BUTTON 1
#define BLUE_BUTTON 2
#define YELLOW_BUTTON 3

//for use with gameSequence[n]
uint8_t button_pin_map[4] = {RED_BUTTON, GREEN_BUTTON, BLUE_BUTTON, YELLOW_BUTTON};
char button_text_map[4][14] = {"RED_BUTTON", "GREEN_BUTTON", "BLUE_BUTTON", "YELLOW_BUTTON"};
uint8_t led_pin_map[4] = {RED_LED, GREEN_LED, BLUE_LED, YELLOW_LED};
char led_text_map[4][14] = {"RED_LED", "GREEN_LED", "BLUE_LED", "YELLOW_LED"};

enum command {RESTART, STARTUP, LEVEL_UP, FAILED};
enum buzzer_msg {PLAY_START, PLAY_SUCCESS, PLAY_LVL_UP, PLAY_FAIL};

// GPIO 8 is for RGD LED on board

#define BUZZER_PIN 9 // PDM

#define MAX_GAME_SEQUENCE 30
#define FLASH_DELAY 250

Bounce debouncerRed = Bounce();
Bounce debouncerGreen = Bounce();
Bounce debouncerBlue = Bounce();
Bounce debouncerYellow = Bounce();

unsigned short gameSequence[MAX_GAME_SEQUENCE];

bool gameInProgress = false;
bool attractLEDOn = false;
bool showingSequenceToUser;
unsigned short currentSequenceLength;
unsigned short userPositionInSequence;
unsigned short n;
unsigned long flash_timer;
QueueHandle_t buzzer_queue;

void led_on(uint8_t pin){
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void all_leds_on(){
  led_on(RED_LED);
  led_on(GREEN_LED);
  led_on(BLUE_LED);
  led_on(YELLOW_LED);
}

void led_off(uint8_t pin){
  pinMode(pin, INPUT);
}

void all_leds_off(){
  led_off(RED_LED);
  led_off(GREEN_LED);
  led_off(BLUE_LED);
  led_off(YELLOW_LED);
}

void update_buttons(){
  debouncerRed.update();
  debouncerGreen.update();
  debouncerBlue.update();
  debouncerYellow.update();
}

void fail_strobe(){
  for(int i = 0; i < 10; ++i){
    all_leds_on();
    delay(75);
    all_leds_off();
    delay(75);
  }
}

void play_start(){
  log_d("play start (but not really)");
  delay(2000);
  /*
  if(I2S.begin(PDM_MONO_MODE, start_1_sampling_frequency, start_1_bits_per_sample)){
    I2S.setDataInPin(BUZZER_PIN);
    I2S.write(start_1_samples, start_1_size);
    I2S.end();
  }
  */
/*
  tone(BUZZER_PIN, NOTE_D3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_E3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_F3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_G3, 300);
*/
}

void play_success(){ // play after each successful button press (not necessary)
  //tone(BUZZER_PIN, NOTE_A3, 250);
}

void play_lvl_up(){
  log_d("play lvl up (but not really)");
  delay(2000);
  /*
  if(I2S.begin(PDM_MONO_MODE, lvl_up_1_sampling_frequency, lvl_up_1_bits_per_sample)){
    I2S.setDataInPin(BUZZER_PIN);
    I2S.write(lvl_up_1_samples, lvl_up_1_size);
    I2S.end();
  }
  */
/*
  tone(BUZZER_PIN, NOTE_B3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_C4, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_D4, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_E4, 200);
  */
}

void play_fail(){
  log_d("play fail (but not really)");
  delay(2000);
  /*
  if(I2S.begin(PDM_MONO_MODE, fail_1_sampling_frequency, fail_1_bits_per_sample)){
    I2S.setDataInPin(BUZZER_PIN);
    I2S.write(fail_1_samples, fail_1_size);
    I2S.end();
  }
  */
  /*
  // tu-du-du-duuu
  tone(BUZZER_PIN, NOTE_G3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_F3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_E3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_D3, 400);
  */
}

void generate_sequence(unsigned short *sequence, unsigned long max_game_sequence){
  //log_d("gen sequence");
  // Create a new game sequence.
  randomSeed(analogRead(0));
  unsigned short second_prev;
  unsigned short prev;

  sequence[0] = random(0, 3);
  second_prev = sequence[0];
  sequence[1] = random(0, 3);
  prev = sequence[1];

  //log_d("start generating");
  for (int i = 2; i < max_game_sequence; i++) {
    sequence[i] = random(0, 3);
    if(sequence[i] == second_prev || sequence[i] == prev){
      // prevent 3 matching colors in a row (2 in row is ok)
      sequence[i] = (sequence[i]+1)%4;
    }
    Serial.printf("[%d]=%d\n", i, sequence[i]);
    second_prev = prev;
    prev = sequence[i];
  }
  //log_d("done generating");
}

void correct_strobe(){
    led_on(YELLOW_LED);delay(100);
    led_on(BLUE_LED);delay(100);
    led_on(GREEN_LED);delay(100);
    led_on(RED_LED); delay(100);

    led_off(RED_LED);delay(100);
    led_off(GREEN_LED);delay(100);
    led_off(BLUE_LED);delay(100);
    led_off(YELLOW_LED);delay(100);
}

void buzzer_task(void* param){
  while(1){
    uint8_t buzzer_job = 255;
    //log_d("receiving side - buzzer_job after init %d", buzzer_job);
    if( buzzer_queue != NULL ){
      //log_d("receiving side - queue exists");
      if(xQueueReceive(buzzer_queue, &buzzer_job, ( TickType_t ) 100 ) == pdPASS){
        log_d("receiving side - queue received item %d", buzzer_job);
        //delay(500);
        //Serial.printf("buzzer task job %d\n", buzzer_job);
        //delay(500);
        switch(buzzer_job){
          case PLAY_START:
            log_d("switch: PLAY_START");
            play_start();
            break;
          case PLAY_SUCCESS:
            log_d("switch: PLAY_SUCCESS");
            play_success();
            break;
          case PLAY_LVL_UP:
            log_d("switch: PLAY_LVL_UP");
            play_lvl_up();
            break;
          case PLAY_FAIL:
            log_d("switch: PLAY_FAIL");
            play_fail();
            break;
          default:
            log_d("switch: default");
        } // swtich
      } // if xQueueReceive OK
    } // if buzzer_queue exists
  } // inf loop
}

void setup() {
  Serial.begin(115200);
  while(!Serial){delay(100);}

  debouncerRed.attach(RED_BUTTON, INPUT_PULLUP);
  debouncerRed.interval(30);
  debouncerGreen.attach(GREEN_BUTTON, INPUT_PULLUP);
  debouncerGreen.interval(30);
  debouncerBlue.attach(BLUE_BUTTON, INPUT_PULLUP);
  debouncerBlue.interval(30);
  debouncerYellow.attach(YELLOW_BUTTON, INPUT_PULLUP);
  debouncerYellow.interval(30);

  pinMode(BUZZER_PIN, OUTPUT);

  //Serial1.begin(115200);
  //while(!Serial1){delay(100);}

  buzzer_queue = xQueueCreate(10, sizeof(uint8_t));
  TaskHandle_t buzzer_task_handler = NULL;
  xTaskCreate(
    buzzer_task,   // Function to implement the task
    "buzzer_task", // Name of the task
    5000,              // Stack size in words
    (void *) NULL,    // Task input parameter
    1,                       // Priority of the task
    &buzzer_task_handler     // Task handle.
    );
  if(buzzer_task_handler == NULL){
    log_e("Could not create red task");
    // todo
  }

  //I2S.begin(PDM_STEREO_MODE, 8000, 16);
  //I2S.begin(PDM_MONO_MODE, lvl_up_audioSamplingFrequency, 16);
  //I2S.setDataInPin(BUZZER_PIN);

  //Serial1.write(RESTART);
  flash_timer = millis();
}

void loop() {
  uint8_t buzzer_job = 255;
  if (! gameInProgress) {
    // Waiting for someone to press any button...
      update_buttons();

    if (debouncerRed.fell() || debouncerGreen.fell() || debouncerBlue.fell() || debouncerYellow.fell()) {
      all_leds_off();
      
      buzzer_job = PLAY_START;
      if( buzzer_queue != 0 ){
        if(xQueueSend(buzzer_queue, ( void * ) &buzzer_job, ( TickType_t ) 100 ) != pdPASS ){
          log_d("O-oufailt to push into queue");
        }else{
          log_d("ok, pushed to queue %d", buzzer_job);
        }
      }else{
        log_d("Oh no, how could this happen buzzer_queue is == 0");
      }
      //Serial1.write(STARTUP);
      generate_sequence(gameSequence, MAX_GAME_SEQUENCE);

      currentSequenceLength = 1;

      gameInProgress = true;
      showingSequenceToUser = true;

      // Little delay before the game starts.
      delay(500);
    } else {
      // Attract mode - flash the green LED.
      if (millis() - flash_timer >= FLASH_DELAY){
        attractLEDOn = ! attractLEDOn;
        if(attractLEDOn){
          //all_leds_on(); // TODO - uncommnet - this was annoying to work with
        }else{
          all_leds_off();
        }
        flash_timer = millis();
      }
    }
  } else {
    // Game is in progress...
    if (showingSequenceToUser) {
      // Play the pattern out to the user
      for (n = 0; n < currentSequenceLength; n++) {
        Serial.printf("gameSequence[%d] = %d => Flash pin led_pin_map[%d] = %d = \"%s\"\n", n, gameSequence[n], gameSequence[n], led_pin_map[gameSequence[n]], led_text_map[gameSequence[n]]);
        led_on(led_pin_map[gameSequence[n]]);
        delay(FLASH_DELAY);
        led_off(led_pin_map[gameSequence[n]]);
        delay(FLASH_DELAY);
      }

      showingSequenceToUser = false;
      userPositionInSequence = 0;
    } else {
      // Waiting for the user to repeat the sequence back
      update_buttons();

      int userPressed = -1;

      if (debouncerRed.fell()) {
        Serial.println("red");
        led_on(RED_LED);
        delay(FLASH_DELAY);
        led_off(RED_LED);
        userPressed = RED_BUTTON;
      } else  if (debouncerGreen.fell()) {
        Serial.println("green");
        led_on(GREEN_LED);
        delay(FLASH_DELAY);
        led_off(GREEN_LED);
        userPressed = GREEN_BUTTON;
      } else  if (debouncerBlue.fell()) {
        Serial.println("blue");
        led_on(BLUE_LED);
        delay(FLASH_DELAY);
        led_off(BLUE_LED);
        userPressed = BLUE_BUTTON;
      } else if (debouncerYellow.fell()) {
        Serial.println("yellow");
        led_on(YELLOW_LED);
        delay(FLASH_DELAY);
        led_off(YELLOW_LED);
        userPressed = YELLOW_BUTTON;
      }

      if (userPressed > -1) {
        Serial.printf("Evaluate button press:\n userPressed=%d\nuserPositionInSequence=%d\ngameSequence[userPositionInSequence]=%d\nbutton_pin_map[gameSequence[userPositionInSequence]]=%d\nuserPressed != button_pin_map[gameSequence[userPositionInSequence]]=%d\n",userPressed, userPositionInSequence, gameSequence[userPositionInSequence], button_pin_map[gameSequence[userPositionInSequence]], userPressed != button_pin_map[gameSequence[userPositionInSequence]]);
        // A button was pressed, check it against current sequence...
        if (userPressed != button_pin_map[gameSequence[userPositionInSequence]]) {
          // Failed...
          Serial.println("  Fail");
          //Serial1.write(FAILED);
          buzzer_job = PLAY_FAIL;
          xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
          fail_strobe();
          //tone(BUZZER_PIN, NOTE_F3, 300);
          //delay(300);
          //tone(BUZZER_PIN, NOTE_G3, 500);
          //delay(2500);
          gameInProgress = false;
        } else {
          Serial.println("  Correct");
          userPositionInSequence++;
          if (userPositionInSequence == currentSequenceLength) {
            buzzer_job = PLAY_LVL_UP;
            xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
            //Serial1.write(LEVEL_UP);
            correct_strobe();

            // User has successfully repeated back the sequence so make it one longer...
            currentSequenceLength++;

            // There's no win scenario here, so just reset...
            if (currentSequenceLength >= MAX_GAME_SEQUENCE) {
              gameInProgress = false;
            }

            // Play a tone...
            buzzer_job = PLAY_SUCCESS;
            xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );

            //tone(BUZZER_PIN, NOTE_A3, 300);
            //delay(2000);

            showingSequenceToUser = true;
          }
        }
      }
    }
  }
}
