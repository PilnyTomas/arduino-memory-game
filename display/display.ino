// Connect to C3 DevkitC via UART:
//   C3   | Kaluga
// RX1 18 | 19 TX
// TX1 19 | 20 RX
//
// 17/18

#define TFT_DC     13
#define TFT_CS     11
#define TFT_MOSI    9
#define TFT_CLK    15
#define TFT_MISO    8
#define TFT_BL      6
#define TFT_RST    16

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define CANDY_LVL 5
#define KEYCHAIN_LVL 10
#define TSHIRT_LVL 18
#define DEV_BOARD_LVL 21

#define FAKE_HIGH_SCORE_INIT CANDY_LVL-1

enum command {RESTART, STARTUP, LEVEL_UP, FAILED};

SPIClass TFT_SPI = SPIClass(HSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(&TFT_SPI, TFT_DC, TFT_CS, TFT_RST);

void draw_info(uint8_t high_score){
  tft.fillScreen(0xFFFF);
  tft.setTextSize(2);
  //tft.setCursor(10, 20); tft.printf("Press any button to start the game");
  //tft.setCursor(5, 20); tft.printf("Stiskni keterekoliv");
  tft.setCursor(20, 20); tft.printf("Stiskni zelene");
  tft.setCursor(15, 60); tft.printf("tlacitko a zacni");
  tft.setCursor(90, 100); tft.printf("hrat");

  tft.setCursor(90, 120); tft.printf("Nejvyssi dosazena");
  tft.setCursor(90, 140); tft.printf("uroven");
  tft.setTextSize(3);
  tft.setCursor(90, 160); tft.printf("%d", high_score);
}

void draw_fail(uint8_t level, uint8_t high_score){
  tft.fillScreen(0xFFFF);

  tft.setTextSize(3);
              // X, Y
  //tft.setCursor(20, 20); tft.printf("GAME OVER! You have reach level");
  tft.setCursor(30, 20); tft.printf("KONEC HRY!");

  tft.setTextSize(2);
  tft.setCursor(20, 60); tft.printf("Dosahl jsi urovne");

  tft.setTextSize(4);
  tft.setCursor(100, 100);
  tft.printf("%d", level);

  tft.setTextSize(2);
  if(level < 5){
    //tft.setCursor(80, 20); tft.printf("Unfortunately that is not enough for any prize. Give way to others and try again later");
    tft.setCursor(5, 150); tft.printf("Bohuzel to nestaci");
    tft.setCursor(5, 170); tft.printf("na zadnou z vyher.");
    tft.setCursor(5, 190); tft.printf("Pust ostatni a zkus");
    tft.setCursor(5, 210); tft.printf("to znova pozdeji");
  }else{
    //tft.setCursor(80, 20); tft.printf("Congratulation! You have won ");
    tft.setCursor(50, 150); tft.printf("Gratulujeme!");
    tft.setCursor(5, 170); tft.printf("Vyhravate %s%s", level >= CANDY_LVL ? "bonbon" : "", level >= KEYCHAIN_LVL ? ", klicenku`" : "");
    tft.setCursor(5, 190); tft.printf("%s%s", level >= TSHIRT_LVL ? ", platenku" : "", level >= DEV_BOARD_LVL ? ", tricko" : "");
  }

  //tft.setCursor(5, 230); tft.printf("Stiskni keterekoliv");
  tft.setCursor(20, 240); tft.printf("Stiskni zelene");
  tft.setCursor(15, 260); tft.printf("tlacitko a zacni");
  tft.setCursor(90, 280); tft.printf("hrat");
}

void draw_progress(uint8_t level){
  tft.fillScreen(0xFFFF);
  //tft.setCursor(20, 20); tft.printf("Levels completed:");
  tft.setCursor(80, 20); tft.printf("Uroven:");

  tft.setTextSize(4);
  tft.setCursor(100, 60);
  tft.printf("%d", level);

  tft.setTextSize(2);

  //tft.setCursor(5, 110); tft.printf("%s 5:Candy", level < 5 ? " " : "*");
  tft.setCursor(5, 110); tft.printf("%s%d: Bonbon", level < CANDY_LVL ? " " : "*", CANDY_LVL);

  //tft.setCursor(5, 130); tft.printf("%s10:T-shirt", level < 10 ? " " : "*");
  tft.setCursor(5, 130); tft.printf("%s%d: Klicenka", level < KEYCHAIN_LVL ? " " : "*", KEYCHAIN_LVL);

  //tft.setCursor(5, 130); tft.printf("%s10:T-shirt", level < 10 ? " " : "*");
  tft.setCursor(5, 150); tft.printf("%s%d: Tricko", level < TSHIRT_LVL ? " " : "*", TSHIRT_LVL);

  //tft.setCursor(5, 150); tft.printf("%s15:Dev board", level < 15 ? " " : "*");
  tft.setCursor(5, 170); tft.printf("%s%d: ESP32", level < DEV_BOARD_LVL ? " " : "*", DEV_BOARD_LVL);

  //tft.setCursor(5, 170); tft.printf("%s20:Job @ Espressif", level < 20 ? " " : "*");
}

void setup() {
  Serial.begin(115200);
  while(!Serial){delay(100);}

  Serial1.begin(115200);
  while(!Serial1){delay(100);}

  TFT_SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, -1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);
  tft.begin();
  tft.setRotation(2);
  tft.setTextColor(0x0000);
  tft.setFont(NULL);
  tft.setTextSize(2);

  draw_info(FAKE_HIGH_SCORE_INIT); // Press any button to start
}

void loop(){
  static uint8_t game_level = 0;
  static uint8_t high_score = FAKE_HIGH_SCORE_INIT;
  uint8_t incomingByte = RESTART;
  static uint8_t prev_state = RESTART;
  static uint32_t fail_time = 0;

  // 10 sec after fail draw starting info, and do not block incoming commands
  //Serial.printf("prev_state = %d; millis(%lu) - fail_time %d = %lu\n", prev_state, millis(), fail_time, millis() - fail_time); // debug
  //Serial.printf("prev_state = %d; \n", prev_state); // debug
  if(prev_state == FAILED){
    Serial.printf("prev_state = %d expecting 3\n", prev_state); // debug
    if((millis() - fail_time) > 10000){
      Serial.printf("reset display\n"); // debug
      draw_info(high_score);
      prev_state = RESTART;
    }
  }

  if(Serial1.available() > 0){
    incomingByte = Serial1.read();
    //incomingByte = incomingByte - '0'; // convert from char to number - used for manual input
    prev_state = incomingByte;
    Serial.printf("new prev state is %d\n", prev_state); // debug
    Serial.printf("Received %d\n", incomingByte); // debug
    switch(incomingByte){
      case RESTART: // after controller startup/flashing/restart
        draw_info(high_score); // Press any button to start
        break;
      case STARTUP: // after button press, at the beginning of a new game
        game_level = 0;
        draw_progress(game_level);
        break;
      case LEVEL_UP:
        draw_progress(++game_level);
        break;
      case FAILED:
        if(Serial1.available() > 0){ high_score = Serial1.read(); }
        draw_fail(game_level, high_score);
        fail_time =  millis();
        Serial.printf("Failed note fail time = %ul\n", fail_time); // debug
        game_level = 0;
        break;
    }
  }
}
