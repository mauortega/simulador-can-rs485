#include "Rs485Application.h"

namespace {
static const uint8_t kValidatorTestPacket[] = {
    0x01, 0x30, 0x10, 0x21, 0xF9, 0x10, 0x21, 0x10, 0x21, 0x00,
    0x0A, 0x71, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x21,
    0x62, 0xBB, 0x00, 0x10, 0x21, 0x3D, 0x75, 0xC1, 0x04,
};
} // namespace

void Rs485Application::begin() {
  Serial2.begin(Baud, SERIAL_8N1, -1, TxPin);
  Serial.printf("[RS485] TX only | TX=%d | baud=%lu | period=%lums\n", TxPin,
                (unsigned long)Baud, (unsigned long)TxPeriodMs);
}

void Rs485Application::loop(uint32_t nowMs) {
  if ((nowMs - mLastTxMs) < TxPeriodMs) {
    return;
  }

  mLastTxMs = nowMs;
  Serial2.write(kValidatorTestPacket, sizeof(kValidatorTestPacket));
  Serial2.flush();
  Serial.printf("[RS485] TX %u bytes\n",
                (unsigned int)sizeof(kValidatorTestPacket));
}
