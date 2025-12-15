#include <Wire.h>
#include "rgb_lcd.h"
#include <DHT.h>
#include <math.h>

// ================== PIN DEFINITIONS ==================
#define DHTPIN     4
#define DHTTYPE    DHT22

#define TRIG_PIN   26
#define ECHO_PIN   27
#define LED_PIN    13

// ================== OBJECTS ==================
rgb_lcd lcd;
DHT dht(DHTPIN, DHTTYPE);

// ================== HISTORY ==================
const int HISTORY_SIZE = 10;          // 10 samples × 15s
float distanceHistory[HISTORY_SIZE];
float tempHistory[HISTORY_SIZE];

int historyIndex = 0;
bool historyFilled = false;

unsigned long lastSampleTime = 0;
const unsigned long DATA_INTERVAL = 15000;

bool sittingWarningPrinted = false;
bool tempHighReported = false;
bool tempLowReported  = false;

// ================== FUNCTIONS ==================

float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return NAN;

  float cm = duration * 0.0343 / 2.0;
  if (cm < 2 || cm > 400) return NAN;
  return cm;
}

bool isSittingTooLong() {
  if (!historyFilled) return false;

  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (!isnan(distanceHistory[i]) && distanceHistory[i] > 90.0) {
      return false;
    }
  }
  return true;
}

void printJsonFloat(const char* key, float value, int decimals) {
  Serial.print("\"");
  Serial.print(key);
  Serial.print("\":");
  if (isnan(value)) Serial.print("null");
  else Serial.print(value, decimals);
}

// ================== SETUP ==================

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  lcd.begin(16, 2);
  lcd.setRGB(0, 0, 255);
  lcd.print("DeskBuddy Start");
  delay(1000);
  lcd.clear();
}

// ================== LOOP ==================

void loop() {
  float distance = getDistanceCm();

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  bool dhtOk = !(isnan(temp) || isnan(hum));

  bool sampled = false;

  // -------- History sampling --------
  if (lastSampleTime == 0 || millis() - lastSampleTime >= DATA_INTERVAL) {
    lastSampleTime = millis();
    sampled = true;

    distanceHistory[historyIndex] = distance;
    tempHistory[historyIndex] = temp;

    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    if (historyIndex == 0) historyFilled = true;

    if (historyFilled) {
      bool allHigh = true;
      bool allLow  = true;

      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (isnan(tempHistory[i])) {
          allHigh = false;
          allLow = false;
          break;
        }
        if (tempHistory[i] <= 23.0) allHigh = false;
        if (tempHistory[i] >= 19.0) allLow  = false;
      }

      if (allHigh && !tempHighReported) {
        Serial.println("{\"event\":\"temp_high_5min\"}");
        tempHighReported = true;
        tempLowReported = false;
      }

      if (allLow && !tempLowReported) {
        Serial.println("{\"event\":\"temp_low_5min\"}");
        tempLowReported = true;
        tempHighReported = false;
      }
    }
  }

  bool sittingTooLong = isSittingTooLong();

  // -------- LED logic --------
  digitalWrite(LED_PIN, (dhtOk && temp > 23.0) ? HIGH : LOW);

  // -------- LCD --------
  lcd.setCursor(0, 0);
  if (dhtOk) {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(hum, 0);
    lcd.print("% ");
  } else {
    lcd.print("DHT ERROR       ");
  }

  lcd.setCursor(0, 1);
  if (sittingTooLong) {
    lcd.print("Stand up advised");
  } else if (isnan(distance)) {
    lcd.print("Dist: OutRange  ");
  } else {
    lcd.print("Dist:");
    lcd.print(distance, 1);
    lcd.print("cm         ");
  }


  Serial.print("{");
  Serial.print("\"ts_ms\":"); Serial.print(millis()); Serial.print(",");
  printJsonFloat("distance_cm", distance, 1); Serial.print(",");
  printJsonFloat("temp_c", temp, 1); Serial.print(",");
  printJsonFloat("hum_pct", hum, 0);
  Serial.println("}");


  delay(200);
}
