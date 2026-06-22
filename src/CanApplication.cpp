#include "CanApplication.h"

namespace {
struct DebugTxSlot {
  uint8_t busIndex = 0xFF;
  uint32_t id = 0;
  uint32_t lastMs = 0;
};

static constexpr uint8_t kDebugTxSlotCount = 40;
DebugTxSlot gDebugTxSlots[kDebugTxSlotCount];
} // namespace

CanApplication::CanApplication()
    : mSpi(SPI),
      mCanBuses{
          MCP2515Driver(AppConfig::CanBuses[0].csPin,
                        AppConfig::CanBuses[0].name),
          MCP2515Driver(AppConfig::CanBuses[1].csPin,
                        AppConfig::CanBuses[1].name),
          MCP2515Driver(AppConfig::CanBuses[2].csPin,
                        AppConfig::CanBuses[2].name),
          MCP2515Driver(AppConfig::CanBuses[3].csPin,
                        AppConfig::CanBuses[3].name),
      } {}

void CanApplication::begin() {
  Serial.println();
  Serial.println("=== CAN module: Mercedes CAN1 + J1939 CAN2 ===");
  Serial.printf("SPI SCK=%u MISO=%u MOSI=%u | MCP2515 clock=%lu\n",
                AppConfig::CanSck, AppConfig::CanMiso, AppConfig::CanMosi,
                (unsigned long)AppConfig::Mcp2515QuartzHz);

  initCanBuses();
}

void CanApplication::loop(uint32_t nowMs) {
  pollCanBuses();
  runSimulation(nowMs);
  printStats(nowMs);
}

bool CanApplication::send(uint8_t busIndex, const CanFrame &frame) {
  if (busIndex >= AppConfig::CanBusCount || !AppConfig::CanBuses[busIndex].enabled) {
    return false;
  }

  const bool ok = mCanBuses[busIndex].send(frame.id, frame.extended, frame.data,
                                          frame.len);
  printTxFrame(busIndex, frame, ok);
  mCanBuses[busIndex].poll();
  delayMicroseconds(250);
  return ok;
}

void CanApplication::initCanBuses() {
  bool allEnabledOk = true;

  for (uint8_t i = 0; i < AppConfig::CanBusCount; i++) {
    const AppConfig::CanBusConfig &cfg = AppConfig::CanBuses[i];
    pinMode(cfg.csPin, OUTPUT);
    digitalWrite(cfg.csPin, HIGH);
    Serial.printf("[%s] CS=%u | %s\n", cfg.name, cfg.csPin,
                  cfg.enabled ? "enabled" : "reserved");

    if (!cfg.enabled) {
      continue;
    }

    const bool ok =
        mCanBuses[i].begin(mSpi, AppConfig::CanSck, AppConfig::CanMiso,
                           AppConfig::CanMosi, cfg.bitrate,
                           AppConfig::Mcp2515QuartzHz);
    allEnabledOk = allEnabledOk && ok;
  }

  Serial.println(allEnabledOk ? "CAN enabled buses init OK"
                              : "CAN init COM ERRO");
}

void CanApplication::pollCanBuses() {
  for (uint8_t i = 0; i < AppConfig::CanBusCount; i++) {
    if (AppConfig::CanBuses[i].enabled) {
      mCanBuses[i].poll();
    }
  }
}

void CanApplication::runSimulation(uint32_t nowMs) {
  mMercedes.sendDue(*this, AppConfig::Can1Mercedes, nowMs);
  mJ1939Can2.sendDue(*this, AppConfig::Can2J1939, nowMs);
 
  if ((nowMs - mLastStateUpdateMs) < AppConfig::StateUpdateMs) {
    return;
  }

  mLastStateUpdateMs = nowMs;
  mMercedes.tick();
  mJ1939Can2.tick();
 
  mStateUpdates++;
}

void CanApplication::printStats(uint32_t nowMs) {
  if ((nowMs - mLastStatsMs) < AppConfig::StatsPeriodMs) {
    return;
  }

  mLastStatsMs = nowMs;
  const MCP2515Driver::Stats can1Stats = statsFor(AppConfig::Can1Mercedes);
  const MCP2515Driver::Stats can2Stats = statsFor(AppConfig::Can2J1939);
  const MercedesSimulator::State &mercedes = mMercedes.state();
  const J1939Simulator::State &j1939 = mJ1939Can2.state();

  Serial.printf("[CAN1] updates=%lu TX_OK=%lu TX_FAIL=%lu REC=%lu RPM=%.0f Vel=%.0f\n",
                (unsigned long)mStateUpdates, (unsigned long)can1Stats.txOk,
                (unsigned long)can1Stats.txFail,
                (unsigned long)can1Stats.recoveries, mercedes.rpm,
                mercedes.speed);
  Serial.printf("[CAN2] updates=%lu TX_OK=%lu TX_FAIL=%lu REC=%lu RPM=%.0f Vel=%.0f\n",
                (unsigned long)mStateUpdates, (unsigned long)can2Stats.txOk,
                (unsigned long)can2Stats.txFail,
                (unsigned long)can2Stats.recoveries, j1939.rpm, j1939.speed);
}

void CanApplication::printTxFrame(uint8_t busIndex, const CanFrame &frame,
                                  bool ok) {
  if (!AppConfig::DebugTxFrames ||
      (busIndex != AppConfig::Can1Mercedes &&
       busIndex != AppConfig::Can2J1939)) {
    return;
  }

  const uint32_t nowMs = millis();
  DebugTxSlot *slot = nullptr;
  for (uint8_t i = 0; i < kDebugTxSlotCount; i++) {
    DebugTxSlot &candidate = gDebugTxSlots[i];
    if (candidate.busIndex == busIndex && candidate.id == frame.id) {
      slot = &candidate;
      break;
    }
    if (slot == nullptr && candidate.busIndex == 0xFF) {
      slot = &candidate;
    }
  }

  if (slot == nullptr) {
    return;
  }

  if (slot->busIndex != 0xFF &&
      (nowMs - slot->lastMs) < AppConfig::DebugTxPeriodMs) {
    return;
  }
  slot->busIndex = busIndex;
  slot->id = frame.id;
  slot->lastMs = nowMs;

  const AppConfig::CanBusConfig &cfg = AppConfig::CanBuses[busIndex];
  Serial.printf("[TX %s] %s ID=%s%08lX LEN=%u DATA=",
                cfg.name, ok ? "OK" : "FAIL", frame.extended ? "X" : "S",
                (unsigned long)frame.id, frame.len);

  for (uint8_t i = 0; i < frame.len; i++) {
    Serial.printf("%02X", frame.data[i]);
    if (i + 1 < frame.len) {
      Serial.print(' ');
    }
  }
  Serial.println();
}

MCP2515Driver::Stats CanApplication::statsFor(uint8_t busIndex) const {
  if (busIndex >= AppConfig::CanBusCount) {
    return MCP2515Driver::Stats{};
  }
  return mCanBuses[busIndex].stats();
}
