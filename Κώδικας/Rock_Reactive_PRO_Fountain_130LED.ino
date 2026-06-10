#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#define LED_PIN 6
#define NUM_LEDS 129

#define MIC_PIN A0
#define PUMP1 2
#define PUMP2 3
#define PUMP3 4

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial mySerial(10, 11);
DFRobotDFPlayerMini player;

int rawSignal = 0;
int soundLevel = 0;
int smoothedLevel = 0;
int dynamicMax = 150;

int vuLevel = 0;
int displayLevel = 0;
int peak = 0;

unsigned long lastUpdate = 0;
const int updateInterval = 20;

unsigned long lastPattern = 0;
int patternStep = 0;

// Relay protection (2 sec)
const unsigned long MIN_PUMP_INTERVAL = 2000;

bool pump1State = false;
bool pump2State = false;
bool pump3State = false;

unsigned long pump1Timer = 0;
unsigned long pump2Timer = 0;
unsigned long pump3Timer = 0;

void setup() {

  Serial.begin(9600);

  strip.begin();
  //periorismos fotinotitas led tainias
  strip.setBrightness(80);
  strip.clear();
  strip.show();

  pinMode(PUMP1, OUTPUT);
  pinMode(PUMP2, OUTPUT);
  pinMode(PUMP3, OUTPUT);

  pump1OFF();
  pump2OFF();
  pump3OFF();

  mySerial.begin(9600);

  if (player.begin(mySerial)) {
    player.volume(22);
    player.enableLoopAll();
    Serial.println("DFPlayer OK");
  } else {
    Serial.println("DFPlayer ERROR");
  }
}

void loop() {

  if (millis() - lastUpdate >= updateInterval) {

    lastUpdate = millis();

    readAudio();
    updateVU();
    choreography();
  }
}

void readAudio() {

  rawSignal = analogRead(MIC_PIN);

  int centered = rawSignal - 512;

  soundLevel = abs(centered);

  smoothedLevel =
    (smoothedLevel * 6 + soundLevel * 4) / 10;

  if (smoothedLevel < 12)
    smoothedLevel = 0;

  if (smoothedLevel > dynamicMax)
    dynamicMax = smoothedLevel;

  dynamicMax--;

  if (dynamicMax < 80)
    dynamicMax = 80;
}

void updateVU() {

  vuLevel = map(smoothedLevel, 0, dynamicMax, 0, NUM_LEDS);

  vuLevel = constrain(vuLevel, 0, NUM_LEDS);

  if (vuLevel > displayLevel)
    displayLevel = vuLevel;
  else {
    displayLevel--;
    if (displayLevel < 0)
      displayLevel = 0;
  }

  if (displayLevel > peak)
    peak = displayLevel;
  else {
    peak--;
    if (peak < 0)
      peak = 0;
  }
}

void choreography() {

  strip.clear();

  // High energy
  if (displayLevel >= 100) {

    pump1ON();
    pump2ON();
    pump3ON();

    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
    }

    strip.show();
    return;
  }

  // Medium energy
  if (displayLevel >= 40) {

    if (millis() - lastPattern > 1400) {

      lastPattern = millis();

      patternStep++;

      if (patternStep > 2)
        patternStep = 0;
    }

    if (patternStep == 0) {
      pump1ON(); pump2OFF(); pump3OFF();
    }
    else if (patternStep == 1) {
      pump1OFF(); pump2ON(); pump3OFF();
    }
    else {
      pump1OFF(); pump2OFF(); pump3ON();
    }

    for (int i = 0; i < displayLevel; i++) {

      if (i < NUM_LEDS * 0.5)
        strip.setPixelColor(i, strip.Color(0, 255, 0));
      else if (i < NUM_LEDS * 0.8)
        strip.setPixelColor(i, strip.Color(255, 140, 0));
      else
        strip.setPixelColor(i, strip.Color(255, 0, 0));
    }

    if (peak > 0 && peak < NUM_LEDS)
      strip.setPixelColor(peak, strip.Color(255, 255, 255));

    strip.show();
    return;
  }

  // Low energy
  if (millis() - lastPattern > 900) {

    lastPattern = millis();

    patternStep++;

    if (patternStep >= NUM_LEDS)
      patternStep = 0;
  }

  strip.setPixelColor(patternStep, strip.Color(0, 0, 255));
  strip.setPixelColor((patternStep + 1) % NUM_LEDS,
                      strip.Color(0, 100, 255));

  pump1ON();
  pump2OFF();
  pump3OFF();

  strip.show();
}

// ACTIVE LOW RELAYS WITH 2 SEC PROTECTION

void pump1ON() {
  if (!pump1State && millis() - pump1Timer >= MIN_PUMP_INTERVAL) {
    digitalWrite(PUMP1, LOW);
    pump1State = true;
    pump1Timer = millis();
  }
}

void pump1OFF() {
  if (pump1State && millis() - pump1Timer >= MIN_PUMP_INTERVAL) {
    digitalWrite(PUMP1, HIGH);
    pump1State = false;
    pump1Timer = millis();
  }
}

void pump2ON() {
  if (!pump2State && millis() - pump2Timer >= MIN_PUMP_INTERVAL) {
    digitalWrite(PUMP2, LOW);
    pump2State = true;
    pump2Timer = millis();
  }
}

void pump2OFF() {
  if (pump2State && millis() - pump2Timer >= MIN_PUMP_INTERVAL) {
    digitalWrite(PUMP2, HIGH);
    pump2State = false;
    pump2Timer = millis();
  }
}

void pump3ON() {
  if (!pump3State && millis() - pump3Timer >= MIN_PUMP_INTERVAL) {
    digitalWrite(PUMP3, LOW);
    pump3State = true;
    pump3Timer = millis();
  }
}

void pump3OFF() {
  if (pump3State && millis() - pump3Timer >= MIN_PUMP_INTERVAL) {
    digitalWrite(PUMP3, HIGH);
    pump3State = false;
    pump3Timer = millis();
  }
}
