#pragma once

#include <Arduino.h>
#include <MCP2515Driver.h>

namespace AppConfig {

static constexpr uint8_t CanSck = 18;
static constexpr uint8_t CanMiso = 19;
static constexpr uint8_t CanMosi = 23;
static constexpr uint32_t Mcp2515QuartzHz = 8000000UL;
static constexpr uint32_t StateUpdateMs = 500UL;
static constexpr uint32_t StatsPeriodMs = 2000UL;
static constexpr uint32_t LoopDelayMs = 1UL;
static constexpr bool DebugTxFrames = true;
static constexpr uint32_t DebugTxPeriodMs = 250UL;
static constexpr uint8_t StatusLedPin = 2;
static constexpr uint32_t StatusLedPeriodMs = 500UL;
static constexpr bool StatusLedActiveLow = true;

enum BusIndex : uint8_t {
  Can1Mercedes = 0,
  Can2J1939 = 1,
  Can3J1939 = 2,
  Can4Reserved = 3,
  CanBusCount = 4,
};

struct CanBusConfig {
  const char *name;
  uint8_t csPin;
  MCP2515Driver::BitRate bitrate;
  bool enabled;
};

static constexpr CanBusConfig CanBuses[CanBusCount] = {
    {"CAN1", 22, MCP2515Driver::BitRate::K500, true},
    {"CAN2", 21, MCP2515Driver::BitRate::K250, true},
    {"CAN3", 16, MCP2515Driver::BitRate::K250, false},
    {"CAN4", 17, MCP2515Driver::BitRate::K500, false},
};

} // namespace AppConfig
