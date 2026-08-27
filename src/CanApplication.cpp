#include "CanApplication.h"

namespace {
struct DebugTxSlot {
  uint8_t busIndex = 0xFF;
  uint32_t id = 0;
  uint32_t lastMs = 0;
};

static constexpr uint8_t kDebugTxSlotCount = 12;
DebugTxSlot gDebugTxSlots[kDebugTxSlotCount];
} // namespace

CanApplication::CanApplication()
    : mSpi(SPI),
      mCanBuses{
          MCP2515Driver(AppConfig::CanBuses[0].csPin,
                        AppConfig::CanBuses[0].name),
      } {}

void CanApplication::begin() {
  pinMode(AppConfig::Can250LedPin, OUTPUT);
  pinMode(AppConfig::Can500LedPin, OUTPUT);
  digitalWrite(AppConfig::Can250LedPin, LOW);
  digitalWrite(AppConfig::Can500LedPin, LOW);

  Serial.println();
  Serial.println(F("=== CAN: can250 = J1939 | can500 = Mercedes ==="));
  Serial.print(F("SPI SCK=13 MISO=12 MOSI=11 | MCP2515 clock="));
  Serial.println(AppConfig::Mcp2515QuartzHz);

  initCanBuses();
  updateBitrateLeds();
  pinMode(AppConfig::CanInterruptPin, INPUT_PULLUP);
  Serial.println(F("Comandos CAN: can250 ou can500 + Enter"));
}

void CanApplication::loop(uint32_t nowMs) {
  processSerialCommand();
  pollCanBuses();
  runSimulation(nowMs);
  printStats(nowMs);
}

void CanApplication::processSerialCommand() {
  while (Serial.available() > 0) {
    char received = (char)Serial.read();
    if (received == '\r' || received == '\n') {
      if (mSerialCommandLength == 0) {
        continue;
      }

      mSerialCommand[mSerialCommandLength] = '\0';
      if (strcmp(mSerialCommand, "can250") == 0) {
        setCanBitrate(MCP2515Driver::BitRate::K250);
      } else if (strcmp(mSerialCommand, "can500") == 0) {
        setCanBitrate(MCP2515Driver::BitRate::K500);
      } else {
        Serial.println(F("Comando invalido. Use can250 ou can500."));
      }
      mSerialCommandLength = 0;
      continue;
    }

    if (received >= 'A' && received <= 'Z') {
      received = (char)(received - 'A' + 'a');
    }

    const bool validCharacter =
        (received >= 'a' && received <= 'z') ||
        (received >= '0' && received <= '9');
    if (validCharacter && mSerialCommandLength < sizeof(mSerialCommand) - 1) {
      mSerialCommand[mSerialCommandLength++] = received;
    } else if (received != ' ' && received != '\t') {
      mSerialCommandLength = 0;
    }
  }
}

void CanApplication::setCanBitrate(MCP2515Driver::BitRate bitrate) {
  const bool ok = mCanBuses[AppConfig::CanJ1939].setBitrate(bitrate);
  Serial.print(F("CAN "));
  Serial.print(static_cast<uint32_t>(bitrate) / 1000UL);
  Serial.print(F(" kbit/s: "));
  Serial.print(ok ? F("OK") : F("ERRO"));
  if (ok) {
    updateBitrateLeds();
    Serial.print(F(" | "));
    Serial.println(bitrate == MCP2515Driver::BitRate::K500 ? F("Mercedes")
                                                           : F("J1939"));
  } else {
    Serial.println();
  }
}

void CanApplication::updateBitrateLeds() {
  const bool is500 = isMercedesMode();
  digitalWrite(AppConfig::Can250LedPin, is500 ? LOW : HIGH);
  digitalWrite(AppConfig::Can500LedPin, is500 ? HIGH : LOW);
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
    Serial.print('['); Serial.print(cfg.name); Serial.print(F("] CS="));
    Serial.print(cfg.csPin); Serial.println(F(" | enabled"));

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
  if (isMercedesMode()) {
    mMercedes.sendDue(*this, AppConfig::CanJ1939, nowMs);
  } else {
    mJ1939.sendDue(*this, AppConfig::CanJ1939, nowMs);
  }

  if ((nowMs - mLastStateUpdateMs) < AppConfig::StateUpdateMs) {
    return;
  }

  mLastStateUpdateMs = nowMs;
  if (isMercedesMode()) {
    mMercedes.tick();
  } else {
    mJ1939.tick();
  }

  mStateUpdates++;
}

void CanApplication::printStats(uint32_t nowMs) {
  if ((nowMs - mLastStatsMs) < AppConfig::StatsPeriodMs) {
    return;
  }

  mLastStatsMs = nowMs;
  const MCP2515Driver::Stats canStats = statsFor(AppConfig::CanJ1939);
  const bool mercedes = isMercedesMode();
  const float rpm = mercedes ? mMercedes.state().rpm : mJ1939.state().rpm;
  const float speed = mercedes ? mMercedes.state().speed : mJ1939.state().speed;

  Serial.print(F("[CAN] "));
  Serial.print(mercedes ? F("MB500") : F("VW250"));
  Serial.print(F(" updates=")); Serial.print(mStateUpdates);
  Serial.print(F(" TX_OK=")); Serial.print(canStats.txOk);
  Serial.print(F(" TX_FAIL=")); Serial.print(canStats.txFail);
  Serial.print(F(" REC=")); Serial.print(canStats.recoveries);
  Serial.print(F(" RPM=")); Serial.print(rpm, 0);
  Serial.print(F(" Vel=")); Serial.println(speed, 0);
}

void CanApplication::printTxFrame(uint8_t busIndex, const CanFrame &frame,
                                  bool ok) {
  if (!AppConfig::DebugTxFrames ||
      busIndex != AppConfig::CanJ1939) {
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
  Serial.print(F("[TX ")); Serial.print(cfg.name); Serial.print(F("] "));
  Serial.print(ok ? F("OK") : F("FAIL")); Serial.print(F(" ID="));
  Serial.print(frame.extended ? 'X' : 'S'); Serial.print(frame.id, HEX);
  Serial.print(F(" LEN=")); Serial.print(frame.len); Serial.print(F(" DATA="));

  for (uint8_t i = 0; i < frame.len; i++) {
    if (frame.data[i] < 0x10) Serial.print('0');
    Serial.print(frame.data[i], HEX);
    if (i + 1 < frame.len) {
      Serial.print(' ');
    }
  }
  Serial.println();
}

bool CanApplication::isMercedesMode() const {
  return mCanBuses[AppConfig::CanJ1939].actualBitrate() ==
         static_cast<uint32_t>(MCP2515Driver::BitRate::K500);
}

MCP2515Driver::Stats CanApplication::statsFor(uint8_t busIndex) const {
  if (busIndex >= AppConfig::CanBusCount) {
    return MCP2515Driver::Stats{};
  }
  return mCanBuses[busIndex].stats();
}
