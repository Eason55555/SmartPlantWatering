#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensorPin = A0;
const int relayPin = 7;

const int DRY_THRESHOLD = 40;   // 開始加水的門檻 (%)
const int WET_THRESHOLD = 60;   // 停止加水的門檻 (%)

const unsigned long MEASURE_INTERVAL = 15UL * 60UL * 1000UL;      // 每15分鐘測一次
const unsigned long PAUSE_AFTER_WATERING = 60UL * 60UL * 1000UL;  // 加水完暫停1小時

bool isPumping = false;
unsigned long lastMeasureTime = 0;
unsigned long pauseUntil = 0;

void setup() {
  Serial.begin(9600);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // 預設為高電平（低電平觸發）

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // 加水期間每秒監控濕度
  if (isPumping) {
    int raw = analogRead(sensorPin);
    int percent = map(raw, 1023, 0, 0, 100); // 若濕度低→數值高
    percent = constrain(percent, 0, 100);

    Serial.print("[Pumping] Raw: ");
    Serial.print(raw);
    Serial.print(" → ");
    Serial.print(percent);
    Serial.println("%");

    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");
    lcd.print(percent);
    lcd.print("%   "); // 清除殘影

    lcd.setCursor(0, 1);
    lcd.print("Watering...     ");

    if (percent >= WET_THRESHOLD) {
      digitalWrite(relayPin, HIGH);  // 停止加水（高電平）
      isPumping = false;
      pauseUntil = now + PAUSE_AFTER_WATERING;

      Serial.println("Stopped Pump - Moisture OK");

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Moisture: ");
      lcd.print(percent);
      lcd.print("%");

      lcd.setCursor(0, 1);
      lcd.print("Done. Pause 1hr");
    }

    delay(1000); // 每秒檢查一次
    return;
  }

  // 暫停期間不做事
  if (now < pauseUntil) {
    return;
  }

  // 每15分鐘檢查一次濕度
  if (now - lastMeasureTime >= MEASURE_INTERVAL || lastMeasureTime == 0) {
    lastMeasureTime = now;

    int raw = analogRead(sensorPin);
    int percent = map(raw, 1023, 0, 0, 100);
    percent = constrain(percent, 0, 100);

    Serial.print("[Check] Raw: ");
    Serial.print(raw);
    Serial.print(" → ");
    Serial.print(percent);
    Serial.println("%");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");
    lcd.print(percent);
    lcd.print("%   ");

    lcd.setCursor(0, 1);

    if (percent < DRY_THRESHOLD) {
      digitalWrite(relayPin, LOW); // 啟動繼電器（低電平觸發）
      isPumping = true;
      lcd.print("Start watering!");
      Serial.println("Pump ON - Moisture low");
    } else {
      lcd.print("I'm fine :)     ");
    }
  }
}
