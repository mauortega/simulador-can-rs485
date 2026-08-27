#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>
#include <new>

class MCP2515Driver {
public:
  enum class BitRate : uint32_t {
    K250 = 250000UL,
    K500 = 500000UL,
  };

  struct Stats {
    uint32_t txOk = 0;
    uint32_t txFail = 0;
    uint32_t recoveries = 0;
    uint8_t lastError = 0;
  };

  MCP2515Driver(uint8_t csPin, const char *name);

  bool begin(SPIClass &spi, uint8_t sck, uint8_t miso, uint8_t mosi,
             BitRate bitrate, uint32_t quartzHz = 8000000UL,
             bool log = true);
  bool send(uint32_t id, bool extended, const uint8_t *data, uint8_t len = 8);
  bool setBitrate(BitRate bitrate);
  void poll();
  void recover();

  const Stats &stats() const;
  uint8_t csPin() const;
  const char *name() const;
  uint32_t actualBitrate() const;

private:
  bool configureController();
  static uint8_t mapBitrate(BitRate bitrate);
  static uint8_t mapClock(uint32_t quartzHz);

  alignas(MCP_CAN) uint8_t mCanStorage[sizeof(MCP_CAN)] = {};
  MCP_CAN *mCan = nullptr;
  const uint8_t mCsPin;
  const char *const mName;
  SPIClass *mSpi = nullptr;
  uint8_t mSck = 0;
  uint8_t mMiso = 0;
  uint8_t mMosi = 0;
  BitRate mBitrate = BitRate::K500;
  uint32_t mQuartzHz = 8000000UL;
  uint32_t mLastRecoverMs = 0;
  bool mStarted = false;
  Stats mStats;
};
