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

#define ERROR 100

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"


SPIClass TFT_SPI = SPIClass(HSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(&TFT_SPI, TFT_DC, TFT_CS, TFT_RST);

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

  draw_progress(0);

  // demonstration
  /*
  uint8_t level = 0;
  do{
    draw_progress(level++);
    delay(1000);
  }while(level <= 25);
  */
}

void loop(){
  uint8_t incomingByte;
  if(Serial.available() > 0){
    incomingByte = Serial.read();
    if(incomingByte < 30){
      draw_progress(incomingByte);
    }else if(incomingByte >= '0' && incomingByte <= '9'){
      uint8_t level = incomingByte - '0';
      if(Serial.available() > 0){ // second digit ?
        incomingByte = Serial.read();
        if(incomingByte >= '0' && incomingByte <= '9'){
          level = level * 10 + incomingByte - '0';
        }else{ // else second character is junk and whole transmission will be ignored
          Serial.println("junk");
          level = ERROR;
        }
      }else{
      } // second digit?
      if(level < ERROR){
        draw_progress(level);
      } // if ERROR
    } // is it character ?

  }else{ // Serial not available
    delay(100);
  } // if serial available
}