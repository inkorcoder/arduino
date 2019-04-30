/**************************************************************************

        74 HC 595 (8-bit shift register)
  ┌───────────────────────────────────────┐
  │       ┌────────────────────────┐      │
  │    DS ┤                        │      │
  │  SHCP ┤ 8-STAGE SHIFT REGISTER │      │
  │    __ │                        │      │
  │    MR ┤                        │      │
  │       └────────────────────────┘      │
  │         │  │  │  │  │  │  │  ├── Q7S  │
  │       ┌────────────────────────┐      │
  │  STCP ┤ 8-BIT STORAGE REGISTER │      │
  │       └────────────────────────┘      │
  │         │  │  │  │  │  │  │  │        │
  │    __ ┌────────────────────────┐      │
  │    OE ┤      8-BIT OUTPUT      │      │
  │       └────────────────────────┘      │
  │         │  │  │  │  │  │  │  │        │
  │        Q0 Q1 Q2 Q3 Q4 Q5 Q6 Q7        │
  └───────────────────────────────────────┘

**************************************************************************/

#include <EEPROM.h>

/*
  DS - is the signal line. Here we need to paste our bits step by step.
  Important thing: we need to paste it from the end to the start.
  Because our first bit must be the last in the queue. If not -
  we can't attach another register after this, because the bits were wrong.
  In 74HC595 the number of bits is 8. [ 0 1 0 1 0 1 0 1 ]
*/
const int DS = 2;
/*
  SHCP - when we paste a bit on the DS line, we need to click on clutch,
  and the clutch will push this bit into register. We need to click it
  each time we paste a bit.
*/
const int SH = 4;
/*
  MR - master reset. Usually, this pin is attached to +5v.
  But if we need to reset the register, we need to attach it to GND line
  and turn on again. In this program we're using it to reset the registers.
*/
const int MR = 5;
/*
  STCP - is the clutch whish release all our bits to outpus. We need to click it
  once after loading all bits. Then this bits goes to output.
*/
const int ST = 3;

/* the coefficient of simple line attenuation. From 0 to 1 (smaller = faster) */
const float LINE_ATTENUATION = 0.990;

/* the coefficient of peak attenuation
   it must be slower than line, because we need to see how peak is catching up
   the line. From 0 to 1 (smaller = faster)  */
const float PEAK_ATTENUATION = 0.992;

/* this is delay after we need to move the peak down */
const int PEAK_TIMEOUT = 300;

const int BULLETS_ARRAY_LENGTH = 50;
const float BULLET_SPEED = 5;

const float CHANNEL_LENGTH = 8;

/* basically we have 7 modes for indication
   and we need some flag for preventing a "noise" of pressed button */
int mode = 0;

const int BULLET_TRIGGER = 38;
const int BULLET_TICK_TIMEOUT = 100;

float leftChannelBullets[20];
float rightChannelBullets[20];

float bulletLeft = 0;
float bulletRight = 0;

unsigned long oldTimeLeft,
         oldTimeRight,
         newTimeLeft,
         newTimeRight,
         lastButtonPress,
         bulletStartTime;


float bulletsLeft[BULLETS_ARRAY_LENGTH];
float bulletsRight[BULLETS_ARRAY_LENGTH];
unsigned long lastLeftBullet = 0;
unsigned long lastRightBullet = 0;


unsigned long oldBulletTime, newBulletTime;
boolean buttonState = false;

void setup() {
  Serial.begin(9600);
  for (int i = 2; i < 13; i++) {
    pinMode(i, OUTPUT);
  }
  pinMode(13, INPUT);
  digitalWrite(13, LOW);
  analogReference(INTERNAL);
  reset();
  oldTimeLeft = oldTimeRight = millis();
  oldBulletTime = 0;
  for (int i = 0; i < BULLETS_ARRAY_LENGTH; i++) {
    bulletsLeft[i] = 0;
    bulletsRight[i] = 0;
  }

  if (EEPROM.read(0)) {
    mode = EEPROM.read(0);
  }

  Serial.print("Start");
}

int i = 1;


float leftChannelValue = 0,
      rightChannelValue = 0;
float leftChannelPeak = 0,
      rightChannelPeak = 0;

int j = 0;

int bulletLeftTick = 0,
    bulletRightTick = 0;

void loop() {

  newTimeLeft = newTimeRight = millis();

  buttonState = digitalRead(13);
  if ((buttonState == HIGH) && (newTimeLeft > lastButtonPress + 200)) {
    goToNextMode();
    lastButtonPress = millis();
  }

  float newLeftChannelValue = analogRead(2);
  float newRightChannelValue = analogRead(3);

  if (leftChannelValue < newLeftChannelValue) {
    leftChannelValue = newLeftChannelValue;
  }
  if (rightChannelValue < newRightChannelValue) {
    rightChannelValue = newRightChannelValue;
  }

  if (leftChannelPeak < newLeftChannelValue) {
    leftChannelPeak = newLeftChannelValue;
    oldTimeLeft = newTimeLeft;
  }
  if (rightChannelPeak < newRightChannelValue) {
    rightChannelPeak = newRightChannelValue;
    oldTimeRight = newTimeRight;
  }

  resetDip(MR);

  if (mode == 0) {
    drawSimpleLine();

  } else if (mode == 1) {
    drawLineWithPeak();

  } else if (mode == 2) {
    drawOnePoint();

  } else if (mode == 3) {
    drawMultiLine();

  } else if (mode == 4) {
    drawMarkedLine();

  } else if (mode == 5) {
    drawLineDown();

  } else if (mode == 6) {
    //    drawDoubleMultiLine();
    drawBullet();

  } else if (mode == 7) {
    drawFire();
  }

  indicateMode();

}

/**************************************************************************
*                                                                         *
                       M A I N   F U N C T I O N S
*                                                                         *
**************************************************************************/

/* MAIN FUNCITON
   Going to next mode
*/
void goToNextMode() {
  mode++;
  if (mode > 7) {
    mode = 0;
  }
  EEPROM.write(0, mode);
}

/* MAIN FUNCITON
   Resetting specific DIP. The *595's docs told us that we need to set a zero
   and after that set a HIGH value
*/
void resetDip(int dipMr) {
  digitalWrite(dipMr, LOW);
  digitalWrite(dipMr, HIGH);
}

/* MAIN FUNCITON
   Also, from docs we know that we need to click a latch if we need
   to set push value into shift register
*/
void applyShiftRegister() {
  digitalWrite(SH, HIGH);
  digitalWrite(SH, LOW);
}

/* MAIN FUNCITON
   Also, the same^ for storage register
*/
void applyStorageRegister() {
  digitalWrite(ST, HIGH);
  digitalWrite(ST, LOW);
}

/* MAIN FUNCITON
   Resetting all of the LEDS
*/
void reset() {
  for (float d = CHANNEL_LENGTH; d > 0; d--) {

    // push new value
    digitalWrite(DS, HIGH);
    applyShiftRegister();
  }

  // release all of the values
  applyStorageRegister();
}


/**************************************************************************
*                                                                         *
                                 M O D E S
*                                                                         *
**************************************************************************/


/* SIMPLE LINE                                                   [ mode 1 ]
   Basic line, from top to bottom

    ─       ─
    ─       ─
    ─       ─
   ───     ───
   ───     ───
   ───     ───
   ───     ───
*/
void drawSimpleLine() {

  // for each channel
  for (int channel = 0; channel < 2; channel++) {
    float value = (channel == 0) ? leftChannelValue : rightChannelValue;

    // for each LED
    for (float d = CHANNEL_LENGTH; d > 0; d--) {

      // as we can see from *595's docs - we need to set a value on DS port
      // and click the latch. This bit will be stored in shift register
      digitalWrite(DS, (d == 1 || (d * d) / 10 < value) ? LOW : HIGH);

      // click the latch
      applyShiftRegister();
    }
  }

  // after all we need to release the data into storage register
  applyStorageRegister();

  // also after each step we need attenuate our values, for
  // actually seeing the animation
  leftChannelValue *= LINE_ATTENUATION;
  rightChannelValue *= LINE_ATTENUATION;
}
/**************************************************************************/





/* LINE WITH PEAKS                                                [ mode 2 ]

   Basic line, from top to bottom, but it also has a "peaks", which is
   the highest point of signal. It looks "freezed" some time and also drops
   down if signal volume is lower than it.

    ─       ─
   ───     ─── <- peak
    ─       ─
    ─       ─
   ───     ───
   ───     ───
   ───     ───
*/
float drawLineWithPeak() {

  // for each channel
  for (int channel = 0; channel < 2; channel++) {
    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
    float peak = (channel == 0) ? leftChannelPeak : rightChannelPeak;

    // for each LED
    for (float d = CHANNEL_LENGTH; d > 0; d--) {
      digitalWrite(DS, (
                     (((d * d) / 10 <= peak) && ((d + 1) * (d + 1)) / 10 > peak) ||
                     ((d * d) / 10 < value) ||
                     (d == 1 && peak < 0.1)
                   ) ? LOW : HIGH);
      // push
      applyShiftRegister();
    }
  }

  // release
  applyStorageRegister();

  // attenuation. But in this case we need to make it faster, for better effect
  // of dividing the line from peak. You can replace it as
  //  leftChannelValue *= LINE_ATTENUATION;
  leftChannelValue *= (LINE_ATTENUATION / 1.01);
  rightChannelValue *= (LINE_ATTENUATION / 1.01);

  // and here we need to attenuate the peaks. To do that - we need count some
  // time from last highest point and if it's larger - then we need to move
  // the peaks down slower than line, that's why we have a two variables -
  // for lines and peaks attenuation
  if (newTimeLeft > oldTimeLeft + PEAK_TIMEOUT) leftChannelPeak *= PEAK_ATTENUATION;
  if (newTimeRight > oldTimeRight + PEAK_TIMEOUT) rightChannelPeak *= PEAK_ATTENUATION;
}
/**************************************************************************/





/* ONLY ONE SEGMENT AS THE HIGHEST                                [ mode 3 ]

   It's not a line, but is it only the highest point of it.
    ─       ─
   ───     ─── <- top
    ─       ─
    ─       ─
    ─       ─
    ─       ─
    ─       ─
*/
void drawOnePoint() {

  // for each channel
  for (int channel = 0; channel < 2; channel++) {
    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
    float peak = (channel == 0) ? leftChannelPeak : rightChannelPeak;

    // for each LED
    for (float d = CHANNEL_LENGTH; d > 0; d--) {
      digitalWrite(DS, (
                     ((((d - 1) * (d - 1)) / 10 <= value) && (d * d) / 10 > value) ||
                     (value < 0.1 && d == 1)
                   ) ? LOW : HIGH);
      // push
      applyShiftRegister();
    }
  }

  // release
  applyStorageRegister();

  // attenuation
  leftChannelValue *= LINE_ATTENUATION;
  rightChannelValue *= LINE_ATTENUATION;
}
/**************************************************************************/





/* MULTILINE                                                      [ mode 4 ]

   This is line from the cebter of tower
    ─       ─
   ───     ───
   ───     ───
   ───     ─── <- center
   ───     ───
   ───     ───
    ─       ─
*/
void drawMultiLine() {

  // the middle of the tower
  float half = CHANNEL_LENGTH / 2;

  // for each channel
  for (int channel = 0; channel < 2; channel++) {
    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
    float peak = (channel == 0) ? leftChannelPeak : rightChannelPeak;

    // for each LED
    for (float d = CHANNEL_LENGTH; d > 0; d--) {

      // if
      if (d >= half) {
        digitalWrite(DS, (((d - half) * (d - half)) / 2 < value) ? LOW : HIGH);
      } else {
        digitalWrite(DS, (((half - d) * (half - d)) / 2 < value) ? LOW : HIGH);
      }

      // push
      applyShiftRegister();
    }
  }

  // release
  applyStorageRegister();

  // attenuation
  leftChannelValue *= (LINE_ATTENUATION / 1.01);
  rightChannelValue *= (LINE_ATTENUATION / 1.01);
}
/**************************************************************************/






/* MARKED LINE                                                    [ mode 5 ]

   Line which contains some markers, and line goes thtough it
    ─       ─
   ───     ─── <- mark
    ─       ─
   ───     ─── <- mark
    ─       ─
   ───     ─── <- signal
   ───     ─── <- signal
*/
void drawMarkedLine() {

  // for each channel
  for (int channel = 0; channel < 2; channel++) {
    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
    float peak = (channel == 0) ? leftChannelPeak : rightChannelPeak;

    // for each LED
    for (float d = CHANNEL_LENGTH; d > 0; d--) {
      digitalWrite(DS, ((int)d % 8 == 0 || (d * d) / 10 < value) ? LOW : HIGH);
      applyShiftRegister();
    }
  }

  // release
  applyStorageRegister();

  // attenuation
  leftChannelValue *= LINE_ATTENUATION;
  rightChannelValue *= LINE_ATTENUATION;
}
/**************************************************************************/





/* LINE TO DOWN                                                   [ mode 6 ]
   Basic line, from top to bottom

   ───     ───
   ───     ───
   ───     ───
   ───     ───
    ─       ─
    ─       ─
    ─       ─
*/
void drawLineDown() {

  // for each channel
  for (int channel = 0; channel < 2; channel++) {
    float value = (channel == 0) ? leftChannelValue : rightChannelValue;

    // for each LED
    for (float d = 1; d < CHANNEL_LENGTH + 1; d++) {

      digitalWrite(DS, (d == 1 || (d * d) / 10 < value) ? LOW : HIGH);

      // push
      applyShiftRegister();
    }
  }

  // release
  applyStorageRegister();

  // attenuation
  leftChannelValue *= LINE_ATTENUATION;
  rightChannelValue *= LINE_ATTENUATION;
}
/**************************************************************************/





/* MULTILINE (TWO PARTS)                                         [ mode 7 ]

   The same as multiline, but it has two centers
    ─       ─
   ───     ─── <- center
   ───     ───
    ─       ─
   ───     ─── <- center
   ───     ───
    ─       ─
*/
//void drawDoubleMultiLine() {
//
//  // the middle of the tower
//  float quarter = CHANNEL_LENGTH / 4;
//
//  // for each channel
//  for (int channel = 0; channel < 2; channel++) {
//    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
//    float peak = (channel == 0) ? leftChannelPeak : rightChannelPeak;
//
//    // for each LED
//    for (float d = CHANNEL_LENGTH; d > 0; d--) {
//
//      // 4/4
//      if (d >= quarter * 3) {
//        digitalWrite(DS, (((d - quarter * 2) * (d - quarter * 2)) / 3 < value) ? LOW : HIGH);
//
//        // 3/4
//      } else if ((d >= quarter * 2) && (d < quarter * 3)) {
//        digitalWrite(DS, (((quarter * 3 - d) * (quarter * 3 - d)) / 1 < value) ? LOW : HIGH);
//
//        // 2/4
//      } else if (d >= quarter && d < quarter * 2) {
//        digitalWrite(DS, (((d - quarter) * (d - quarter)) / 1 < value) ? LOW : HIGH);
//
//        // 1/4
//      } else {
//        digitalWrite(DS, (((quarter - d) * (quarter - d)) / 1 < value) ? LOW : HIGH);
//      }
//
//      // push
//      applyShiftRegister();
//    }
//  }
//
//  // release
//  applyStorageRegister();
//
//  // attenuation
//  leftChannelValue *= LINE_ATTENUATION;
//  rightChannelValue *= LINE_ATTENUATION;
//}
/**************************************************************************/





/* BULLETS                                                        [ mode 8 ]
   This is bullets emulation. It looks like a bullet fires from bottom and
   moves to top

    ─      ─
    ^      ^
   ───    ─── <- it moves from bottom to top
    ^      ^
    ─      ─
    ─      ─
    ─      ─
*/
//float drawBullet() {
//
//  // for each channel
//  for (int channel = 0; channel < 2; channel++) {
//
//    // we need to define a links for our data variables
//    // this is the value
//    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
//    // array with bullets
//    float* bullets = (channel == 0) ? leftChannelBullets : rightChannelBullets;
//    // current free bullet
//    int newBullet = getFreeCell(bullets);
//    // current tick for current bullet. It moves the bullet
//    int* tick = (channel == 0) ? bulletLeftTick : bulletRightTick;
//
//    // if we have a free bullet and specific volume of signal and also timeout was done
//    if (newBullet >= 0 && value > BULLET_TRIGGER && tick > BULLET_TICK_TIMEOUT) {
//      // we're activating a bullet
//      bullets[newBullet] = 1;
//      // and reset the tick
//      tick = 0;
//    }
//
//    // for each LED
//    for (float d = CHANNEL_LENGTH; d > 0; d--) {
//
//      // if it's the first one - then we activate it and continute
//      if (d == 1) {
//        digitalWrite(DS, LOW);
//        applyShiftRegister();
//        continue;
//      }
//
//      // otherwise - we're finding a moving bullet and update it
//      if (isMoving(d, bullets)) {
//        bullets[(int)d] = (bullets[(int)d] > CHANNEL_LENGTH * 30) ? 0 : bullets[(int)d] + BULLET_SPEED;
//      }
//
//      // flag for checking if there is any moving bullet on this LED
//      boolean isActive = false;
//
//      // for each bullets for this channel
//      for (int j = 0; j < BULLETS_ARRAY_LENGTH; j++) {
//
//        // if there is any moving bullet on this LED
//        if ((bullets[j] >= d * d) && (bullets[j] <= (d + 1) * (d + 1))) {
//          digitalWrite(DS, LOW);
//
//          // break the loop, we don't need to handle multiple bullets on one LED,
//          // because it's already flashed
//          isActive = true;
//          applyShiftRegister();
//          break;
//        }
//      }
//
//      // if there is no bullets on LED - turn it off
//      if (!isActive) {
//        digitalWrite(DS, HIGH);
//        applyShiftRegister();
//      }
//    }
//  }
//
//  // release
//  applyStorageRegister();
//
//  // updating moving bullets for left channel
//  leftChannelValue = 0;
//  bulletLeftTick++;
//  if (bulletLeftTick > 100) {
//    bulletLeftTick = 0;
//  }
//
//  // updating moving bullets for right channel
//  rightChannelValue = 0;
//  bulletRightTick++;
//  if (bulletRightTick > 100) {
//    bulletRightTick = 0;
//  }
//}



void drawBullet() {

  for (int channel = 0; channel < 2; channel++) {

    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
    float* bullets = (channel == 0) ? bulletsLeft : bulletsRight;
    float lastBullet = (channel == 0) ? lastLeftBullet : lastRightBullet;

    int bulletIndex = getFreeCell(bullets);

    unsigned long timeNow = millis();

    if ((value > BULLET_TRIGGER) && (bullets[bulletIndex] == 0) && (timeNow > lastBullet + BULLET_TICK_TIMEOUT)) {
      bullets[bulletIndex] = 1;
      if (channel == 0) {
        lastLeftBullet = timeNow;
      } else if (channel == 1) {
        lastRightBullet = timeNow;
      }
    }

    for (float d = CHANNEL_LENGTH; d > 0; d--) {
      boolean isActive = false;
      for (int j = 0; j < BULLETS_ARRAY_LENGTH - 1; j++) {
        if ((bullets[j] >= d * d) && (bullets[j] < (d + 1) * (d + 1))) {
          digitalWrite(DS, LOW);
          isActive = true;
          applyShiftRegister();
          break;
        }
      }
      if (!isActive) {
        digitalWrite(DS, HIGH);
        applyShiftRegister();
      }
    }

  }


  applyStorageRegister();

  updateBullets(bulletsLeft);
  updateBullets(bulletsRight);

  leftChannelValue = 0;
  rightChannelValue = 0;

}
/**************************************************************************/

float tongueLeft = 0;
float tongueRight = 0;
float tongueLength = 0;

void drawFire() {

  for (int channel = 0; channel < 2; channel++) {


    float value = (channel == 0) ? leftChannelValue : rightChannelValue;
    float tongue = (channel == 0) ? tongueLeft : tongueRight;
    float lastBullet = (channel == 0) ? lastLeftBullet : lastRightBullet;

    unsigned long timeNow = millis();

    if ((value > pow(CHANNEL_LENGTH, 2) / 10) && (timeNow > lastBullet + BULLET_TICK_TIMEOUT)) {

      tongueLength = random(250, pow(CHANNEL_LENGTH, 2));
      if (channel == 0) {
        tongueLeft = pow(CHANNEL_LENGTH, 2) / 4;
        lastLeftBullet = timeNow;
      } else if (channel == 1) {
        tongueRight = pow(CHANNEL_LENGTH, 2) / 4;
        lastRightBullet = timeNow;
      }
    }

    for (float d = CHANNEL_LENGTH; d > 0; d--) {

      if (d > CHANNEL_LENGTH / 2) {
        if ((tongue >= d * d) && (tongue < (d + 1) * (d + 1))) {
          digitalWrite(DS, LOW);
        } else {
          digitalWrite(DS, HIGH);
        }
        applyShiftRegister();
      } else {
        digitalWrite(DS, (d == 1 || (d * d) / 2.4 <= value) ? LOW : HIGH);
        applyShiftRegister();
      }
    }

  }


  applyStorageRegister();

  updateTongues();

  leftChannelValue *= (LINE_ATTENUATION / 1.1);
  rightChannelValue *= (LINE_ATTENUATION / 1.1);

}
/**************************************************************************/




/* HELPER
   For finding free cell in bullets array
*/
int getFreeCell(float arr[]) {
  for (int i = 0; i < BULLETS_ARRAY_LENGTH; i++) {
    if (arr[i] < 1) {
      return i;
    }
  }
  return -1;
}
/**************************************************************************/


/* HELPER
   Check if bullet is moving
*/
boolean isMoving(int index, float bullets[]) {
  if (bullets[index] > 1) {
    return true;
  }
  return false;
}

void updateBullets(float bullets[]) {
  for (int i = 0; i < BULLETS_ARRAY_LENGTH; i++) {
    if (bullets[i] >= 1 && bullets[i] < pow(CHANNEL_LENGTH, 2)) {
      bullets[i] += BULLET_SPEED * 2;
    } else {
      bullets[i] = 0;
    }
  }
}

void updateTongues() {
  if (tongueLeft >= pow(CHANNEL_LENGTH, 2) / 4 && tongueLeft < tongueLength) {
    tongueLeft += BULLET_SPEED * 2;
  } else {
    tongueLeft = 0;
  }
  if (tongueRight >= pow(CHANNEL_LENGTH, 2) / 4 && tongueRight < tongueLength) {
    tongueRight += BULLET_SPEED * 2;
  } else {
    tongueRight = 0;
  }
}
/**************************************************************************/







int DS2 = 9,
    ST2 = 8,
    SH2 = 7,
    MR2 = 6;

void indicateMode() {
  resetDip2(MR2);
  for (int i = 0; i < 8; i++) {
    int pin = 7 - i;
    digitalWrite(DS2, (mode == pin) ? HIGH : LOW);
    applyShiftRegister2();
  }
  applyStorageRegister2();
}

void resetDip2(int dipMr) {
  digitalWrite(dipMr, LOW);
  digitalWrite(dipMr, HIGH);
}

void applyShiftRegister2() {
  digitalWrite(SH2, HIGH);
  digitalWrite(SH2, LOW);
}

void applyStorageRegister2() {
  digitalWrite(ST2, HIGH);
  digitalWrite(ST2, LOW);
}
