#include <Arduino.h>
#include <BleKeyboard.h>

#define PIN_CLK 2
#define PIN_DT  3
#define PIN_SW  4
#define PIN_LED 8 // LED integrato ESP32-C3 SuperMini (Active LOW)

#define TIMEOUT_INACTIVITY_MS 120000 // 2 Minuti

BleKeyboard bleKeyboard("Volumefy", "Custom", 100);

int lastCLKState;
unsigned long lastActivityTime = 0;
unsigned long lastLedBlinkTime = 0;

// Gestione non bloccante del pulsante
unsigned long lastButtonPress = 0;
bool lastButtonState = HIGH;

void goToSleep() {
  digitalWrite(PIN_LED, HIGH); // Spegne il LED (logic LOW = acceso)
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_SW, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

void checkEncoder() {
  int currentCLK = digitalRead(PIN_CLK);
  
  // Lettura rotazione manopola
  if (currentCLK != lastCLKState && currentCLK == LOW) {
    if (bleKeyboard.isConnected()) {
      if (digitalRead(PIN_DT) != currentCLK) {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
      } else {
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
      }
    }
    lastActivityTime = millis();
  }
  lastCLKState = currentCLK;

  // Lettura pulsante Mute senza cicli di attesa (antirimbalzo software)
  bool currentButtonState = digitalRead(PIN_SW);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (millis() - lastButtonPress > 200) { 
      if (bleKeyboard.isConnected()) {
        bleKeyboard.write(KEY_MEDIA_MUTE);
      }
      lastActivityTime = millis();
      lastButtonPress = millis();
    }
  }
  lastButtonState = currentButtonState;
}

void handleLedAndState() {
  unsigned long currentMillis = millis();
  bool connected = bleKeyboard.isConnected();

  if (connected) {
    // Connesso: impulso breve ogni 15 secondi
    if (currentMillis - lastLedBlinkTime >= 15000) {
      digitalWrite(PIN_LED, LOW); // Accende
      delay(30);
      digitalWrite(PIN_LED, HIGH); // Spegne
      lastLedBlinkTime = currentMillis;
    }
  } else {
    // Discoverable: lampeggio ogni secondo
    if (currentMillis - lastLedBlinkTime >= 1000) {
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
      lastLedBlinkTime = currentMillis;
    }
  }
}

void setup() {
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH); // Spento all'avvio

  lastCLKState = digitalRead(PIN_CLK);
  lastActivityTime = millis();

  bleKeyboard.begin();
}

void loop() {
  checkEncoder();
  handleLedAndState();

  if (millis() - lastActivityTime >= TIMEOUT_INACTIVITY_MS) {
    goToSleep();
  }

  delay(1); // Cede il controllo allo stack Bluetooth ed evita i reset del watchdog
}