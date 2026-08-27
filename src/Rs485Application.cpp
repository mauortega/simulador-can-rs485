#include "Rs485Application.h"
#include <SoftwareSerial.h>

namespace {
static const uint8_t kValidatorTestPacket[] = {
    0x01, 0x30, 0x10, 0x21, 0xF9, 0x10, 0x21, 0x10, 0x21, 0x00,
    0x0A, 0x71, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x21,
    0x62, 0xBB, 0x00, 0x10, 0x21, 0x3D, 0x75, 0xC1, 0x04,
};
SoftwareSerial rs485Serial(Rs485Application::RxPin, Rs485Application::TxPin);
} // namespace

void Rs485Application::begin() {
  pinMode(DirectionPin, OUTPUT);
  digitalWrite(DirectionPin, LOW);
  rs485Serial.begin(Baud);
  Serial.print(F("[RS485] TX=4 RX=5 DE/RE=6 | baud="));
  Serial.print(Baud); Serial.print(F(" | period="));
  Serial.print(TxPeriodMs); Serial.println(F("ms"));
}

bool Rs485Application::loop(uint32_t nowMs) {
  if ((nowMs - mLastTxMs) < TxPeriodMs) {
    return false;
  }

  const size_t packetSize = sizeof(kValidatorTestPacket);
  mLastTxMs = nowMs;
  digitalWrite(DirectionPin, HIGH);
  delayMicroseconds(10);
  const size_t bytesWritten =
      rs485Serial.write(kValidatorTestPacket, packetSize);
  rs485Serial.flush();
  digitalWrite(DirectionPin, LOW);
  Serial.print(F("[RS485] TX ")); Serial.print(packetSize);
  Serial.println(F(" bytes"));
  return bytesWritten == packetSize;
}
