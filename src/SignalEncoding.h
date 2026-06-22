#pragma once

#include <Arduino.h>

float clampFloat(float value, float lo, float hi);
uint32_t clampU32(int32_t value, uint32_t lo, uint32_t hi);
void clearFrame(uint8_t *data);
void fillFrame(uint8_t *data, uint8_t value);
void setSignal(uint8_t *data, uint8_t startBit, uint8_t length,
               uint32_t rawValue);
uint16_t le16Raw(float value);
uint32_t le32Raw(float value);
void putLe16(uint8_t *data, uint8_t offset, uint16_t value);
void putLe32(uint8_t *data, uint8_t offset, uint32_t value);
