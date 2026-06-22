#include "SignalEncoding.h"

float clampFloat(float value, float lo, float hi) {
  if (value < lo) {
    return lo;
  }
  if (value > hi) {
    return hi;
  }
  return value;
}

uint32_t clampU32(int32_t value, uint32_t lo, uint32_t hi) {
  if (value < (int32_t)lo) {
    return lo;
  }
  if (value > (int32_t)hi) {
    return hi;
  }
  return (uint32_t)value;
}

void clearFrame(uint8_t *data) { fillFrame(data, 0x00); }

void fillFrame(uint8_t *data, uint8_t value) {
  for (uint8_t i = 0; i < 8; i++) {
    data[i] = value;
  }
}

void setSignal(uint8_t *data, uint8_t startBit, uint8_t length,
               uint32_t rawValue) {
  for (uint8_t i = 0; i < length; i++) {
    const uint8_t absoluteBit = startBit + i;
    const uint8_t byteIndex = absoluteBit / 8;
    const uint8_t bitInByte = absoluteBit % 8;
    if ((rawValue >> i) & 0x01) {
      data[byteIndex] |= (1 << bitInByte);
    } else {
      data[byteIndex] &= ~(1 << bitInByte);
    }
  }
}

uint16_t le16Raw(float value) {
  if (value < 0) {
    return 0;
  }
  if (value > 65535) {
    return 65535;
  }
  return (uint16_t)(value + 0.5f);
}

uint32_t le32Raw(float value) {
  if (value < 0) {
    return 0;
  }
  if (value > 4294967295.0f) {
    return 0xFFFFFFFFUL;
  }
  return (uint32_t)(value + 0.5f);
}

void putLe16(uint8_t *data, uint8_t offset, uint16_t value) {
  data[offset] = (uint8_t)(value & 0xFF);
  data[offset + 1] = (uint8_t)(value >> 8);
}

void putLe32(uint8_t *data, uint8_t offset, uint32_t value) {
  data[offset] = (uint8_t)(value & 0xFF);
  data[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
  data[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
  data[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
}
