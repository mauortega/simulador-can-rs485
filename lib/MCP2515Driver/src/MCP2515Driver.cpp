#include "MCP2515Driver.h"

static bool gCanSpiStarted = false;

MCP2515Driver::MCP2515Driver(uint8_t csPin, const char *name)
    : mCsPin(csPin), mName(name) {}

bool MCP2515Driver::begin(SPIClass &spi, uint8_t sck, uint8_t miso,
                          uint8_t mosi, BitRate bitrate, uint32_t quartzHz,
                          bool log) {
  mSpi = &spi;
  mSck = sck;
  mMiso = miso;
  mMosi = mosi;
  mBitrate = bitrate;
  mQuartzHz = quartzHz;

  pinMode(mCsPin, OUTPUT);
  digitalWrite(mCsPin, HIGH);

  if (mCan == nullptr) {
    mCan = new (mCanStorage) MCP_CAN(mCsPin);
  }

  if (!gCanSpiStarted) {
    mSpi->begin();
    gCanSpiStarted = true;
  }

  const bool ok = configureController();

  if (log) {
    Serial.print('['); Serial.print(mName); Serial.print(F("] begin="));
    Serial.print(ok ? F("OK") : F("FAIL")); Serial.print(F(" code="));
    Serial.print(mStats.lastError); Serial.print(F(" | CS="));
    Serial.print(mCsPin); Serial.print(F(" | bitrate="));
    Serial.print(actualBitrate()); Serial.print(F(" | clock="));
    Serial.println(mQuartzHz);
  }

  return ok;
}

bool MCP2515Driver::send(uint32_t id, bool extended, const uint8_t *data,
                         uint8_t len) {
  if (data == nullptr || len > 8) {
    mStats.txFail++;
    return false;
  }

  if (!mStarted) {
    mStats.txFail++;
    recover();
    return false;
  }

  uint8_t buffer[8] = {0};
  memcpy(buffer, data, len);

  uint8_t err = mCan->sendMsgBuf(id, extended ? 1 : 0, len, buffer);
  if (err == CAN_OK) {
    mStats.txOk++;
    return true;
  }

  mCan->abortTX();
  mStats.txFail++;
  mStats.lastError = err;
  recover();
  return false;
}

bool MCP2515Driver::setBitrate(BitRate bitrate) {
  mBitrate = bitrate;
  if (mCan == nullptr) {
    return false;
  }

  mCan->abortTX();
  return configureController();
}

void MCP2515Driver::poll() {
  if (!mStarted) {
    recover();
    return;
  }

  if (mCan != nullptr && mCan->checkError() == CAN_CTRLERROR) {
    recover();
  }
}

void MCP2515Driver::recover() {
  if (mCan == nullptr) {
    return;
  }

  const uint32_t nowMs = millis();
  if (mLastRecoverMs != 0 && (uint32_t)(nowMs - mLastRecoverMs) < 250UL) {
    mCan->abortTX();
    return;
  }
  mLastRecoverMs = nowMs;

  mCan->abortTX();
  configureController();
  mStats.recoveries++;
}

const MCP2515Driver::Stats &MCP2515Driver::stats() const { return mStats; }

uint8_t MCP2515Driver::csPin() const { return mCsPin; }

const char *MCP2515Driver::name() const { return mName; }

uint32_t MCP2515Driver::actualBitrate() const {
  return static_cast<uint32_t>(mBitrate);
}

bool MCP2515Driver::configureController() {
  if (mCan == nullptr) {
    mStarted = false;
    return false;
  }

  const uint8_t err =
      mCan->begin(MCP_ANY, mapBitrate(mBitrate), mapClock(mQuartzHz));
  mStats.lastError = err;
  mStarted = (err == CAN_OK);

  if (mStarted) {
    mCan->setMode(MCP_NORMAL);
    mCan->enOneShotTX();
  }

  return mStarted;
}

uint8_t MCP2515Driver::mapBitrate(BitRate bitrate) {
  switch (bitrate) {
  case BitRate::K250:
    return CAN_250KBPS;
  case BitRate::K500:
  default:
    return CAN_500KBPS;
  }
}

uint8_t MCP2515Driver::mapClock(uint32_t quartzHz) {
  switch (quartzHz) {
  case 8000000UL:
  default:
    return MCP_8MHZ;
  }
}
