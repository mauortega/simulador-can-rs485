#pragma once

#include "AppConfig.h"
#include "CanTypes.h"
#include "J1939Simulator.h"
#include "MercedesSimulator.h"

#include <MCP2515Driver.h>
#include <SPI.h>

class CanApplication : public CanFrameSink {
public:
  CanApplication();

  void begin();
  void loop(uint32_t nowMs);
  bool send(uint8_t busIndex, const CanFrame &frame) override;

private:
  void initCanBuses();
  void pollCanBuses();
  void runSimulation(uint32_t nowMs);
  void printStats(uint32_t nowMs);
  void printTxFrame(uint8_t busIndex, const CanFrame &frame, bool ok);
  MCP2515Driver::Stats statsFor(uint8_t busIndex) const;

  SPIClass &mSpi;
  MCP2515Driver mCanBuses[AppConfig::CanBusCount];
  MercedesSimulator mMercedes;
  J1939Simulator mJ1939Can2;
  uint32_t mStateUpdates = 0;
  uint32_t mLastStateUpdateMs = 0;
  uint32_t mLastStatsMs = 0;
};
