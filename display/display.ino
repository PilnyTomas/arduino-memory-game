// Connect to C3 DevkitC via UART:
//   C3   | Kaluga
// RX1 18 | 19 TX
// TX1 19 | 20 RX

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

enum command {RESTART, STARTUP, LEVEL_UP, FAILED};

SPIClass TFT_SPI = SPIClass(HSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(&TFT_SPI, TFT_DC, TFT_CS, TFT_RST);

void draw_info(){
  tft.fillScreen(0xFFFF);
  tft.setTextSize(2);
  //tft.setCursor(10, 20); tft.printf("Press any button to start the game");
  tft.setCursor(5, 20); tft.printf("Stiskni keterekoliv");
  tft.setCursor(15, 60); tft.printf("tlacitko a zacni");
  tft.setCursor(90, 100); tft.printf("hrat");
}

void draw_fail(uint8_t level){
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
    tft.setCursor(5, 220); tft.printf("Pust ostatni a zkus");
    tft.setCursor(5, 240); tft.printf("to znova pozdeji");
  }else{
    //tft.setCursor(80, 20); tft.printf("Congratulation! You have won ");
    tft.setCursor(50, 150); tft.printf("Gratulujeme!");
    tft.setCursor(5, 170); tft.printf("Vyhravate %s%s", level >= 5 ? "bonbon" : "",  level >= 10 ? "," : "");
    tft.setCursor(5, 190); tft.printf("%s%s", level >= 10 ? "tricko" : "", level >= 15 ? ", ESP32" : "");
  }

  tft.setCursor(5, 230); tft.printf("Stiskni keterekoliv");
  tft.setCursor(15, 250); tft.printf("tlacitko a zacni");
  tft.setCursor(90, 270); tft.printf("hrat");
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
  tft.setCursor(5, 110); tft.printf("%s 5: Bonbon", level < 5 ? " " : "*");

  //tft.setCursor(5, 130); tft.printf("%s10:T-shirt", level < 10 ? " " : "*");
  tft.setCursor(5, 130); tft.printf("%s10: Tricko", level < 10 ? " " : "*");

  //tft.setCursor(5, 150); tft.printf("%s15:Dev board", level < 15 ? " " : "*");
  tft.setCursor(5, 150); tft.printf("%s15: ESP32", level < 15 ? " " : "*");

  //tft.setCursor(5, 170); tft.printf("%s20:Job @ Espressif", level < 20 ? " " : "*");
}

void setup() {
  Serial.begin(115200);
  while(!Serial){delay(100);}

  TFT_SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, -1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);
  tft.begin();
  tft.setRotation(2);
  tft.setTextColor(0x0000);
  tft.setFont(NULL);
  tft.setTextSize(2);

  draw_info(); // Press any button to start
}

void loop(){
  static uint8_t game_level = 0;
  uint8_t incomingByte;
  if(Serial.available() > 0){
    incomingByte = Serial.read();
    incomingByte = incomingByte - '0'; // convert from char to number

    switch(incomingByte){
      case RESTART: // after controller startup/flashing/restart
        draw_info(); // Press any button to start
        break;
      case STARTUP: // after button press, at the beginning of a new game
        game_level = 0;
        draw_progress(game_level);
        break;
      case LEVEL_UP:
        draw_progress(++game_level);
        break;
      case FAILED:
        draw_fail(game_level);
        delay(10000);
        game_level = 0;
        draw_info();
        break;
    }
  }
}