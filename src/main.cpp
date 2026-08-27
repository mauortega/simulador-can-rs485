#include <Arduino.h>

#include "CanApplication.h"
#include "Rs485Application.h"

#include "AppConfig.h"

static CanApplication canApp;
static Rs485Application rs485App;
static uint32_t rs485LedOnMs = 0;
static bool rs485LedOn = false;

static void setRs485ActivityLed(bool on) {
  rs485LedOn = on;
  const bool level = AppConfig::Rs485ActivityLedActiveLow ? !on : on;
  digitalWrite(AppConfig::Rs485ActivityLedPin, level ? HIGH : LOW);
}

static void updateRs485ActivityLed(uint32_t nowMs, bool packetSent) {
  if (packetSent) {
    rs485LedOnMs = nowMs;
    setRs485ActivityLed(true);
  }

  if (rs485LedOn &&
      (nowMs - rs485LedOnMs) >= AppConfig::Rs485ActivityLedPulseMs) {
    setRs485ActivityLed(false);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(AppConfig::Rs485ActivityLedPin, OUTPUT);
  setRs485ActivityLed(false);

  Serial.println();
  Serial.println("=== CAN_teste_03: CAN + RS485 ===");

  canApp.begin();
  rs485App.begin();
}

void loop() {
  const uint32_t nowMs = millis();

  canApp.loop(nowMs);
  const bool rs485PacketSent = rs485App.loop(nowMs);
  updateRs485ActivityLed(nowMs, rs485PacketSent);

  delay(AppConfig::LoopDelayMs);
}
