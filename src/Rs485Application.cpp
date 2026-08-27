#include "Rs485Application.h"
#include <isrSerial.h>

namespace {
static const uint8_t kValidatorTestPacket[] = {
    0x01, 0x30, 0x10, 0x21, 0xF9, 0x10, 0x21, 0x10, 0x21, 0x00,
    0x0A, 0x71, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x21,
    0x62, 0xBB, 0x00, 0x10, 0x21, 0x3D, 0x75, 0xC1, 0x04,
};
} // namespace

void Rs485Application::begin() {
  pinMode(DirectionPin, OUTPUT);
  digitalWrite(DirectionPin, LOW);
  isrSerial.begin(Baud);
  Serial.print(F("[RS485] isrSerial TX=4 RX=5 DE/RE=6 | baud="));
  Serial.print(Baud); Serial.print(F(" | period="));
  Serial.print(TxPeriodMs); Serial.println(F("ms"));
}

bool Rs485Application::loop(uint32_t nowMs) {
  if (mTransmitting) {
    if (!isrSerial.txBusy()) {
      digitalWrite(DirectionPin, LOW);
      mTransmitting = false;
    }
    return false;
  }

  if ((nowMs - mLastTxMs) < TxPeriodMs) {
    return false;
  }

  const size_t packetSize = sizeof(kValidatorTestPacket);
  if (isrSerial.availableForWrite() < static_cast<int>(packetSize)) {
    return false;
  }

  mLastTxMs = nowMs;
  digitalWrite(DirectionPin, HIGH);
  delayMicroseconds(10);
  const size_t bytesWritten = isrSerial.write(kValidatorTestPacket, packetSize);
  mTransmitting = bytesWritten == packetSize;
  if (!mTransmitting) {
    digitalWrite(DirectionPin, LOW);
  }
  Serial.print(F("[RS485] TX ")); Serial.print(packetSize);
  Serial.println(F(" bytes"));
  return mTransmitting;
}
