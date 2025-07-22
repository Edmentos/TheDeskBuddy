#include <Wire.h>
#include "rgb_lcd.h"
#include <DHT22.h>

// --- Pin Definitions ---
#define DHTPIN 4
#define TRIG_PIN 26
#define ECHO_PIN 27
#define BUTTON_PIN 15

// --- Objects ---
rgb_lcd lcd;
DHT22 dht(DHTPIN);

// --- Display Mode State ---
int displayMode = 0; // 0 = Distance, 1 = Temp/Humidity

// --- Setup ---
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
}

// --- Main Loop ---
void loop() {
  static bool lastState = HIGH;

  bool currentState = digitalRead(BUTTON_PIN);
  if (lastState == HIGH && currentState == LOW) {
    // Button press detected
    displayMode = (displayMode + 1) % 2;
    lcd.clear(); // clear screen on mode change
    delay(300);  // simple debounce delay
  }
  lastState = currentState;

  if (displayMode == 0) {
    showDistance();
  } else {
    showTempHumidity();
}

  delay(200);
}

// --- Show Distance ---
void showDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distanceCm = duration * 0.0343 / 2;

  lcd.setCursor(0, 0);
  lcd.print("Distance:");
  lcd.setCursor(0, 1);
  if (duration == 0) {
    lcd.print("Out of Range    ");
  } else {
    lcd.print(distanceCm, 1);
    lcd.print(" cm         ");
  }
}

// --- Show Temp & Humidity ---
void showTempHumidity() {
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();

  lcd.setCursor(0, 0);
  if (dht.getLastError() != dht.OK) {
    lcd.print("Sensor error    ");
  } else {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(hum, 0);
    lcd.print("%");
  }

  lcd.setCursor(0, 1);
  lcd.print("                "); // clear 2nd line
}
