#include "MercedesSimulator.h"

#include "MercedesSignalTable.h"
#include "SignalEncoding.h"

namespace {
static constexpr uint32_t kMercedesPeriodsMs[] = {
    100,  // 0x250 brake/speed/rpm
    100,  // 0x22C gear
    100,  // 0x304 retarder
    100,  // 0x450 accelerator pedal
    500,  // 0x5A0 air pressure / voltage
    500,  // 0x554 engine temp / oil pressure
    1000, // 0x550 external temperature
    1000, // 0x65E fuel economy / AdBlue average
    1000, // 0x6B5 odometer
    2000, // 0x65F AdBlue level
    2000, // 0x6A0 diesel level
};

static uint32_t rawFromPhysical(float value,
                                const MercedesTable::SignalSpec &signal) {
  const float clamped = clampFloat(value, signal.min, signal.max);
  return (uint32_t)((clamped - signal.offset) / signal.factor + 0.5f);
}
} // namespace

void MercedesSimulator::sendDue(CanFrameSink &sink, uint8_t busIndex,
                                uint32_t nowMs) {
  uint8_t d[8];

  if (shouldSend(BrakeSpeedRpm, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::HandBrake.pos, MercedesTable::HandBrake.len,
              rawFromPhysical(0.0f, MercedesTable::HandBrake));
    setSignal(d, MercedesTable::FootBrake.pos, MercedesTable::FootBrake.len,
              rawFromPhysical(0.0f, MercedesTable::FootBrake));
    setSignal(d, MercedesTable::Speed.pos, MercedesTable::Speed.len,
              rawFromPhysical(mState.speed, MercedesTable::Speed));
    setSignal(d, MercedesTable::Rpm.pos, MercedesTable::Rpm.len,
              rawFromPhysical(mState.rpm, MercedesTable::Rpm));
    sendFrame(sink, busIndex, MercedesTable::Speed.msgId, d);
    markSent(BrakeSpeedRpm, nowMs);
  }

  if (shouldSend(Gear, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::Gear.pos, MercedesTable::Gear.len,
              gearToRaw(mState.gear));
    sendFrame(sink, busIndex, MercedesTable::Gear.msgId, d);
    markSent(Gear, nowMs);
  }

  if (shouldSend(Retarder, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::Retarder.pos, MercedesTable::Retarder.len,
              rawFromPhysical(mState.retarder - 1, MercedesTable::Retarder));
    sendFrame(sink, busIndex, MercedesTable::Retarder.msgId, d);
    markSent(Retarder, nowMs);
  }

  if (shouldSend(Pedal, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::GasPedalPos.pos,
              MercedesTable::GasPedalPos.len,
              rawFromPhysical(mState.pedal, MercedesTable::GasPedalPos));
    sendFrame(sink, busIndex, MercedesTable::GasPedalPos.msgId, d);
    markSent(Pedal, nowMs);
  }

  if (shouldSend(AirVoltage, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::AirPressure.pos,
              MercedesTable::AirPressure.len,
              rawFromPhysical(mState.airPressure, MercedesTable::AirPressure));
    setSignal(d, MercedesTable::PowerVoltage.pos,
              MercedesTable::PowerVoltage.len,
              rawFromPhysical(mState.voltage, MercedesTable::PowerVoltage));
    sendFrame(sink, busIndex, MercedesTable::AirPressure.msgId, d);
    markSent(AirVoltage, nowMs);
  }

  if (shouldSend(EngineTempOil, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::WaterTemp.pos, MercedesTable::WaterTemp.len,
              rawFromPhysical(mState.waterTemp, MercedesTable::WaterTemp));
    setSignal(d, MercedesTable::EngOilPress.pos,
              MercedesTable::EngOilPress.len,
              rawFromPhysical(mState.oilPressureBar,
                              MercedesTable::EngOilPress));
    sendFrame(sink, busIndex, MercedesTable::WaterTemp.msgId, d);
    markSent(EngineTempOil, nowMs);
  }

  if (shouldSend(ExternalTemp, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::EnvTemp.pos, MercedesTable::EnvTemp.len,
              rawFromPhysical(mState.externalTemp, MercedesTable::EnvTemp));
    sendFrame(sink, busIndex, MercedesTable::EnvTemp.msgId, d);
    markSent(ExternalTemp, nowMs);
  }

  if (shouldSend(FuelEconomy, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::DieselConsAvg.pos,
              MercedesTable::DieselConsAvg.len,
              rawFromPhysical(mState.dieselConsAvgMl100Km,
                              MercedesTable::DieselConsAvg));
    setSignal(d, MercedesTable::AdBlueConsAvg.pos,
              MercedesTable::AdBlueConsAvg.len,
              rawFromPhysical(mState.adblueConsAvgMl100Km,
                              MercedesTable::AdBlueConsAvg));
    sendFrame(sink, busIndex, MercedesTable::DieselConsAvg.msgId, d);
    markSent(FuelEconomy, nowMs);
  }

  if (shouldSend(Odometer, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::TripTotal.pos, MercedesTable::TripTotal.len,
              rawFromPhysical((float)mState.odometerM,
                              MercedesTable::TripTotal));
    sendFrame(sink, busIndex, MercedesTable::TripTotal.msgId, d);
    markSent(Odometer, nowMs);
  }

  if (shouldSend(AdblueLevel, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::AdBlueVol.pos, MercedesTable::AdBlueVol.len,
              rawFromPhysical(mState.adblueLevel, MercedesTable::AdBlueVol));
    sendFrame(sink, busIndex, MercedesTable::AdBlueVol.msgId, d);
    markSent(AdblueLevel, nowMs);
  }

  if (shouldSend(DieselLevel, nowMs)) {
    clearFrame(d);
    setSignal(d, MercedesTable::DieselVol.pos, MercedesTable::DieselVol.len,
              rawFromPhysical(mState.dieselLevel, MercedesTable::DieselVol));
    sendFrame(sink, busIndex, MercedesTable::DieselVol.msgId, d);
    markSent(DieselLevel, nowMs);
  }
}

void MercedesSimulator::tick() {
  mState.rpm = clampFloat(mState.rpm + 7.0f, 1000.0f, 1600.0f);
  if (mState.rpm >= 1590.0f) {
    mState.rpm = 1200.0f;
  }

  mState.speed = clampFloat(mState.speed + 0.4f, 50.0f, 95.0f);
  if (mState.speed >= 94.0f) {
    mState.speed = 60.0f;
  }

  mState.odometerM += (uint32_t)(mState.speed / 7.2f);

  mState.pedal = clampFloat(mState.pedal + 0.5f, 20.0f, 60.0f);
  if (mState.pedal >= 59.0f) {
    mState.pedal = 35.0f;
  }
}

const MercedesSimulator::State &MercedesSimulator::state() const {
  return mState;
}

uint8_t MercedesSimulator::gearToRaw(int gear) {
  if (gear < 0) {
    return 0x0E;
  }
  if (gear == 0) {
    return 0x0F;
  }
  return (uint8_t)clampU32(0x10 + gear, 0x11, 0x1F);
}

bool MercedesSimulator::shouldSend(FrameIndex frameIndex,
                                   uint32_t nowMs) const {
  const uint32_t lastSent = mLastSent[frameIndex];
  return lastSent == 0 ||
         (uint32_t)(nowMs - lastSent) >= kMercedesPeriodsMs[frameIndex];
}

void MercedesSimulator::markSent(FrameIndex frameIndex, uint32_t nowMs) {
  mLastSent[frameIndex] = nowMs;
}

void MercedesSimulator::sendFrame(CanFrameSink &sink, uint8_t busIndex,
                                  uint32_t id, const uint8_t *data) {
  CanFrame frame;
  frame.id = id;
  frame.extended = false;
  frame.len = 8;
  memcpy(frame.data, data, sizeof(frame.data));
  sink.send(busIndex, frame);
}
