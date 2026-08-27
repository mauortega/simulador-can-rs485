#pragma once

#include <Arduino.h>
#include <MCP2515Driver.h>

namespace AppConfig {

// SPI hardware fixo do Arduino Nano (ATmega328P).
static constexpr uint8_t CanSck = 13;
static constexpr uint8_t CanMiso = 12;
static constexpr uint8_t CanMosi = 11;
static constexpr uint32_t Mcp2515QuartzHz = 8000000UL;
static constexpr uint32_t StateUpdateMs = 500UL;
static constexpr uint32_t StatsPeriodMs = 2000UL;
static constexpr uint32_t LoopDelayMs = 1UL;
static constexpr bool DebugTxFrames = false;
static constexpr uint32_t DebugTxPeriodMs = 250UL;
static constexpr uint8_t Rs485ActivityLedPin = A0;
static constexpr uint32_t Rs485ActivityLedPulseMs = 120UL;
static constexpr bool Rs485ActivityLedActiveLow = false;
static constexpr uint8_t Can250LedPin = A1;
static constexpr uint8_t Can500LedPin = A2;

enum BusIndex : uint8_t {
  CanJ1939 = 0,
  CanBusCount = 1,
};

static constexpr uint8_t CanInterruptPin = 3;

struct CanBusConfig {
  const char *name;
  uint8_t csPin;
  MCP2515Driver::BitRate bitrate;
  bool enabled;
};

static constexpr CanBusConfig CanBuses[CanBusCount] = {
    {"CAN", 8, MCP2515Driver::BitRate::K250, true},
};

} // namespace AppConfig
