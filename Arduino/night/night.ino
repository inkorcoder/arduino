/* Amount of leds */
short MAX_LEDS = 12;

/*
  Input pin
  It is output of operational amplifier (D2 chip)
*/
short INPUT_PIN = 13;

/*
  Detector is a complicated unit and contains three resistors,
  and operational amplifier (D2, LM358), which is playing role
  of comparator (check if voltage on photoresistor is higher)
  We don't need to power up it by the main supply,
  because of current leak.
  So we need to turn it on only when we're measuring output
  and checking whether it's time to light up or not.
*/
short ADDITIONAL_VCC_PIN = 8;

/*
  Leds on the scheme could be on very different places,
  so it is an option to make correct queue of leds for having
  opportunity to loop through it
*/
short leds[12] = {7, 6, 5, 3, 4, 2, 1, 11, 12, 9, 10};

/*
  Index of current led turned on
  Again, for the best power economy - we don't need to turn on
  all leds at same time.
  As our eyes has some sort of perception delay - we can add a delay
  between leds and if it's not too long - you won't see it.
  But it gives us a huge economy.
*/
short currentIndex = 0;
short DELAY = 2;

/* time markers */
long unsigned lastTime = 0;
long unsigned currentTime = 0;

/* lighting triggers */
boolean isLighting = false;
boolean lasWasLight = false;

/*
  lighting time. The Atmega328 will work at 8MHz, and in this mode
  it has larger time amount per tick. In my measurements it took
  7-8 times more than 1 second. I need 1 hour of lighting, it means
  60 000 milliseconds will be 480 000, but i left it as 450 just
  because this number looks pretty good
*/
long lightingTime = 450000; /* 60 * 8 */


/********************************************************************
  MAIN  LOOP
********************************************************************/
void setup() {

  /* setting up inputs and outputs */
  for (short i = 1; i <= MAX_LEDS + 2; i++) {
    pinMode(i, OUTPUT);
  }
  pinMode(INPUT_PIN, INPUT);

  /*
    also, we need to attach interrupting for making the "wake up"
    for our Atmega328
  */
  attachInterrupt(0, digitalInterrupt, FALLING);

  /* ant set first marker for time, this is startup time */
  lastTime = millis();
}



/* interruption markers */
boolean isInterrupted = false;
boolean isLongTimeout = false;


/********************************************************************
  MAIN  LOOP
********************************************************************/
void loop() {

  /* first we need - is to find out our current time */
  currentTime = millis();

  /* IMPORTANT!!! steps queue is wrong a little bit */

  /* STEP 2 */
  if (currentTime > lastTime + lightingTime) {

    /*
      if the timeout was done, or if we're loopping if we have a day
      we need to check the state and update time marker
    */
    lastTime = currentTime;

    /*
      again, we need to turn off our LEDs, because our light sensor
      based on photoresistor, and we don't need a interference from
      our shining LEDs
    */
    reset();

    /*
      again, it was disabled from final version, but you can set
      the much longer timeout for sleeping, if you need it.
      The longer sleep will be enabled after the turning off the LEDs
      after timeout we done before
    */
    (isLongTimeout) ? doSleep() : doSleep();

    /* ang click back out flag */
    isLongTimeout = false;

    /*
      STEP 1
    */
  } else {

    /*
      if controller was waked up right now, we need
      to check state. This is the trigger. If we have
      a night, - we need to set the flag, and also we
      need to update the last state flag.
      This is because we have the next logic:
      - if we have a day, - star goes sleep and wakes up
        every 1 second (in reality 8 seconds, see the top)
      - if we have a night - star is waking up and starts
        lighting. And it will be as long as "lightingTime"
        says. After that it goes sleep. And never be waked up
        for shinig until we will have a day.
      - if we have a day, or if we will turn on the light in
        the room, star will check it and the "lasWasLight" flag
        will be updated to "false"
      - and if we will turn of the light or if we again will
        have a night - the star goes shine again
    */
    if (isInterrupted) {

      /* trigger the flag for the first time when the star wakes up */
      isInterrupted = false;
      /* and checking the current state */
      checkState();

      /* after the first time the star waked up we will go through this */
    } else if (isLighting && !lasWasLight) {

      /*
        this flag is not work in final program, but you can chage it.
        This is for the longest shining if it's needed
      */
      isLongTimeout = true;

      /* we need to disable all leds */
      reset();

      /* and turn on current led */
      set(currentIndex);

      /* update index */
      currentIndex = (currentIndex >= MAX_LEDS - 1) ? 0 : ++currentIndex;

      /* and make delay */
      delay(DELAY);

      /*
        it needs for looping again and again each 8 seconds
        if we have a day. On this step we're moving to STEP 1
      */
    } else {
      lastTime = currentTime - lightingTime - 100;
    }

  }
}

/* reset all leds */
void reset() {
  for (short i = 1; i <= MAX_LEDS + 2; i++) {
    /* exept input and outpu pins */
    if ((i == INPUT_PIN) || (i == ADDITIONAL_VCC_PIN)) {
      continue;
    }
    digitalWrite(i, LOW);
  }
}

/* turn on all leds */
void setAll() {
  for (short i = 1; i <= MAX_LEDS + 2; i++) {
    /* exept input and output pins */
    if ((i == INPUT_PIN) || (i == ADDITIONAL_VCC_PIN)) {
      continue;
    }
    digitalWrite(i, HIGH);
  }
}

/* set specific led by index */
void set(short index) {
  digitalWrite(leds[index], HIGH);
}


/********************************************************************
  CHECKING THE STATE
********************************************************************/
void checkState() {

  digitalWrite(ADDITIONAL_VCC_PIN, HIGH);
  /* wait 10 milliseconds */
  delay(10);
  /* update the old flag for last state */
  lasWasLight = (isLighting) ? true : false;
  /*
    and check the current state, the output of comparator is attached
    to 4th pin on Atmega328
  */
  isLighting = (digitalRead(INPUT_PIN) == HIGH) ? true : false;
  /* turn off the power line */
  digitalWrite(ADDITIONAL_VCC_PIN, LOW);

  /*
    it's just for indication. On my project i have the one extra LED
    attached to my 10th pin. I need to blink it (turn it on, wait, and
    turn it off). So, we can see the moment of checking the state
  */
  digitalWrite(leds[5], HIGH);
  delay(50);
  digitalWrite(leds[5], LOW);
}


/********************************************************************
  DEEP SLEEP

  all this tricks is documented in Atmega328 datasheet, but it's so
  complicated. I've found it on youtube and it's more understandable
********************************************************************/
void doSleep() {

  /* set the flag on */
  isInterrupted = true;

  /* here we're need to set up our watchdog timer */
  WDTCSR = (24);
  WDTCSR = (33);
  WDTCSR |= (1 << 6);

  /* analog to digital converter */
  ADCSRA &= ~(1 << 7);

  /* clock rate setup */
  SMCR |= (1 << 2);
  SMCR |= 1;

  /* interruption setup, how much we need to sleep */
  MCUCR |= (3 << 5);
  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);

  /* and finally, we're going to sleep */
  __asm__  __volatile__("sleep");
}


/*
  this is unused, but if you need more than 8 seconds sleep,
  you can repeat it 5 times (for example)
*/
void doLongSleep() {
  for (int s = 0; s < 5; s++) {
    doSleep();
  }
}

/* this is the helper, this funciton will be launched when we're goin up */
void digitalInterrupt() {}
ISR(WDT_vect) {}
