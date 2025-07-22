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

// --- Sitting Detection ---
const float SIT_DISTANCE_CM = 90.0;
const float STAND_UP_THRESHOLD_CM = 105.0;
const int HISTORY_SIZE = 8;
float distanceHistory[HISTORY_SIZE];
int historyIndex = 0;
bool historyFilled = false;
bool adviceShown = false;

unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 15000; // 15 seconds

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

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    displayMode = (displayMode + 1) % 2;
    lcd.clear();
    delay(300);
  }
  lastButtonState = currentButtonState;

  float distance = getDistance();
  unsigned long now = millis();

  // --- Add to history every 15 seconds ---
  if (now - lastSampleTime >= SAMPLE_INTERVAL || lastSampleTime == 0) {
    lastSampleTime = now;
    distanceHistory[historyIndex] = distance;
    Serial.print("Sample ");
    Serial.print(historyIndex);
    Serial.print(": ");
    Serial.println(distance, 1);

    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    if (historyIndex == 0) historyFilled = true;
  }

  if (displayMode == 0) {
    showDistance(distance);
  } else {
    showTempHumidity();
  }

  // --- Sitting check ---
  if (historyFilled && isSittingTooLong()) {
    if (!adviceShown) {
      Serial.println("We advise you to stand up!");
      adviceShown = true;
    }
    clearLine(1);
    lcd.setCursor(0, 1);
    lcd.print("Stand up advised");
  } else if (adviceShown && !isSittingTooLong()) {
    Serial.println("Great job!");
    clearLine(1);
    lcd.setCursor(0, 1);
    lcd.print("Great job!");
    adviceShown = false;
  }

  delay(200);
}

// --- Helper to Clear LCD Line ---
void clearLine(int line) {
  lcd.setCursor(0, line);
  lcd.print("                ");
}

// --- Distance Measurement ---
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distanceCm = duration * 0.0343 / 2;
  return distanceCm;
}

// --- Show Distance ---
void showDistance(float distanceCm) {
  lcd.setCursor(0, 0);
  lcd.print("Distance:       ");
  clearLine(1);
  lcd.setCursor(0, 1);

  if (distanceCm <= 0 || distanceCm > 400) {
    lcd.print("Out of range");
  } else {
    lcd.print(distanceCm, 1);
    lcd.print(" cm");
  }

  digitalWrite(LED_PIN, LOW);
}

// --- Show Temperature & Humidity ---
void showTempHumidity() {
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();

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

  clearLine(1);
}

// --- Sitting Check ---
bool isSittingTooLong() {
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (distanceHistory[i] > SIT_DISTANCE_CM) return false;
  }
  return true;
}
