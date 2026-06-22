#pragma once

#include <Arduino.h>

class Rs485Application {
public:
  void begin();
  void loop(uint32_t nowMs);

private:
  static constexpr int TxPin = 33;
  static constexpr uint32_t Baud = 19200UL;
  static constexpr uint32_t TxPeriodMs = 2000UL;

  uint32_t mLastTxMs = 0;
};
