#pragma once

#include <Arduino.h>

class Rs485Application {
public:
  void begin();
  bool loop(uint32_t nowMs);

public:
  static constexpr uint8_t RxPin = 5;
  static constexpr uint8_t TxPin = 4;
  static constexpr uint8_t DirectionPin = 6;
  static constexpr uint32_t Baud = 19200UL;
  static constexpr uint32_t TxPeriodMs = 2000UL;
  static constexpr uint16_t DirectionReleaseUs = 100;

private:
  uint32_t mLastTxMs = 0;
  uint32_t mTxFinishedUs = 0;
  bool mTransmitting = false;
};
