#include "tft.h"

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

void tft_setup() {
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