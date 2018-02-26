#include <Encoder.h>
#include <EEPROM.h>

// after including we need to initialize our Encoder on 2nd and 3d pins
Encoder myEnc(2, 3);

// we need two variables for comparing and finding the rotation direction
int value, oldValue;

// also we need to handle "Mute" by pressing the button
int buttonPress, lastButtonState;
bool isMuted = false;

// **********************************************************************
// SETUP
// **********************************************************************
void setup() {

  // first of all, we need to clear ROM memory, because we will save the
  // values into it and after launching we need to take this value to
  // restore the state. By default all bits in memory are setted to some
  // values, so we just clear it all
  if (EEPROM.read(0) == 255) {

    // write in bit <0> value of <0>
    EEPROM.write(0, 0);
  }

  // strting the output by 9600 times pre second (it's just speed),
  // 9600 - is optimal for most cases
  Serial.begin(9600);

  // and initial value from ROM
  value = EEPROM.read(0);

  // and initial old value
  oldValue = value - 1;

  // here we're doing the setup for our pins, for LED indication
  // it will be 5 LEDs
  // previously, we use Encoder on 2nd and 3d pins, so we need to
  // attach LEDs on next free pins
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  // but this pin will be input from button, and i will trigger
  // state from controling to muted state, and back
  pinMode(9, INPUT);

  // print current value
  // here our system can recive messages
  Serial.println(value);
}

// **********************************************************************
// MAIN LOOP
// **********************************************************************

// specifying the position
int oldPosition = 0;

// setup the loop counter
int i = 0;

// loop
void loop() {

  // if button change state - we need to save it to variable
  if (buttonPress != digitalRead(9)) {
    buttonPress = digitalRead(9);
  }

  // reading the encoder position
  int newPosition = myEnc.read();

  // and finding the direction (left or right)
  int dir = newPosition - oldPosition;

  // if direction is greater than 0 then we increment value
  if (dir > 0) {
    value--;
  }
  // if less than 0, dercrement it
  if (dir < 0) {
    value++;
  }

  // clamping value between 0 and 64
  value = (value > 128) ? 128 : (value < 0) ? 0 : value;

  // if we have only one change in trigger - then we send the message
  // because encoder can have 4 changes in one time, we need to handle it
  if (value == oldValue + 1 || value == oldValue - 1) {
    Serial.println((isMuted) ? 0 : value);
    oldValue = value;

    // and save new value to ROM
    EEPROM.write(0, value);
  }

  // also if we presws the button
  if ((lastButtonState != buttonPress) && (buttonPress == HIGH)) {

    // we change the state
    isMuted = !isMuted;

    // send it to system
    Serial.println((isMuted) ? 0 : value);

    // and save it to ROM
    EEPROM.write(0, value);

    // and restore values if unmute the system
    oldValue = value - 1;

    // and clear counter for LED indication
    if (isMuted) {
      i = 0;
    }
  }

  // save to old position and button state
  oldPosition = newPosition;
  lastButtonState = buttonPress;

  // by default our LEDs shows us how high is volume level
  if (!isMuted) {
    digitalWrite(4, (value > 0) ? HIGH : LOW);
    digitalWrite(5, (value >= 25) ? HIGH : LOW);
    digitalWrite(6, (value >= 50) ? HIGH : LOW);
    digitalWrite(7, (value >= 75) ? HIGH : LOW);
    digitalWrite(8, (value >= 100) ? HIGH : LOW);

  // but if we mute the system then LEDs will be "float"
  // from left to right (it looks interesting)
  } else {
    digitalWrite(4, (i > 0 && i < 2000) ? HIGH : LOW);
    digitalWrite(5, (i > 2000 && i < 4000) ? HIGH : LOW);
    digitalWrite(6, (i > 4000 && i < 6000) ? HIGH : LOW);
    digitalWrite(7, (i > 6000 && i < 8000) ? HIGH : LOW);
    digitalWrite(8, (i > 8000 && i < 10000) ? HIGH : LOW);
  }

  // reset the counter to start animation from the begining
  i = (i < 20000) ? ++i : 0;
}

