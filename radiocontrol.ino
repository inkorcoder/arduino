#include "IRremote.h"
#include <EEPROM.h>

int One = 0xFFA25D;
int Two = 0xFF629D;
int Three = 0xFFE21D;
int Four = 0xFF22DD;
int Five = 0xFF02FD;
int Six = 0xFFC23D;
int Seven = 0xFFE01F;
int Eight = 0xFFA857;
int Nine = 0xFF906F;
int Zero = 0xFF9867;

int Star = 0xFF6897;
int Hash = 0xFFB04F;

int Ok = 0xFF38C7;
int Up = 0xFF18E7;
int Down = 0xFF4AB5;
int Left = 0xFF10EF;
int Right = 0xFF5AA5;

int codesArray[10] = {Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine};

int RECV_PIN = 11;

IRrecv irrecv(RECV_PIN);

decode_results results;

// global variables
#define RELAY_PIN 5
#define MODE_EEPROM_INDEX 9
#define BRIGHTNESS_EEPROM_INDEX 10
#define CURRENT_COLOR_EEPROM_INDEX 11
#define VOLUME_EEPROM_INDEX 12

#include "FastLED.h"
#define NUM_LEDS 58 * 2
#define LED_PIN     6
int MODE = 0;
int BRIGHTNESS = 0;
int BRIGHTNESS_STEP = 50;
int brightnessIndex = 0;
int brightnessSteps[5] = {2, 10, 30, 50, 255};
#define LED_TYPE    NEOPIXEL
#define COLOR_ORDER RGB
int GLOBAL_SIGNAL_DIVIDER = 17;
int VOLUME = 0;

int GRADIENT_SPEED = 1;

int CURRENT_COLOR_INDEX = 0;
CRGB CURRENT_COLORS[3];
CRGB GRADIENT_COLOR;

CRGB leds[NUM_LEDS];
CRGB Colors[10] = {
  CRGB(250, 250, 250),
  CRGB(250, 50, 50),
  CRGB(250, 178, 50),
  CRGB(161, 250, 50),
  CRGB(50, 250, 50),
  CRGB(50, 250, 203),
  CRGB(50, 136, 250),
  CRGB(115, 50, 250),
  CRGB(199, 50, 250),
  CRGB(250, 50, 155)
};

void setup(){
  Serial.begin(9600);
  irrecv.enableIRIn();
  FastLED.addLeds<LED_TYPE, LED_PIN>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );

  EEPROM.get(BRIGHTNESS_EEPROM_INDEX, brightnessIndex);
  BRIGHTNESS = brightnessSteps[brightnessIndex];
  FastLED.setBrightness(  BRIGHTNESS );

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  EEPROM.get(CURRENT_COLOR_EEPROM_INDEX, CURRENT_COLOR_INDEX);
  GetCurrentColors();

  EEPROM.get(VOLUME_EEPROM_INDEX, VOLUME);

}

int index = 0;
float percent = 0;
float valueLeft = 0, valueRight = 0;
void loop(){
  GRADIENT_COLOR = CHSV(percent * 255, 255, 255);
  // controls read  
  while (!irrecv.isIdle());
  if (irrecv.decode(&results)){
    int digit = getDigitFromEvent(results.value);
    String key = getButtonFromEvent(results.value);
    if (digit > -1){
      Serial.print("digit: ");    
      Serial.println(digit);    
      // mode
      if (digit == 1) setMode(0);
      if (digit == 2) setMode(1);
      if (digit == 3) setMode(2);
      if (digit == 4) setMode(3);
      if (digit == 5) setMode(4);
      if (digit == 6) setMode(5);
      if (digit == 7) setMode(6);
      if (digit == 8) setMode(7);
      if (digit == 9) {
        setMode(8);
        if (MODE == 8){
          changeSpeed();
        }
      }
    }
    if (key != "undefined"){
      Serial.print("key: ");    
      Serial.println(key);
      // brightness
      if (key == "Up") increaseBrightness();
      if (key == "Down") decreaseBrightness();
      // volume
      if (key == "Right") increaseVolume();
      if (key == "Left") decreaseVolume();
      // color change
      if (key == "Ok") nextColor();
      // power led
      if (key == "Star") digitalWrite(RELAY_PIN, HIGH);
      if (key == "Hash") digitalWrite(RELAY_PIN, LOW);
    }
    irrecv.resume();
  }


  float newValueLeft = analogRead(1);
  float newValueRight = analogRead(2);
  if (newValueLeft > valueLeft) valueLeft = newValueLeft;
  if (newValueRight > valueRight) valueRight = newValueRight;

  for (int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }
  
  for (int i = 0; i < NUM_LEDS / 2; i++){
    if (MODE == 4) stroboscope(i, valueLeft);
    if (MODE == 7) fullLight(i);
    if (MODE == 8) fullLightGradient(i);
    if (pow(i, 2) / getDivider() < valueLeft){
        if (MODE == 0) singleColor(i);
        if (MODE == 1) twoColors(i);
        if (MODE == 2) threeColors(i);
        if (MODE == 3) vuMeter(i);
        // 4 -> stroboscope
        if (MODE == 5) smoothColor(i, valueLeft);
        if (MODE == 6) smoothSegment(i, valueLeft);

//      smoothColor(i, valueLeft);
//        smoothSegment(i, valueLeft);
//      leds[getLeftIndex(i)] = CURRENT_COLORS[0];
//      leds[getRightIndex(i)] = CURRENT_COLORS[0];
    }
  }
  if (MODE == 4){
    valueLeft *= 0.95;
    valueRight *= 0.95;  
  } else {
    valueLeft *= 0.98;
    valueRight *= 0.98;
  }
  FastLED.show();
  percent += (0.0001 * pow(2, GRADIENT_SPEED));
  if (percent >= 1) percent = 0;
}

int getDivider(){
  return GLOBAL_SIGNAL_DIVIDER + VOLUME;
}

int getLeftIndex(int index){
  return 57 - index;
}
int getRightIndex(int index){
  return 58 + index;
}

void changeSpeed(){
  GRADIENT_SPEED++;
  if (GRADIENT_SPEED > 5){
     GRADIENT_SPEED = 0;
  }
}

void increaseBrightness(){
  brightnessIndex = Clamp(brightnessIndex + 1, 0, 4);
  BRIGHTNESS = brightnessSteps[brightnessIndex];
  FastLED.setBrightness(BRIGHTNESS);
  EEPROM.put(BRIGHTNESS_EEPROM_INDEX, brightnessIndex);
  Serial.print("Brightness changed: ");
  Serial.println(BRIGHTNESS);
}
void decreaseBrightness(){
  brightnessIndex = Clamp(brightnessIndex - 1, 0, 4);
  BRIGHTNESS = brightnessSteps[brightnessIndex];
  FastLED.setBrightness(BRIGHTNESS);
  EEPROM.put(BRIGHTNESS_EEPROM_INDEX, brightnessIndex);
  Serial.print("Brightness changed: ");
  Serial.println(BRIGHTNESS);
}

void increaseVolume(){
  VOLUME = Clamp(VOLUME + 1, 0, 5);
  EEPROM.put(VOLUME_EEPROM_INDEX, VOLUME);
  Serial.print("Volume changed: ");
  Serial.println(VOLUME);
}
void decreaseVolume(){
  VOLUME = Clamp(VOLUME - 1, 0, 5);
  EEPROM.put(VOLUME_EEPROM_INDEX, VOLUME);
  Serial.print("Volume changed: ");
  Serial.println(VOLUME);
}

void prevMode(){
  if (MODE == 4) FastLED.setBrightness(BRIGHTNESS);
  MODE = (MODE > 0) ? --MODE : 8; 
  EEPROM.put(MODE_EEPROM_INDEX, MODE);
}
void nextMode(){
  if (MODE == 4) FastLED.setBrightness(BRIGHTNESS);
  MODE = (MODE < 8) ? ++MODE : 0; 
  EEPROM.put(MODE_EEPROM_INDEX, MODE);
}
void setMode(int index){
  if (MODE == 4) FastLED.setBrightness(BRIGHTNESS);
  MODE = Clamp(index, 0, 8); 
  EEPROM.put(MODE_EEPROM_INDEX, MODE);
  Serial.print("Mode changed: ");
  Serial.println(MODE);
}


int Clamp(int value, int _min, int _max){
  return (value < _min) ? _min : (value > _max) ? _max : value;
}

float Lerp(float value1, float value2, float t){
  return value1 + (value2 - value1) * t;
}

int getDigitFromEvent(int hexCode){
  for (int i = 0; i < 10; i++){
    if (hexCode == codesArray[i]){
      return i;
    }
  }
  return -1;
}

String getButtonFromEvent(int hexCode){
  String result = "undefined";
  switch (hexCode){
    case 0xFF18E7: result = "Up"; break;
    case 0xFF4AB5: result = "Down"; break;
    case 0xFF10EF: result = "Left"; break;
    case 0xFF5AA5: result = "Right"; break;
    case 0xFF38C7: result = "Ok"; break;
    case 0xFF6897: result = "Star"; break;
    case 0xFFB04F: result = "Hash"; break;
  }
  return result;
}

void GetCurrentColors(){
  for (int i = 0; i < 3; i++){
    int index = CURRENT_COLOR_INDEX + i;
    if (index <= 8){
      CURRENT_COLORS[i] = CRGB(Colors[index].r, Colors[index].g, Colors[index].b);
    } else {
      CURRENT_COLORS[i] = Colors[index - 9];
    }
  }
  printColor(CURRENT_COLORS[0]);
  printColor(CURRENT_COLORS[1]);
  printColor(CURRENT_COLORS[2]);
}

void nextColor(){
  if (CURRENT_COLOR_INDEX < 0) CURRENT_COLOR_INDEX = 0;
  CURRENT_COLOR_INDEX = (CURRENT_COLOR_INDEX >= 8) ? 0 : ++CURRENT_COLOR_INDEX;
  Serial.print("Color index changed: ");
  Serial.println(CURRENT_COLOR_INDEX);
  EEPROM.put(CURRENT_COLOR_EEPROM_INDEX, CURRENT_COLOR_INDEX);
  GetCurrentColors();
}

void printColor(CRGB _color){
  Serial.print("RGB(");
  Serial.print(_color.r);
  Serial.print(".");
  Serial.print(_color.g);
  Serial.print(".");
  Serial.print(_color.b);
  Serial.println(")");
}


void singleColor(int index){
  leds[getLeftIndex(index)] = CURRENT_COLORS[0];
  leds[getRightIndex(index)] = CURRENT_COLORS[0];
}

void twoColors(int index){
  int singleLineCount = NUM_LEDS / 2;
  leds[getLeftIndex(index)] = CURRENT_COLORS[(index >= singleLineCount * 0.75) ? 1 : 0];
  leds[getRightIndex(index)] = CURRENT_COLORS[(index >= singleLineCount * 0.75) ? 1 : 0];
}

void threeColors(int index){
  int singleLineCount = NUM_LEDS / 2;
  leds[getLeftIndex(index)] = CURRENT_COLORS[(index >= singleLineCount * 0.75) ? 2 : (index >= singleLineCount * 0.5) ? 1 : 0];
  leds[getRightIndex(index)] = CURRENT_COLORS[(index >= singleLineCount * 0.75) ? 2 : (index >= singleLineCount * 0.5) ? 1  : 0];
}

void vuMeter(int index){
  CURRENT_COLOR_INDEX = 4;
  CURRENT_COLORS[0] = Colors[4];
  CURRENT_COLORS[1] = Colors[2];
  CURRENT_COLORS[2] = Colors[1];
  threeColors(index);
}

void stroboscope(int index, float value){
//  Serial.println(value);
  singleColor(index);
  FastLED.setBrightness((float)pow(value, 2) / (float)50000 * BRIGHTNESS);
}

void smoothColor(int index, float value){
  CRGB lerpedColor = CRGB(
    Lerp(CURRENT_COLORS[0].r, CURRENT_COLORS[2].r, value / (float)250),
    Lerp(CURRENT_COLORS[0].g, CURRENT_COLORS[2].g, value / (float)250),
    Lerp(CURRENT_COLORS[0].b, CURRENT_COLORS[2].b, value / (float)250)
  );
  leds[getLeftIndex(index)] = lerpedColor;
  leds[getRightIndex(index)] = lerpedColor;
}

void smoothSegment(int index, float value){
  CRGB lerpedColor = CRGB(
    Lerp(CURRENT_COLORS[0].r, CURRENT_COLORS[2].r, value / (float)250),
    Lerp(CURRENT_COLORS[0].g, CURRENT_COLORS[2].g, value / (float)250),
    Lerp(CURRENT_COLORS[0].b, CURRENT_COLORS[2].b, value / (float)250)
  );
  if (pow(index + 1, 2) / getDivider() > valueLeft){
    for (int i = -3; i < 3; i++){ // number of leds in segment
      if (index + i < NUM_LEDS / 2 && index + i > 0){
        leds[getLeftIndex(index + i)] = lerpedColor;
        leds[getRightIndex(index + i)] = lerpedColor;    
      }  
    }
  }
}

void fullLight(int index){
  leds[getLeftIndex(index)] = CURRENT_COLORS[0];
  leds[getRightIndex(index)] = CURRENT_COLORS[0];
}
void fullLightGradient(int index){
  leds[getLeftIndex(index)] = GRADIENT_COLOR;
  leds[getRightIndex(index)] = GRADIENT_COLOR;
}
