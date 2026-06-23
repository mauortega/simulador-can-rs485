#include <Arduino.h>

#include "CanApplication.h"
#include "Rs485Application.h"

#include "AppConfig.h"

static CanApplication canApp;
static Rs485Application rs485App;
static uint32_t lastStatusLedToggleMs = 0;
static bool statusLedOn = false;

static void setStatusLed(bool on) {
  statusLedOn = on;
  const bool level = AppConfig::StatusLedActiveLow ? !on : on;
  digitalWrite(AppConfig::StatusLedPin, level ? HIGH : LOW);
}

static void updateStatusLed(uint32_t nowMs) {
  if ((nowMs - lastStatusLedToggleMs) < AppConfig::StatusLedPeriodMs) {
    return;
  }

  lastStatusLedToggleMs = nowMs;
  setStatusLed(!statusLedOn);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(AppConfig::StatusLedPin, OUTPUT);
  setStatusLed(false);

  Serial.println();
  Serial.println("=== CAN_teste_03: CAN + RS485 ===");

  canApp.begin();
  rs485App.begin();
}

void loop() {
  const uint32_t nowMs = millis();

  canApp.loop(nowMs);
  rs485App.loop(nowMs);
  updateStatusLed(nowMs);

  delay(AppConfig::LoopDelayMs);
}
