#include "J1939Simulator.h"

#include "SignalEncoding.h"

namespace {
static constexpr uint32_t kJ1939PeriodsMs[] = {
    100,  // EEC1 - engine speed
    1000, // ET1 - engine temperature
    1000, // ambient/cabin temperature
    1000, // vehicle distance
    1000, // engine hours
    1000, // total fuel
    100,  // CCVS - vehicle speed
    100,  // TSC1 - pedal/load
    500,  // fluid pressure
    1000, // electrical power
    1000, // fuel economy
    1000, // dash display
    200,  // gear
    100,  // brake
    2000, // DEF level
    500,  // air supply pressure
    1000, // FMS standard
    500,  // retarder
    1000, // fuel consumption
    1000, // tachograph
};
} // namespace

void J1939Simulator::sendDue(CanFrameSink &sink, uint8_t busIndex,
                             uint32_t nowMs) {
  uint8_t d[8];

  if (shouldSend(Eec1, nowMs)) {
    fillFrame(d, 0xFF);
    putLe16(d, 3, le16Raw(mState.rpm / 0.125f));
    sendFrame(sink, busIndex, 0x18F00400UL, d);
    markSent(Eec1, nowMs);
  }

  if (shouldSend(Et1, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = (uint8_t)clampU32((int32_t)(mState.temp + 40.0f + 0.5f), 0, 250);
    sendFrame(sink, busIndex, 0x18FEEE00UL, d);
    markSent(Et1, nowMs);
  }

  if (shouldSend(Amb, nowMs)) {
    fillFrame(d, 0xFF);
    putLe16(d, 1, le16Raw((roundf(mState.cabinTemp) + 273.0f) / 0.03125f));
    putLe16(d, 3, le16Raw((roundf(mState.ambientTemp) + 273.0f) / 0.03125f));
    sendFrame(sink, busIndex, 0x18FEF500UL, d);
    markSent(Amb, nowMs);
  }

  if (shouldSend(VehicleDistance, nowMs)) {
    fillFrame(d, 0xFF);
    putLe32(d, 0, le32Raw(roundf(mState.odometerKm) / 0.005f));
    sendFrame(sink, busIndex, 0x18FEC100UL, d);
    markSent(VehicleDistance, nowMs);
  }

  if (shouldSend(EngineHours, nowMs)) {
    fillFrame(d, 0xFF);
    putLe32(d, 0, le32Raw(roundf(mState.hours) / 0.05f));
    sendFrame(sink, busIndex, 0x18FEE500UL, d);
    markSent(EngineHours, nowMs);
  }

  if (shouldSend(FuelTotal, nowMs)) {
    fillFrame(d, 0xFF);
    putLe32(d, 4, le32Raw(roundf(mState.fuelTotalL) * 1000.0f));
    sendFrame(sink, busIndex, 0x18FD0900UL, d);
    markSent(FuelTotal, nowMs);
  }

  if (shouldSend(Ccvs, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = (uint8_t)((0xFF & ~(0b11 << 2)) | (0 << 2));
    putLe16(d, 1, le16Raw(mState.speed * 256.0f));
    d[3] = 0xFF;
    sendFrame(sink, busIndex, 0x18FEF100UL, d);
    markSent(Ccvs, nowMs);
  }

  if (shouldSend(Tsc1, nowMs)) {
    fillFrame(d, 0xFF);
    d[1] = (uint8_t)clampU32((int32_t)(roundf(mState.pedal) / 0.4f), 0, 250);
    d[2] = (uint8_t)clampU32((int32_t)roundf(mState.engineLoad), 0, 250);
    sendFrame(sink, busIndex, 0x0CF00300UL, d);
    markSent(Tsc1, nowMs);
  }

  if (shouldSend(FluidPressure, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = (uint8_t)clampU32(
        (int32_t)(roundf(mState.fuelPressureKpa / 100.0f) * 100.0f / 4.0f),
        0, 250);
    d[3] = (uint8_t)clampU32(
        (int32_t)(roundf(mState.oilPressureKpa / 100.0f) * 100.0f / 4.0f), 0,
        250);
    sendFrame(sink, busIndex, 0x18FEEF00UL, d);
    markSent(FluidPressure, nowMs);
  }

  if (shouldSend(VehicleElectricalPower, nowMs)) {
    fillFrame(d, 0xFF);
    putLe16(d, 2, le16Raw(roundf(mState.altVoltage) / 0.05f));
    putLe16(d, 4, le16Raw(roundf(mState.altVoltage) / 0.05f));
    sendFrame(sink, busIndex, 0x18FEF700UL, d);
    markSent(VehicleElectricalPower, nowMs);
  }

  if (shouldSend(FuelEconomy, nowMs)) {
    fillFrame(d, 0xFF);
    putLe16(d, 0, le16Raw(roundf(fuelRate()) / 0.05f));
    putLe16(d, 2, le16Raw(roundf(fuelEconomy()) * 512.0f));
    putLe16(d, 4, le16Raw(roundf(fuelEconomy()) * 512.0f));
    sendFrame(sink, busIndex, 0x18FEF200UL, d);
    markSent(FuelEconomy, nowMs);
  }

  if (shouldSend(DashDisplay, nowMs)) {
    fillFrame(d, 0xFF);
    d[1] =
        (uint8_t)clampU32((int32_t)(roundf(mState.fuelLevel) / 0.4f), 0, 250);
    sendFrame(sink, busIndex, 0x18FEFC00UL, d);
    markSent(DashDisplay, nowMs);
  }

  if (shouldSend(Gear, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = (uint8_t)clampU32(gear() + 125, 0, 250);
    d[3] = (uint8_t)clampU32(gear() + 125, 0, 250);
    sendFrame(sink, busIndex, 0x18F00500UL, d);
    markSent(Gear, nowMs);
  }

  if (shouldSend(Brake, nowMs)) {
    fillFrame(d, 0xFF);
    d[1] =
        (uint8_t)clampU32((int32_t)(roundf(mState.brakePedal) / 0.4f), 0, 250);
    sendFrame(sink, busIndex, 0x18F00100UL, d);
    markSent(Brake, nowMs);
  }

  if (shouldSend(DefLevel, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = (uint8_t)clampU32((int32_t)(roundf(mState.defLevel) / 0.4f), 0,
                             250);
    sendFrame(sink, busIndex, 0x18FE5600UL, d);
    markSent(DefLevel, nowMs);
  }

  if (shouldSend(AirSupplyPressure, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = d[1] = d[2] = (uint8_t)clampU32(
        (int32_t)(roundf(mState.airPressureKpa) / 8.0f), 0, 250);
    sendFrame(sink, busIndex, 0x18FECE00UL, d);
    markSent(AirSupplyPressure, nowMs);
  }

  if (shouldSend(FmsStandard, nowMs)) {
    fillFrame(d, 0xFF);
    d[0] = 0xFC;
    sendFrame(sink, busIndex, 0x18FEFF00UL, d);
    markSent(FmsStandard, nowMs);
  }

  if (shouldSend(Retarder, nowMs)) {
    fillFrame(d, 0xFF);
    d[1] =
        (uint8_t)clampU32((int32_t)(roundf(mState.retarder) + 125.0f), 0, 250);
    sendFrame(sink, busIndex, 0x18F00000UL, d);
    markSent(Retarder, nowMs);
  }

  if (shouldSend(FuelConsumption, nowMs)) {
    fillFrame(d, 0xFF);
    putLe32(d, 4, le32Raw(roundf(mState.totalFuelUsedL) / 0.5f));
    sendFrame(sink, busIndex, 0x18FEE900UL, d);
    markSent(FuelConsumption, nowMs);
  }

  if (shouldSend(Tachograph, nowMs)) {
    fillFrame(d, 0xFF);
    putLe16(d, 6, le16Raw(mState.speed * 256.0f));
    sendFrame(sink, busIndex, 0x18FE6C00UL, d);
    markSent(Tachograph, nowMs);
  }
}

void J1939Simulator::tick() {
  mState.rpm = clampFloat(mState.rpm + 7.0f, 1000.0f, 1600.0f);
  if (mState.rpm >= 1590.0f) {
    mState.rpm = 1200.0f;
  }

  mState.speed = clampFloat(mState.speed + 0.4f, 50.0f, 95.0f);
  if (mState.speed >= 94.0f) {
    mState.speed = 80.0f;
  }

  mState.engineLoad = clampFloat(mState.engineLoad + 0.5f, 35.0f, 65.0f);
  if (mState.engineLoad >= 64.0f) {
    mState.engineLoad = 45.0f;
  }

  mState.pedal = clampFloat(mState.pedal + 0.5f, 20.0f, 60.0f);
  if (mState.pedal >= 59.0f) {
    mState.pedal = 35.0f;
  }

  mState.odometerKm += mState.speed * (0.5f / 3600.0f);
  mState.hours += 0.5f / 3600.0f;
  const float liters = (mState.speed * (0.5f / 3600.0f)) / 3.5f;
  mState.fuelTotalL += liters;
  mState.totalFuelUsedL += liters;
}

const J1939Simulator::State &J1939Simulator::state() const { return mState; }

int J1939Simulator::gear() const {
  return max(1, min(6, (int)(mState.speed / 16.0f) + 1));
}

float J1939Simulator::fuelRate() const {
  return mState.speed > 0.0f ? mState.speed / 3.5f : 2.0f;
}

float J1939Simulator::fuelEconomy() const {
  const float rate = fuelRate();
  return rate > 0.0f ? mState.speed / rate : 0.0f;
}

bool J1939Simulator::shouldSend(FrameIndex frameIndex, uint32_t nowMs) const {
  const uint32_t lastSent = mLastSent[frameIndex];
  return lastSent == 0 ||
         (uint32_t)(nowMs - lastSent) >= kJ1939PeriodsMs[frameIndex];
}

void J1939Simulator::markSent(FrameIndex frameIndex, uint32_t nowMs) {
  mLastSent[frameIndex] = nowMs;
}

void J1939Simulator::sendFrame(CanFrameSink &sink, uint8_t busIndex,
                               uint32_t id, const uint8_t *data) {
  CanFrame frame;
  frame.id = id;
  frame.extended = true;
  frame.len = 8;
  memcpy(frame.data, data, sizeof(frame.data));
  sink.send(busIndex, frame);
}
