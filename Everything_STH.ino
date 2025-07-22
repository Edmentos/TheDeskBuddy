#include <Wire.h>
#include "rgb_lcd.h"
#include <DHT22.h>

// --- Pin Definitions ---
#define DHTPIN 4
#define TRIG_PIN 26
#define ECHO_PIN 27
#define BUTTON_PIN 15
#define LED_PIN 13

// --- Objects ---
rgb_lcd lcd;
DHT22 dht(DHTPIN);

// --- Display Mode State ---
int displayMode = 0; // 0 = Distance, 1 = Temp/Humidity

// --- Sitting History ---
const int HISTORY_SIZE =10 ; // 5 minutes at 15s intervals
float distanceHistory[HISTORY_SIZE];
int distanceIndex = 0;
bool distanceHistoryFilled = false;
unsigned long lastDistanceAddTime = 0;
bool sittingWarningPrinted = false;  // <---- New flag

// --- Temperature Monitoring ---
float tempHistory[HISTORY_SIZE];
int tempIndex = 0;
bool tempHistoryFilled = false;
bool tempHighReported = false;
bool tempLowReported = false;

// --- Timing ---
const unsigned long DATA_INTERVAL = 15000; // 15 seconds

void setup() {
  Serial.begin(115200);

  lcd.begin(16, 2);
  lcd.setRGB(0, 0, 255);
  lcd.print("Starting...");
  delay(1000);
  lcd.clear();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Switch display mode on button press
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    displayMode = (displayMode + 1) % 2;
    lcd.clear();
    delay(300);
  }
  lastButtonState = currentButtonState;

  float distance = getDistance();
  float t = dht.getTemperature();
  float h = dht.getHumidity();

  // Add data every 15s
  if (millis() - lastDistanceAddTime >= DATA_INTERVAL || lastDistanceAddTime == 0) {
    lastDistanceAddTime = millis();

    // Add to distance history
    distanceHistory[distanceIndex] = distance;
    Serial.print("Distance sample [");
    Serial.print(distanceIndex);
    Serial.print("]: ");
    Serial.println(distance);
    distanceIndex = (distanceIndex + 1) % HISTORY_SIZE;
    if (distanceIndex == 0) distanceHistoryFilled = true;

    // Add to temperature history
    tempHistory[tempIndex] = t;
    tempIndex = (tempIndex + 1) % HISTORY_SIZE;
    if (tempIndex == 0) tempHistoryFilled = true;

    // Temperature alerts
    if (tempHistoryFilled) {
      bool allHigh = true, allLow = true;
      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (tempHistory[i] <= 23.0) allHigh = false;
        if (tempHistory[i] >= 19.0) allLow = false;
      }

      if (allHigh && !tempHighReported) {
        Serial.println("⚠️ Temp > 23°C for 5 minutes. Notified team to lower temperature.");
        tempHighReported = true;
        tempLowReported = false;
      }
      if (allLow && !tempLowReported) {
        Serial.println("⚠️ Temp < 19°C for 5 minutes. Notified team to raise temperature.");
        tempLowReported = true;
        tempHighReported = false;
      }
    }
  }

  // Display logic
  if (displayMode == 0) {
    showDistance(distance);
  } else {
    showTempHumidity(t, h);
  }

  // Sitting detection and messages
  if (distanceHistoryFilled && isSittingTooLong()) {
    lcd.setCursor(0, 1);
    lcd.print("Stand up advised");
    if (!sittingWarningPrinted) {
      Serial.println("🔔 Advice: You've been sitting for a while. Please stand up.");
      sittingWarningPrinted = true; // Only print once
    }
  } else {
    sittingWarningPrinted = false; // Reset flag once user stands up
  }

  delay(200);
}

// Distance measurement function
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float cm = duration * 0.0343 / 2;
  return cm;
}

// Display distance on LCD
void showDistance(float dist) {
  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.setCursor(0, 1);
  if (dist <= 0 || dist > 300) {
    lcd.print("Out of Range    ");
  } else {
    lcd.print(dist, 1);
    lcd.print(" cm         ");
  }
  digitalWrite(LED_PIN, LOW); // LED off in distance mode
}

// Display temperature and humidity on LCD
void showTempHumidity(float temp, float hum) {
  lcd.setCursor(0, 0);
  if (dht.getLastError() != dht.OK) {
    lcd.print("Sensor error    ");
    digitalWrite(LED_PIN, LOW);
  } else {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(hum, 0);
    lcd.print("%");

    if (temp > 23.0) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

// Check if sitting too long (all distance readings below 90 cm)
bool isSittingTooLong() {
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (distanceHistory[i] > 90.0) {
      return false;
    }
  }
  return true;
}
