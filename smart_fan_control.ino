#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
hd44780_I2Cexp lcd;

const uint8_t FAN_PIN   = 9;
const uint8_t ALARM_PIN = 8;
const uint8_t BUTTON_PIN = 3;

const uint8_t LED1 = 4;
const uint8_t LED2 = 5;
const uint8_t LED3 = 6;

const float T_LOW      = 30.0;
const float T_MED      = 40.0;
const float T_CRITICAL = 45.0;

const uint8_t PWM_LOW  = 50;
const uint8_t PWM_MED  = 150;
const uint8_t PWM_HIGH = 250;

bool manualOverride = false;


void updateButton() {
  static bool prevState = HIGH;
  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState != prevState) {  // state changed
    if (currentState == LOW) {      // button pressed
      manualOverride = !manualOverride;
      Serial.println(manualOverride ? "Manual ON" : "Manual OFF");
    }
    delay(50);   // debounce
  }

  prevState = currentState;
}
// ------------------------------------------------------------

void updateLEDs(bool manual, float t) {
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  if (manual) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    return;
  }

  if (t >= T_CRITICAL) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    return;
  }

  if (t >= T_MED) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
  } else if (t >= T_LOW) {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
  } else {
    digitalWrite(LED1, HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(FAN_PIN, OUTPUT);
  pinMode(ALARM_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  lcd.clear();
  lcd.print("Initializing...");
  delay(800);
  lcd.clear();
}

void loop() {
  updateButton();  // check button first

  float t = dht.readTemperature();

  if (isnan(t)) {
    analogWrite(FAN_PIN, PWM_LOW);
    digitalWrite(ALARM_PIN, LOW);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sensor Error!");
    lcd.setCursor(0,1);
    lcd.print("Check DHT11");
    delay(800);
    return;
  }

  uint8_t pwmValue;
  const char *modeText;

  if (manualOverride) {
    pwmValue = PWM_HIGH;
    modeText = "MANUAL-HIGH";
    digitalWrite(ALARM_PIN, LOW);
  } else {
    if (t < T_LOW) {
      pwmValue = PWM_LOW; modeText = "LOW";
    } else if (t < T_MED) {
      pwmValue = PWM_MED; modeText = "MED";
    } else {
      pwmValue = PWM_HIGH; modeText = "HIGH";
    }

    bool critical = (t >= T_CRITICAL);
    digitalWrite(ALARM_PIN, critical ? HIGH : LOW);
  }

  analogWrite(FAN_PIN, pwmValue);
  updateLEDs(manualOverride, t);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(t,1);
  lcd.print(" C");

  lcd.setCursor(0,1);
  if (manualOverride) {
    lcd.print("MANUAL OVERRIDE");
  } else if (t >= T_CRITICAL) {
    lcd.print("CRITICAL TEMP!");
  } else {
    lcd.print("Mode: ");
    lcd.print(modeText);
  }

  delay(200);
}
