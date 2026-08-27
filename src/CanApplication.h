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
  void processSerialCommand();
  void setCanBitrate(MCP2515Driver::BitRate bitrate);
  void updateBitrateLeds();
  void runSimulation(uint32_t nowMs);
  void printStats(uint32_t nowMs);
  void printTxFrame(uint8_t busIndex, const CanFrame &frame, bool ok);
  MCP2515Driver::Stats statsFor(uint8_t busIndex) const;
  bool isMercedesMode() const;

  SPIClass &mSpi;
  MCP2515Driver mCanBuses[AppConfig::CanBusCount];
  J1939Simulator mJ1939;
  MercedesSimulator mMercedes;
  uint32_t mStateUpdates = 0;
  uint32_t mLastStateUpdateMs = 0;
  uint32_t mLastStatsMs = 0;
  char mSerialCommand[7] = {};
  uint8_t mSerialCommandLength = 0;
};
