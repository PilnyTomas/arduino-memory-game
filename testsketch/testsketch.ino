#include <Bounce2.h>


#define RED_LED 4
#define GREEN_LED 5
#define BLUE_LED 6
#define YELLOW_LED 7

#define RED_BUTTON 0
#define GREEN_BUTTON 1
#define BLUE_BUTTON 2
#define YELLOW_BUTTON 3

#define BUZZER_PIN 9

Bounce debouncerRed = Bounce();
Bounce debouncerGreen = Bounce();
Bounce debouncerBlue = Bounce();
Bounce debouncerYellow = Bounce();

unsigned short currentDelay;

void led_on(uint8_t pin){
  pinMode(pin, OUTPUT);
  digitalWrite(RED_LED, LOW);
}

void led_off(uint8_t pin){
  pinMode(pin, INPUT);
}

void setup() {
  Serial.begin(115200);
  while(!Serial){delay(100);}

  led_on(RED_LED);
  led_on(GREEN_LED);
  led_on(BLUE_LED);
  led_on(YELLOW_LED);

  delay(3000);

  led_off(RED_LED);
  led_off(GREEN_LED);
  led_off(BLUE_LED);
  led_off(YELLOW_LED);
  debouncerRed.attach(RED_BUTTON, INPUT_PULLUP);
  debouncerRed.interval(30);
  debouncerGreen.attach(GREEN_BUTTON, INPUT_PULLUP);
  debouncerGreen.interval(30);
  debouncerBlue.attach(BLUE_BUTTON, INPUT_PULLUP);
  debouncerBlue.interval(30);
  debouncerYellow.attach(YELLOW_BUTTON, INPUT_PULLUP);
  debouncerYellow.interval(30);
}

bool red_led_state = false;
bool green_led_state = false;
bool blue_led_state = false;
bool yellow_led_state = false;

void loop(){
  debouncerGreen.update();
  debouncerRed.update();
  debouncerBlue.update();
  debouncerYellow.update();

  if(debouncerRed.rose()){
    Serial.println("red");
    if(red_led_state){led_off(RED_LED);red_led_state=false;}else{led_on(RED_LED);red_led_state=true;}
  }
  if(debouncerGreen.rose()){
    Serial.println("green");
    if(green_led_state){led_off(GREEN_LED);green_led_state=false;}else{led_on(GREEN_LED);green_led_state=true;}
  }
  if(debouncerBlue.rose()){
    Serial.println("blue");
    if(blue_led_state){led_off(BLUE_LED);blue_led_state=false;}else{led_on(BLUE_LED);blue_led_state=true;}

  }
  if(debouncerYellow.rose()){
    Serial.println("yellow");
    if(yellow_led_state){led_off(YELLOW_LED);yellow_led_state=false;}else{led_on(YELLOW_LED);yellow_led_state=true;}
  }
  //tone(BUZZER_PIN, 173, 300);
}
