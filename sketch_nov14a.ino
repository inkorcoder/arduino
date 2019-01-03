/* accelerometer libs */
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <EEPROM.h>

/* led matrix libs */
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#include <input.h>
#include <Vector.h>
#include <data.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

int pinCS = 10;
int numberOfHorizontalDisplays = 2;
int numberOfVerticalDisplays = 2;

const int DOTS_COUNT = 60;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

struct Vec {
  byte x;
  byte y;
};

struct Vec2 {
  float x;
  float y;
};

Vec2 dots[DOTS_COUNT];

bool grid[16][16];

Vector * menuItem = new Vector(0, 0);
Vector * pos = new Vector(0, 0);
Input * input = new Input();

int activeGame = 0;

/*
  SNAKE VARIABLES
*/

byte snakeSpeed = 60;
unsigned long lastTime, currentTime;
Vec snakePos = {3, 2};
Vec snakeHead = {7, 7};
Vec snakeTail[51];
int SNAKE_LENGTH = 51;
int snakeLength = 1;
Vec snakeFood = {20, 20};
byte SNAKE_SCORE = 0;
bool newSnakeFood = false;
bool snakeEatsFood = false;
bool SNAKE_WIN = false;
bool SNAKE_LOST = false;
int snakeScore = 0;
int SNAKE_RECORD;
/*---------------------------------------------------*/

byte animationProgress = 0;

void setup(void) {

  Serial.begin(9600);

  input->init();

  SNAKE_RECORD = EEPROM.read(1);

  matrix.setIntensity(1); // яркость от 0 до 15

  /* Initialise the sensor */
  if (!accel.begin()) {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }
  accel.setRange(ADXL345_RANGE_16_G);

  for (int i = 0; i < DOTS_COUNT; i++) {
    dots[i] = {0,0};
  }

  Serial.println("Vectors created");

  clearGrid();
  matrix.write();

  Serial.println("grid cleared");

  lastTime = millis();

}

void loop(void) {

  /* Get a new sensor event */
  sensors_event_t event;
  accel.getEvent(&event);

  clearGrid();

  input->update(event);
  if (activeGame == 0) {
    drawDots(event);
  } else if (activeGame == 1){
    if (!SNAKE_WIN && !SNAKE_LOST) {
      drawSnake();
  
    } else {
      currentTime = millis();
  
      if (animationProgress < 64) {
        animation();
        animationProgress++;
  
      } else if (currentTime > lastTime + 3000){
        SNAKE_WIN = false;
        SNAKE_LOST = false;
        animationProgress = 0;
        lastTime = currentTime;
        snakeLength = 1;
        if (snakeScore > SNAKE_RECORD){
          SNAKE_RECORD = snakeScore;
          EEPROM.put(1, snakeScore);
        }
        snakeScore = 0;
      } else {
        showScore(snakeScore);
      }
    }
  
  }

  if (input->forward) {
    activeGame++;
  }
  if (input->back) {
    activeGame--;
  }

  activeGame = (activeGame > 1) ? 1 : (activeGame < 0) ? 0 : activeGame;

  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      matrix.drawPixel(x, y, (grid[x][y] == true) ? HIGH : LOW);
    }
  }

  matrix.write();



}

void clearGrid() {
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 16; y++) {
      grid[x][y] = false;
    }
  }
}

void drawSnake() {
  currentTime = millis();

  if (input->left) {
    snakePos.x = 2;
    snakePos.y = 3;
  }
  if (input->right) {
    snakePos.x = 4;
    snakePos.y = 3;
  }
  if (input->up) {
    snakePos.x = 3;
    snakePos.y = 2;
  }
  if (input->down) {
    snakePos.x = 3;
    snakePos.y = 4;
  }

  int x = -3 + snakePos.x;
  int y = -3 + snakePos.y;


  if ((snakeEatsFood) || (currentTime > lastTime + snakeSpeed * 10)) {

    snakeHead.x += x;
    snakeHead.y += y;

    if (snakeEatsFood) {
      snakeEatsFood = false;
    }

    for (int i = snakeLength - 1; i > 0; i--) {
      snakeTail[i].x = snakeTail[i - 1].x;
      snakeTail[i].y = snakeTail[i - 1].y;
    }
    snakeTail[0].x = snakeHead.x;
    snakeTail[0].y = snakeHead.y;

    if (snakeHead.x == snakeFood.x && snakeHead.y == snakeFood.y) {
      snakeLength++;
      snakeScore++;
      newSnakeFood = false;
      snakeEatsFood = true;

      // WIN!!!!
      if (snakeLength == SNAKE_LENGTH) {
//        snakeLength = 1;
        for (int i = 0; i < SNAKE_LENGTH; i++) {
          snakeTail[i] = {};
        }
        snakeHead.x = 7;
        snakeHead.y = 7;
        SNAKE_WIN = true;
        snakeSpeed = 50;
        // push new tail segment after eating
      } else {
        snakeTail[snakeLength] = Vec {snakeFood.x, snakeFood.y};
        snakeSpeed--;
      }
    }

    lastTime = currentTime;
  }

//   Serial.println(snakeEatsFood);

  grid[snakeHead.x][snakeHead.y] = true;
  if (newSnakeFood) {
    grid[snakeFood.x][snakeFood.y] = true;
  }
  for (int i = 0; i < SNAKE_LENGTH; i++) {
    if (i < snakeLength) {
      grid[snakeTail[i].x][snakeTail[i].y] = true;
    }
  }

  /* eat food */
  if (!newSnakeFood) {
    snakeFood.x = random(1, 15);
    snakeFood.y = random(1, 15);
    newSnakeFood = true;
  }

  //  LOST
  if (hasCollision() || hasIntersection()){
    for (int i = 0; i < SNAKE_LENGTH; i++) {
      snakeTail[i] = {};
    }
    snakeHead.x = 7;
    snakeHead.y = 7;
    SNAKE_LOST = true;
    snakeSpeed = 50;
  }

}

bool hasCollision(){
  bool state = (snakeHead.x < 0 || snakeHead.x > 15 || snakeHead.y < 0 || snakeHead.y > 15);
  return state;
}

bool hasIntersection(){
  bool state = false;
  for (int i = 1; i < snakeLength; i++) {
    if ((snakeTail[i].x == snakeHead.x) && (snakeTail[i].y == snakeHead.y)){
      return true;
    }
  }
  return state;
}

/*
   DOTS
*/
void drawDots(sensors_event_t event) {
//
//  int eventX = (int)(-event.acceleration.x);
//  int eventY = (int)(event.acceleration.y);
//
//  eventX = (eventX * eventX) * ((eventX < 0) ? -1 : 1) / 10;
//  eventY = (eventY * eventY) * ((eventY < 0) ? -1 : 1) / 10;
//
//  eventX = (eventX < -15) ? -15 : (eventX > 15) ? 15 : eventX;
//  eventY = (eventY < -15) ? -15 : (eventY > 15) ? 15 : eventY;
//
//  for (int i = 0; i < DOTS_COUNT; i++) {
//    byte x = dots[i].x + eventX;
//    byte y = dots[i].y + eventY;
//
//    if (grid[x/16][y/16] != true){
//      dots[i].x += eventX;
//      dots[i].y += eventY;
//      dots[i].x = (dots[i].x < 15) ? 15 : (dots[i].x > 240) ? 240 : dots[i].x;
//      dots[i].y = (dots[i].y < 15) ? 15 : (dots[i].y > 240) ? 240 : dots[i].y;  
//      grid[dots[i].x/16][dots[i].y/16] = true;  
//    }
//  }


  Vector * pointer = new Vector(-event.acceleration.x, event.acceleration.y);
  pointer->divide(10);

  for (int i = 0; i < DOTS_COUNT; i++) {
    int X = (int)(dots[i].x + pointer->x);
    int Y = (int)(dots[i].y + pointer->y);
    if (grid[X][Y] == false) {
      dots[i].x += pointer->x;
      dots[i].y += pointer->y;
      dots[i].x = (dots[i].x < 0) ? 0 : (dots[i].x > 15) ? 15 : dots[i].x;
      dots[i].y = (dots[i].y < 0) ? 0 : (dots[i].y > 15) ? 15 : dots[i].y;
      int x = (int)dots[i].x;
      int y = (int)dots[i].y;
      grid[x][y] = true;
    } else {
      continue;
    }
  }

  delete pointer;
}


void digit(int digit, int X, int Y) {
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 5; y++) {
      if (digits[digit][y][x] == true) {
        grid[x + X][y + Y] = true;
      }
    }
  }
}

void animation() {

  for (byte x = 0; x < 16; x++) {
    for (byte y = 0; y < 16; y++) {
      if (animationProgress < 32) {
        if (x % 2 && y < animationProgress) {
          grid[x][y] = true;
        }
        if ((x & 0x01) == 0 && y < animationProgress - 14) {
          grid[x][y] = true;
        }
      } else {
        if (x % 2 && y > animationProgress - 32) {
          grid[x][y] = true;
        }
        if ((x & 0x01) == 0 && y > animationProgress - 48) {
          grid[x][y] = true;
        }
      }
    }
  }
  delay(20);
  animationProgress++;
}

void showScore(int score){

  Serial.println(score * 10);
  Serial.println(EEPROM.read(1) * 10);
  
  int first = ((score * 10) / 100) % 10;
  int second = ((score * 10) / 10) % 10;
  int third = (score * 10) % 10;
  digit(first, 3, 2);
  digit(second, 7, 2);
  digit(third, 11, 2);  

  int recordFirst = ((EEPROM.read(1) * 10) / 100) % 10;
  int recordSecond = ((EEPROM.read(1) * 10) / 10) % 10;
  int recordThird = (EEPROM.read(1) * 10) % 10;
  digit(recordFirst, 3, 9);
  digit(recordSecond, 7, 9);
  digit(recordThird, 11, 9);  
}
