/* time markers */
long unsigned
     lastTime = 0,
     currentTime = 0;

/* lighting triggers */
boolean
     isLighting = false,
     lasWasLight = false;

/*
  lighting time. The Atmega328 will work at 8MHz, and in this mode
  it has larger time amount per tick. In my measurements it took
  7-8 times more than 1 second. I need 1 hour of lighting, it means
  60 000 milliseconds will be 480 000, but i left it as 450 just
  because this number looks pretty good
*/
long lightingTime = 450000; /* 60 * 8 */


/********************************************************************
  S E T U P
********************************************************************/
void setup() {

  /* on this step we need to set up our inputs and outputs */
  pinMode(2, OUTPUT);
  pinMode(4, INPUT);
  for (int j = 5; j < 10; j++) {
    pinMode(j, OUTPUT);
  }

  /*
    also, we need to attach interrupting for making the "wake up"
    for our Atmega328
  */
  attachInterrupt(0, digitalInterrupt, FALLING);

  /* ant set first marker for time, this is startup time */
  lastTime = millis();
}

/* interruption markers */
boolean
     isInterrupted = false,
     isLongTimeout = false;


/********************************************************************
  M A I N  L O O P
********************************************************************/
/* small counter for the main loop */
int i = 5;
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
    disableLeds();

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

      /* we need to disable all LEDs */
      disableLeds();
      delayMicroseconds(100);

      /*
        and, we're initialize the small counter previously
        each time we're going through this statement we will
        turn on only one LED, and after that trick our consumption
        will be really small. Don't forget: this star will work on
        3 AA batteries
      */
      digitalWrite(i, HIGH);
      i++;
      /*
        5...9 it's just because we have digital outputs
        from 5 to 9 on our Atmega328
      */
      i = (i > 9) ? 5 : i;

      /*
        it needs for looping again and again each 8 seconds
        if we have a day. On this step we're moving to STEP 1 
      */
    } else {
      lastTime = currentTime - lightingTime - 100;
    }

  }

}


/********************************************************************
  C H E C K I N G  T H E  S T A T E
********************************************************************/
void checkState() {
  /*
    in my scheme i have a power line for my comparator and
    photoresistor circle. And it attached to 2nd pin on Atmega328
    So, i need to turn it on
  */
  digitalWrite(2, HIGH);
  /* wait 10 milliseconds */
  delay(10);
  /* update the old flag for last state */
  lasWasLight = (isLighting) ? true : false;
  /*
    and check the current state, the output of comparator is attached
    to 4th pin on Atmega328
  */
  isLighting = (digitalRead(4) == 1) ? true : false;
  /* turn off the power line */
  digitalWrite(2, LOW);

  /*
    it's just for indication. On my project i have the one extra LED
    attached to my 10th pin. I need to blink it (turn it on, wait, and
    turn it off). So, we can see the moment of checking the state
  */
  digitalWrite(10, HIGH);
  delay(20);
  digitalWrite(10, LOW);
}


/********************************************************************
  D E E P  S L E E P

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

/*
  turning off all LEDs, again, i have the LEDs attached
  to 5...9 pins on Atmega
*/
void disableLeds() {
  for (int j = 5; j < 10; j++) {
    digitalWrite(j, LOW);
  }
}

/* this is the helper, this funciton will be launched when we're goin up */
void digitalInterrupt() {}
ISR(WDT_vect) {}
