#include <Wire.h>
#include <RtcDS3231.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include "FastLED.h"
#include "FS.h"
#include <EEPROM.h>

// pins for working with
#define DIN_MINUTES_1   7 // [hh:Mm]
#define DIN_MINUTES_2   8 // [hh:mM]
#define DIN_DIVIDER     6 // [hh|mm]
#define DIN_HOURS_1     4 // [Hh:mm]
#define DIN_HOURS_2     5 // [hH:mm]

// actually can't be changed because we don't have other choices
// leds per digits group
#define LEDS_COUNT 10

// indexes for storing data in EEPROM
#define R_EEPROM_INDEX 100
#define G_EEPROM_INDEX 101
#define B_EEPROM_INDEX 102
#define NIGHT_MODE_EEPROM_INDEX 102
#define SHOW_DATE_EEPROM_INDEX 102
#define SSID_EEPROM_INDEX 200
#define PASSWORD_EEPROM_INDEX 300

/*
  Default network credentials
  will be used if connection to home network was failed
  and we need to set up our own network
*/
const char* DEFAULT_SSID = "Clock-Hotspot";
const char* DEFAULT_PASSWORD = "12345678";

/*
  arrays for leds colors storing
*/
CRGB Minutes1[LEDS_COUNT];
CRGB Minutes2[LEDS_COUNT];
CRGB Divider[LEDS_COUNT];
CRGB Hours1[LEDS_COUNT];
CRGB Hours2[LEDS_COUNT];
int parallelIndexes[10] = {5,4,6,3,7,2,8,1,9,0};
const boolean USE_PARALLEL = true;

/*
  initialize settings
*/
boolean NIGHT_MODE = false;
boolean SHOW_DATE = false;
byte FADING_LEVEL = 250;
CRGB GLOBAL_COLOR = CRGB(255, 0, 0);

/*
  set up server and clock module
*/
RtcDS3231<TwoWire> Rtc(Wire);
ESP8266WebServer server(80);

// current credentials
String ssid = "";
String password = "";


// Define NTP Client to sync time via internet
// uncomment function call below if needed
const long utcOffsetInSeconds = 10800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

// counter for connection fails
int connectionFails = 0;

void setup() {

  // initialize serial for debug, EEPROM (for memory), and SPIFFS (for server)
  Serial.begin(115200);
  EEPROM.begin(512);
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // take all settings from memory
  GLOBAL_COLOR.r = EEPROM.read(R_EEPROM_INDEX);
  GLOBAL_COLOR.g = EEPROM.read(G_EEPROM_INDEX);
  GLOBAL_COLOR.b = EEPROM.read(B_EEPROM_INDEX);
  NIGHT_MODE = (EEPROM.read(NIGHT_MODE_EEPROM_INDEX) == 1) ? true : false;
  SHOW_DATE = (EEPROM.read(SHOW_DATE_EEPROM_INDEX) == 1) ? true : false;

  // take credentials from memory
  ssid = readStringFromMemory(SSID_EEPROM_INDEX);
  password = readStringFromMemory(PASSWORD_EEPROM_INDEX);

  delay(500);
 
  // intialize clock module
  Rtc.Begin();
  RtcDateTime now = Rtc.GetDateTime();
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  /*
    if there is no credentials - startup our own network
  */
  if (ssid == "" || password == "") {
    Serial.print("We don't have credentials, starting own network...");
    // create wifi spot
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("HotSpt IP:");
    Serial.println(myIP);

    // just to be sure that it's doesn't work - delete all credentials
    saveStringIntoMemory(SSID_EEPROM_INDEX, "");
    saveStringIntoMemory(PASSWORD_EEPROM_INDEX, "");

    /*
      if we have have credentials - connect to home network
    */
  } else {
    Serial.println("We have credentials, trying to connect...");
    Serial.print("Name: ");
    Serial.print(ssid);
    Serial.print(", password: ");
    Serial.println(password);

    // setting up our fixed IP
    WiFi.begin(ssid, password);
    IPAddress ip(192, 168, 0, 160);
    IPAddress gateway(192, 168, 0, 254);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(ip, gateway, subnet);

    // tyuing to connect
    // it may take several seconds, so we have 10 chances (10 seconds)
    // loop will continue if there was no success
    // otherwise - it will be breaked
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("connection try ");
      Serial.println(connectionFails);

      // if we're inside of loop - it means no succesfull connection
      // and we trying again
      connectionFails++;

      // if there was too many unsuccessfull takes
      if (connectionFails > 10) {
        Serial.println("too many fails to connect. resetting..");
        delay(500);

        // we're clearing credentials and will try put it again
        saveStringIntoMemory(SSID_EEPROM_INDEX, "");
        saveStringIntoMemory(PASSWORD_EEPROM_INDEX, "");

        ESP.restart();
        break;
      }
      delay(1000);
    }

    // if we're here - it means everything is okay and we connected succesfully
    WiFi.mode(WIFI_STA);
    Serial.print(WiFi.localIP());
  }

  /*
    set up static files for server
  */
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/jquery.js", SPIFFS, "/jquery.js");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/style.css", SPIFFS, "/style.css");

  /*
    here we need to set up credentials we'we taken
    an save it into memory, so it will be used after rebooting
  */
  server.on("/set_credentials", []() {
    Serial.println("Received new credentials");
    Serial.print("Ssid: ");
    Serial.println(server.arg("ssid"));
    Serial.print("password: ");
    Serial.println(server.arg("password"));

    // save to EEPROM - so we will use it to connect to our home network
    saveStringIntoMemory(SSID_EEPROM_INDEX, server.arg("ssid"));
    saveStringIntoMemory(PASSWORD_EEPROM_INDEX, server.arg("password"));

    String Response = (String)millis();
    server.send(200, "text/html", Response);
    delay(100);
    
    ESP.restart();
  });

  /*
    here we need to clear our network credentials
    and reboot device
  */
  server.on("/reset_connection", []() {
    Serial.println("Resetting connecition...");

    // set nothing to EEPROM - so we will put it from scratch
    saveStringIntoMemory(SSID_EEPROM_INDEX, "");
    saveStringIntoMemory(PASSWORD_EEPROM_INDEX, "");

    String Response = (String)millis();
    server.send(200, "text/html", Response);
    delay(100);

    ESP.restart();
  });

  /*
    Setting up server requests for configuring
  */
  server.on("/night-mode", HandleNightMode);
  server.on("/display-mode", HandleDisplayMode);
  server.on("/global-color", HandleGlobalColor);
  server.on("/synchronize-time", HandleTimeSync);
  server.on("/get-settings", GetSettings);
  server.begin();

  /*
    initialize led strips
  */
  FastLED.addLeds<NEOPIXEL, DIN_MINUTES_1>(Minutes1, LEDS_COUNT);
  FastLED.addLeds<NEOPIXEL, DIN_MINUTES_2>(Minutes2, LEDS_COUNT);
  FastLED.addLeds<NEOPIXEL, DIN_DIVIDER>(Divider, LEDS_COUNT);
  FastLED.addLeds<NEOPIXEL, DIN_HOURS_1>(Hours1, LEDS_COUNT);
  FastLED.addLeds<NEOPIXEL, DIN_HOURS_2>(Hours2, LEDS_COUNT);

  /*
   * uncomment if needed (see below)
    synchronizeWithInternet();
  */
}

/*
  index is used for divider blinking
  we have 20 iterations in total
  for 10 of them - divider is lighting up
  for other 10 - if fades down
*/
int indx = 0;

/*
  array for storing current animation step for animations
  -1 - means it is not aninamted
  0...9 - means animated
  this index will be showed when animation is running
*/
int animations[4] = { -1, -1, -1, -1};

/*
  array for storing last used digits
  for example we have 15:24 (hh:mm)
  [1,5,2,4]
*/
int lastTime[4];

/*
  the same but for current time
*/
int currentMinute1, currentMinute2, currentHour1, currentHour2;

int ddd = 0;
void loop() {

  // take actual time from clock module
  RtcDateTime now = Rtc.GetDateTime();
  uint8_t seconds = now.Second();

  // HUE rotation
  // color of divider indicates how many seconds passed
  // red - yellow - green - aqua - blue - purple - pink - red
  // ^0s                         ^30s              ^50s   ^60s
  int hue = (int)seconds;
  float precent = hue / 60.0;

  // all leds to black
  reset();

  // take current digits
  currentMinute1 = getDigit((SHOW_DATE) ? now.Month() : now.Minute(), 0),
  currentMinute2 = getDigit((SHOW_DATE) ? now.Month() : now.Minute(), 1),
  currentHour1 = getDigit((SHOW_DATE) ? now.Day() : now.Hour(), 0),
  currentHour2 = getDigit((SHOW_DATE) ? now.Day() : now.Hour(), 1);

  // check if can animate - and do it
  checkIfCanAnimate(Hours1, 0, currentHour1);      // <H>h
  checkIfCanAnimate(Hours2, 1, currentHour2);      // h<H>
  checkIfCanAnimate(Minutes1, 2, currentMinute1);  // <M>m
  checkIfCanAnimate(Minutes2, 3, currentMinute2);  // m<M>

  // update current digits indexes
  lastTime[0] = currentHour1;
  lastTime[1] = currentHour2;
  lastTime[2] = currentMinute1;
  lastTime[3] = currentMinute2;

  // process digits group
  // find explaining below
  processGroup(Hours1, 0, currentHour1);      // <H>h
  processGroup(Hours2, 1, currentHour2);      // h<H>
  processGroup(Minutes1, 2, currentMinute1);  // <M>m
  processGroup(Minutes2, 3, currentMinute2);  // m<M>

  // divider (seconds)
  if (indx > 10 && !SHOW_DATE) Divider[9] = CHSV(precent * 255, 255, 255);
  // divider (date)
  if (SHOW_DATE) Divider[4] = GLOBAL_COLOR;

  // if we have night mode enabled - we need to fade down all leds
  // just to nod disturb someone at night (it lights very bright)
  if (NIGHT_MODE) {
    fade();
  }

  // release all leds
  FastLED.show();

  // index for detecting blinking of divider
  // it takes 20 times, and only half of that should be displayed
  // so, 0-10 - show, 10-20 - hide
  // we can change it as we need
  indx++;
  if (indx > 19) {
    indx = 0;
  }

  // start server
  server.handleClient();

  // we cant call time module too often, so it should be some small delay
  // this delay also affetcs on speed of divider blinking
  // because calculates by index (the faster index is increasing - the faster blinking is)
  delay(50);
}

/*
  process current digits group
  1) if we have animation - we need to show animated digit instead of actual
  2) otherwise - simply show current one

  [feature] also, it should be possibility to handle parallel lines
*/
void processGroup(CRGB * ledsArray, int animationIndex, int currentDigitIndex) {
  /*
    hard way: we have two parallel led strips
    ->[]->[]->[]->[]->[]-┐
    []<-[]<-[]<-[]<-[]<--┘
  */
  if (USE_PARALLEL){ 
    // if animation running
    if (isAnimationRunning(animationIndex)) {
      ledsArray[parallelIndexes[animations[animationIndex]]] = GLOBAL_COLOR;
      updateAnimation(animationIndex);
  
      // otherwise
    } else {
      ledsArray[parallelIndexes[currentDigitIndex]] = GLOBAL_COLOR;
    }
  /*
    simple way: we have straight led strip
    ->[]->[]->[]->[]->[]->[]->[]->[]->[]->[]
    ->[]->[]->[]->[]->[]->[]->[]->[]->[]->[]
  */
  } else {
    // if animation running
    if (isAnimationRunning(animationIndex)) {
      ledsArray[9 - (animations[animationIndex])] = GLOBAL_COLOR;
      updateAnimation(animationIndex);
  
      // otherwise
    } else {
      ledsArray[9 - currentDigitIndex] = GLOBAL_COLOR;
    }
  }
}

/*
  check if we can start animation
  and if so - start it
  index - means:
  0,1 - hours
  2,3 - minutes
*/
void checkIfCanAnimate(CRGB * ledsArray, int animationIndex, int currentDigitIndex){
  if (lastTime[animationIndex] != currentDigitIndex && !isAnimationRunning(animationIndex)){
    startAnimation(animationIndex, currentDigitIndex);
  }
}

/*
  start animaition
  initialPos - current digit
*/
void startAnimation(int index, int initialPos) {
  animations[index] = initialPos;
}

/*
  stop existing animation
  it's enough just set it as negative number
  because indexes of digits will start from zero
*/
void stopAnimation(int index) {
  animations[index] = -1;
}

/*
  update existing animation
  for example for time change 15:15 - 15:16 (animate last digit)
  the point of animation is:
  1) take current digit (5) and start increasing it's own index (>5,6,7,8,9...)
  2) if there is no further digits - loop over zero (...0,1,2...)
  3) if we reached our next expected digit (6) - stop animation (...4,5,>6)
*/
void updateAnimation(int index) {
  animations[index]++;

  // loop over zero
  if (animations[index] > 9) {
    animations[index] = 0;
  }

  // we reached our destination digit
  if (animations[index] == lastTime[index]) {
    stopAnimation(index);
  }
}

/*
  check if animation for specific digit is running now
*/
bool isAnimationRunning(int index) {
  return animations[index] >= 0;
}

/*
  take night mode flag
  and store it into memory for further reboot
*/
void HandleNightMode() {
  String Response = (String)millis();
  String state = server.arg("enabled");
  NIGHT_MODE = state == "true";

  // store for further reboot
  EEPROM.write(NIGHT_MODE_EEPROM_INDEX, NIGHT_MODE ? 1 : 0);
  EEPROM.commit();
  Serial.println(NIGHT_MODE);
  server.send(200, "text/html", Response);
}

/*
  take color from phone
*/
void HandleGlobalColor() {
  String Response = (String)millis();
  byte r = server.arg("r").toInt(),
       g = server.arg("g").toInt(),
       b = server.arg("b").toInt();
  GLOBAL_COLOR = CRGB(r, g, b);

  // and also save in memory for further reboot
  EEPROM.write(R_EEPROM_INDEX, r);
  EEPROM.write(G_EEPROM_INDEX, g);
  EEPROM.write(B_EEPROM_INDEX, b);
  EEPROM.commit();
  server.send(200, "text/html", Response);
}

/*
  take time & date from phone and save it
*/
void HandleTimeSync() {
  String Response = (String)millis();
  int  Y = server.arg("Y").toInt(),
       M = server.arg("M").toInt(),
       D = server.arg("D").toInt(),
       h = server.arg("h").toInt(),
       m = server.arg("m").toInt(),
       s = server.arg("s").toInt();

  // take phone's date
  synchronizeWithPhone(Y, M, D, h, m, s);
  server.send(200, "text/html", Response);
}


/*
  toggle between TIME and DATE
*/
void HandleDisplayMode() {
  String Response = (String)millis();
  String state = server.arg("mode");
  SHOW_DATE = state == "date";

  // and also save in memory for further reboot
  EEPROM.write(SHOW_DATE_EEPROM_INDEX, SHOW_DATE ? 1 : 0);
  EEPROM.commit();
  server.send(200, "text/html", Response);
}

/*
  turn off all leds
*/
void reset() {
  for (int i = 0; i < LEDS_COUNT; i++) {
    Minutes1[i] = CRGB::Black;
    Minutes2[i] = CRGB::Black;
    Divider[i] = CRGB::Black;
    Hours1[i] = CRGB::Black;
    Hours2[i] = CRGB::Black;
  }
}

/*
  fade all leds (make darker)
*/
void fade() {
  for (int i = 0; i < LEDS_COUNT; i++) {
    Minutes1[i].fadeLightBy(FADING_LEVEL);
    Minutes2[i].fadeLightBy(FADING_LEVEL);
    Divider[i].fadeLightBy(FADING_LEVEL);
    Hours1[i].fadeLightBy(FADING_LEVEL);
    Hours2[i].fadeLightBy(FADING_LEVEL);
  }
}

/*
  get specific digit from number
  by substracting ones, tens or hundreds
*/
int getDigit(int number, int symbol) {
  int first = ((number) / 10) % 10;     // 10
  int second = (number) % 10;           // 1
  return (symbol == 0) ? first : second;
}

/*
  request for all settings
  phone takes it all
*/
void GetSettings() {
  IPAddress myIP = WiFi.localIP();
  String ip = String(myIP[0]) + "." + String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]);
  String Response = "{"
                    "'color': {'r':" + (String)GLOBAL_COLOR.r + ", 'g':" + (String)GLOBAL_COLOR.g + ", 'b':" + (String)GLOBAL_COLOR.b + "},"
                    "'ip': '" + ip + "',"
                    "'nightMode': " + (NIGHT_MODE ? "true" : "false") + ","
                    "'fadingLevel': " + (String)FADING_LEVEL + ","
                    "'displayMode': '" + ((SHOW_DATE) ? "date" : "clock") + "'"
                    "}";
  server.send(200, "text/html", Response);
}

/*
  sync with internet time
  use it only if you have strict timezone
  (without switching between summer and winter)
*/
void synchronizeWithInternet() {
  timeClient.begin();
  delay(200);
  timeClient.update();
  char hours[2], minutes[2], seconds[2];
  sprintf(hours, "%d", timeClient.getHours());
  sprintf(minutes, "%d", timeClient.getMinutes());
  sprintf(seconds, "%d", timeClient.getSeconds());
  char timestring[9];
  timeClient.getFormattedTime().toCharArray(timestring, 9);
  Serial.println(timestring);
  Rtc.SetDateTime(RtcDateTime(__DATE__, timestring));
  timeClient.end();
}

/*
  sync with device
  takes date and saves into clock module itself
*/
void synchronizeWithPhone(int Y, int M, int D, int h, int m, int s) {
  Rtc.SetDateTime(RtcDateTime(Y, M, D, h, m, s));
}

/*
  UTIL
  saves string into EEPROM
*/
void saveStringIntoMemory(char add, String data) {
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++) {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0');
  EEPROM.commit();
}

/*
  UTIL
  takes string from EEPROM
*/
String readStringFromMemory(char add) {
  int i;
  char data[100];                   // Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  if (k == 255) {
    data[0] = '\0';
    return String(data);
  }
  while (k != '\0' && len < 500) {  // Read until null character
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}
