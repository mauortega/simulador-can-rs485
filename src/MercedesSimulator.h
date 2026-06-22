#pragma once

#include "CanTypes.h"

class MercedesSimulator {
public:
  struct State {
    float rpm = 1200.0f;
    float speed = 60.0f;
    float waterTemp = 80.0f;
    float externalTemp = 30.0f;
    float airPressure = 8.2f;
    float voltage = 27.0f;
    float oilPressureBar = 4.0f;
    float pedal = 35.0f;
    float dieselLevel = 80.0f;
    float adblueLevel = 60.0f;
    float adblueConsAvgMl100Km = 20.0f;
    float dieselConsAvgMl100Km = 20000.0f;
    uint32_t odometerM = 25000000UL;
    int gear = 4;
    int retarder = 1;
  };

  void sendDue(CanFrameSink &sink, uint8_t busIndex, uint32_t nowMs);
  void tick();
  const State &state() const;

private:
  enum FrameIndex : uint8_t {
    BrakeSpeedRpm,
    Gear,
    Retarder,
    Pedal,
    AirVoltage,
    EngineTempOil,
    ExternalTemp,
    FuelEconomy,
    Odometer,
    AdblueLevel,
    DieselLevel,
    FrameCount,
  };

  static uint8_t gearToRaw(int gear);
  bool shouldSend(FrameIndex frameIndex, uint32_t nowMs) const;
  void markSent(FrameIndex frameIndex, uint32_t nowMs);
  void sendFrame(CanFrameSink &sink, uint8_t busIndex, uint32_t id,
                 const uint8_t *data);

  State mState;
  uint32_t mLastSent[FrameCount] = {};
};
