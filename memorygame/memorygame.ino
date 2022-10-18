#include <Bounce2.h>
#include "tones.h"

//#include "I2S.h"
//#include "start_1.c"
//#include "lvl_up_1.c"
//#include "fail_1.c"

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
enum buzzer_msg {PLAY_START, PLAY_SUCCESS, PLAY_LVL_UP, PLAY_FAIL, PLAY_RED, PLAY_GREEN, PLAY_BLUE, PLAY_YELLOW};

typedef struct {
  uint8_t led_pin;
  uint16_t flash_millis; // how long the LED stays on
} led_blink_t;

// GPIO 8 is for RGD LED on board

#define BUZZER_PIN 9

#define MAX_GAME_SEQUENCE 30
#define FLASH_DELAY 500

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
QueueHandle_t led_queue;

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

/*
// I2S function
void play_array(const uint32_t sampling_frequency,
                const uint8_t bits_per_sample,
                const size_t size,
                const int16_t *samples){
  log_d("play array");
  if(I2S.begin(PDM_STEREO_MODE, sampling_frequency, bits_per_sample)){
    size_t written = 0;
    int pointer = 0;
    int available = 0;
    I2S.setSckPin(PDM_CLK);
    I2S.setDataInPin(BUZZER_PIN);
    while(pointer < size){
      available = I2S.availableForWrite();
      if(available){
        written = I2S.write((void*)(samples+pointer), available);
        pointer += written;
        log_d("availableForWrite = %d; written = %d; new pointer = %d", available, written, pointer);
      }
    } // while data remain to be played
    I2S.end();
  }else{log_e("Failed to start I2S in PDM mode");}
}
*/

void play_start(){
  //play_array(start_1_sampling_frequency, start_1_bits_per_sample, start_1_size, start_1_samples);
  delay(150);
  tone(BUZZER_PIN, NOTE_D3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_E3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_F3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_G3, 300);
  delay(150);
}

void play_success(){ // play after each successful button press (not necessary)
  //tone(BUZZER_PIN, NOTE_A3, 250);
  //delay(150);
}

void play_lvl_up(){
  //play_array(lvl_up_1_sampling_frequency, lvl_up_1_bits_per_sample, lvl_up_1_size, lvl_up_1_samples);
  delay(250);
  tone(BUZZER_PIN, NOTE_B3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_C4, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_D4, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_E4, 200);
  delay(150);
}

void play_fail(){
  //play_array(fail_1_sampling_frequency, fail_1_bits_per_sample, fail_1_size, fail_1_samples);

  // tu-du-du-duuu
  delay(150);
  tone(BUZZER_PIN, NOTE_G3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_F3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_E3, 150);
  delay(150);
  tone(BUZZER_PIN, NOTE_D3, 400);
  delay(150);
}

void play_red(){
  delay(150);
  tone(BUZZER_PIN, NOTE_F4, 100);
  delay(150);
}
void play_green(){
  delay(150);
  tone(BUZZER_PIN, NOTE_E4, 100);
  delay(150);
}
void play_blue(){
  delay(150);
  tone(BUZZER_PIN, NOTE_D4, 100);
  delay(150);
}
void play_yellow(){
  delay(150);
  tone(BUZZER_PIN, NOTE_C4, 100);
  delay(150);
}

void generate_sequence(unsigned short *sequence, unsigned long max_game_sequence){
  // Create a new game sequence.
  randomSeed(analogRead(0));
  unsigned short second_prev;
  unsigned short prev;

  sequence[0] = random(0, 3);
  second_prev = sequence[0];
  sequence[1] = random(0, 3);
  prev = sequence[1];

  for (int i = 2; i < max_game_sequence; i++) {
    sequence[i] = random(0, 3);
    if(sequence[i] == second_prev || sequence[i] == prev){
      // prevent 3 matching colors in a row (2 in row is ok)
      sequence[i] = (sequence[i]+1)%4;
    }
    //Serial.printf("[%d]=%d\n", i, sequence[i]);
    second_prev = prev;
    prev = sequence[i];
  }
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
    if( buzzer_queue != NULL ){
      if(xQueueReceive(buzzer_queue, &buzzer_job, ( TickType_t ) 100 ) == pdPASS){
        switch(buzzer_job){
          case PLAY_START:
            play_start();
            break;
          case PLAY_SUCCESS:
            play_success();
            break;
          case PLAY_LVL_UP:
            play_lvl_up();
            break;
          case PLAY_FAIL:
            play_fail();
            break;
          case PLAY_RED:
            play_red();
            break;
          case PLAY_GREEN:
            play_green();
            break;
          case PLAY_BLUE:
            play_blue();
            break;
          case PLAY_YELLOW:
            play_yellow();
            break;
        } // swtich
      } // if xQueueReceive OK
    } // if buzzer_queue exists
  } // inf loop
}


void led_task(void* param){
  led_blink_t led_job;
  while(1){
    if( led_queue != NULL ){
      if(xQueueReceive(led_queue, &led_job, ( TickType_t ) 0 ) == pdPASS){
        Serial.printf("flash led %s on pin %d for %d ms\n", led_text_map[led_job.led_pin-RED_LED], led_job.led_pin, led_job.flash_millis);
        led_on(led_job.led_pin);
        delay(led_job.flash_millis);
        led_off(led_job.led_pin);
      } // if xQueueReceive OK
    } // if led_queue exists
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

  Serial1.begin(115200);
  while(!Serial1){delay(100);}

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
    log_e("Could not create buzzer task");
    ESP.restart();
  }

  led_queue = xQueueCreate(10, sizeof(led_blink_t));
  TaskHandle_t led_task_handler = NULL;
  xTaskCreate(
    led_task,   // Function to implement the task
    "led_task", // Name of the task
    5000,              // Stack size in words
    (void *) NULL,    // Task input parameter
    1,                       // Priority of the task
    &led_task_handler     // Task handle.
    );
  if(led_task_handler == NULL){
    log_e("Could not create led task");
    ESP.restart();
  }


  Serial1.write(RESTART);
  flash_timer = millis();
}

// TODO variable speed (FLASH_DELAY)

void loop() {
  uint8_t buzzer_job = 255;
  led_blink_t led_job;
  led_job.flash_millis = FLASH_DELAY;
  if (! gameInProgress) {
    // Waiting for someone to press any button...
      update_buttons();

    //if (debouncerRed.fell() || debouncerGreen.fell() || debouncerBlue.fell() || debouncerYellow.fell()) {
    //all_leds_off();
    if (debouncerGreen.fell()) {
      led_off(GREEN_LED);
      
      buzzer_job = PLAY_START;
      if( buzzer_queue != 0 ){
        if(xQueueSend(buzzer_queue, ( void * ) &buzzer_job, ( TickType_t ) 100 ) != pdPASS ){
          log_w("Failed to push into queue - buzzer will be silent");
        }
      }else{
        log_e("buzzer_queue is == 0");
      }
      Serial1.write(STARTUP);
      generate_sequence(gameSequence, MAX_GAME_SEQUENCE);

      currentSequenceLength = 1;

      gameInProgress = true;
      showingSequenceToUser = true;

      // Little delay before the game starts.
      delay(2*FLASH_DELAY);
    } else {
      // Attract mode - flash the green LED.
      if (millis() - flash_timer >= 2*FLASH_DELAY){
        attractLEDOn = ! attractLEDOn;
        if(attractLEDOn){
          //all_leds_on();
          led_on(GREEN_LED);
        }else{
          //all_leds_off();
          led_off(GREEN_LED);
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
        buzzer_job = PLAY_RED+gameSequence[n]; xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
        led_on(led_pin_map[gameSequence[n]]);
        delay(FLASH_DELAY);
        led_off(led_pin_map[gameSequence[n]]);
        delay(FLASH_DELAY);
      }
      delay(2*FLASH_DELAY);
      showingSequenceToUser = false;
      userPositionInSequence = 0;
    } else {
      // Waiting for the user to repeat the sequence back
      update_buttons();

      int userPressed = -1;

      if (debouncerRed.fell()) {
        Serial.println("Red");
        buzzer_job = PLAY_RED; xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
        led_job.led_pin = RED_LED;
        xQueueSend(led_queue, ( void * ) &led_job, ( TickType_t ) 100 );
        /*
        led_on(RED_LED);
        delay(FLASH_DELAY);
        led_off(RED_LED);
        */
        userPressed = RED_BUTTON;
      } else  if (debouncerGreen.fell()) {
        Serial.println("Green");
        buzzer_job = PLAY_GREEN; xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
        led_job.led_pin = GREEN_LED;
        xQueueSend(led_queue, ( void * ) &led_job, ( TickType_t ) 100 );
        /*
        led_on(GREEN_LED);
        delay(FLASH_DELAY);
        led_off(GREEN_LED);
        */
        userPressed = GREEN_BUTTON;
      } else  if (debouncerBlue.fell()) {
        Serial.println("Blue");
        buzzer_job = PLAY_BLUE; xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
        led_job.led_pin = BLUE_LED;
        led_job.flash_millis =FLASH_DELAY;
        xQueueSend(led_queue, ( void * ) &led_job, ( TickType_t ) 100 );
        /*
        led_on(BLUE_LED);
        delay(FLASH_DELAY);
        led_off(BLUE_LED);
        */
        userPressed = BLUE_BUTTON;
      } else if (debouncerYellow.fell()) {
        Serial.println("Yellow");
        buzzer_job = PLAY_YELLOW; xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
        led_job.led_pin = YELLOW_LED;
        xQueueSend(led_queue, ( void * ) &led_job, ( TickType_t ) 100 );
        /*
        led_on(YELLOW_LED);
        delay(FLASH_DELAY);
        led_off(YELLOW_LED);
        */
        userPressed = YELLOW_BUTTON;
      }

      if (userPressed > -1) {
        Serial.printf("Evaluate button press:\n userPressed=%d\nuserPositionInSequence=%d\ngameSequence[userPositionInSequence]=%d\nbutton_pin_map[gameSequence[userPositionInSequence]]=%d\nuserPressed != button_pin_map[gameSequence[userPositionInSequence]]=%d\n",userPressed, userPositionInSequence, gameSequence[userPositionInSequence], button_pin_map[gameSequence[userPositionInSequence]], userPressed != button_pin_map[gameSequence[userPositionInSequence]]);
        // A button was pressed, check it against current sequence...
        if (userPressed != button_pin_map[gameSequence[userPositionInSequence]]) {
          // Failed...
          Serial.println("  Fail");
          Serial1.write(FAILED);
          buzzer_job = PLAY_FAIL;
          xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
          fail_strobe();
          gameInProgress = false;
        } else {
          Serial.println("  Correct");
          userPositionInSequence++;
          buzzer_job = PLAY_SUCCESS;
          xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );

          if (userPositionInSequence == currentSequenceLength) {
            buzzer_job = PLAY_LVL_UP;
            xQueueSend(buzzer_queue, &buzzer_job, ( TickType_t ) 10 );
            Serial1.write(LEVEL_UP);
            //correct_strobe();
            delay(4*FLASH_DELAY);

            // User has successfully repeated back the sequence so make it one longer...
            currentSequenceLength++;

            // There's no win scenario here, so just reset...
            if (currentSequenceLength >= MAX_GAME_SEQUENCE) {
              gameInProgress = false;
            }

            showingSequenceToUser = true;
          }
        }
      }
    }
  }
}
