#define POWER_ON_BTN 0
#define POWER_OFF_BTN 1
#define RELAY 2

boolean active = false,
        lastPowerOnState = false,
        lastPowerOffState = false;

unsigned long btnOnClickStartTime = 0;

void setup() {
  pinMode(POWER_ON_BTN, INPUT); 
  pinMode(POWER_OFF_BTN, INPUT); 
  pinMode(RELAY, OUTPUT); 
}

void loop() {
  if (btnOnPress()){
    active = true;
    btnOnClickStartTime = millis();
  }
  if (btnOffPress()){
    active = false;
  }
  if (btnOnPressLong()){
    active = false;
  }
  digitalWrite(RELAY, (active == true) ? HIGH : LOW);
}

boolean btnOnPress(){
  if (!lastPowerOnState && digitalRead(POWER_ON_BTN) == HIGH){
    lastPowerOnState = true;
    return true;
  } else if (lastPowerOnState && digitalRead(POWER_ON_BTN) == LOW){
    lastPowerOnState = false;
    return false;  
  } else {
    return false;
  }
}

boolean btnOffPress(){
  if (!lastPowerOffState && digitalRead(POWER_OFF_BTN) == HIGH){
    lastPowerOffState = true;
    return true;
  } else if (lastPowerOffState && digitalRead(POWER_OFF_BTN) == LOW){
    lastPowerOffState = false;
    return false;  
  } else {
    return false;
  }
}


boolean btnOnPressLong(){
  return (lastPowerOnState && digitalRead(POWER_ON_BTN) == HIGH && (btnOnClickStartTime + 1000 < millis())) ? true : false;
}
