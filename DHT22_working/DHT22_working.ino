#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT22.h>

// Pin Definitions
#define DHT22_PIN 4  // You can change to any available GPIO if needed

// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // Common I2C address for Grove 16x2 LCD

// DHT22 Setup
DHT22 dht22(DHT22_PIN);

void setup() {
  Serial.begin(115200);

  // LCD Initialization
  lcd.init();
  lcd.backlight();

  // Welcome Message
  lcd.setCursor(0, 0);
  lcd.print("DHT22 Sensor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

void loop() {
  float temperature = dht22.getTemperature();
  float humidity = dht22.getHumidity();

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  if (dht22.getLastError() != dht22.OK) {
    lcd.print("Err ");
  } else {
    lcd.print(temperature, 1);
    lcd.print((char)223); // Degree symbol
    lcd.print("C ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Hum : ");
  if (dht22.getLastError() != dht22.OK) {
    lcd.print("Err ");
  } else {
    lcd.print(humidity, 1);
    lcd.print("%   ");
  }

  delay(2000); // Update every 2 seconds
}
