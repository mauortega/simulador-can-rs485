#pragma once

#include <Arduino.h>

struct CanFrame {
  uint32_t id = 0;
  bool extended = false;
  uint8_t len = 8;
  uint8_t data[8] = {0};
};

class CanFrameSink {
public:
  virtual ~CanFrameSink() = default;
  virtual bool send(uint8_t busIndex, const CanFrame &frame) = 0;
};
