#pragma once

#include "CanTypes.h"

class J1939Simulator {
public:
  struct State {
    float rpm = 1200.0f;
    float speed = 80.0f;
    float temp = 80.0f;
    float cabinTemp = 20.0f;
    float ambientTemp = 30.0f;
    float odometerKm = 125000.0f;
    float hours = 3500.0f;
    float fuelTotalL = 15000.0f;
    float totalFuelUsedL = 15000.0f;
    float fuelLevel = 80.0f;
    float defLevel = 60.0f;
    float oilPressureKpa = 400.0f;
    float fuelPressureKpa = 350.0f;
    float altVoltage = 27.5f;
    float battVoltage = 27.2f;
    float airPressureKpa = 800.0f;
    float engineLoad = 45.0f;
    float pedal = 35.0f;
    float brakePedal = 0.0f;
    float retarder = 0.0f;
  };

  void sendDue(CanFrameSink &sink, uint8_t busIndex, uint32_t nowMs);
  void tick();
  const State &state() const;

private:
  enum FrameIndex : uint8_t {
    Eec1,
    Et1,
    Amb,
    VehicleDistance,
    EngineHours,
    FuelTotal,
    Ccvs,
    Tsc1,
    FluidPressure,
    VehicleElectricalPower,
    FuelEconomy,
    DashDisplay,
    Gear,
    Brake,
    DefLevel,
    AirSupplyPressure,
    FmsStandard,
    Retarder,
    FuelConsumption,
    Tachograph,
    FrameCount,
  };

  int gear() const;
  float fuelRate() const;
  float fuelEconomy() const;
  bool shouldSend(FrameIndex frameIndex, uint32_t nowMs) const;
  void markSent(FrameIndex frameIndex, uint32_t nowMs);
  void sendFrame(CanFrameSink &sink, uint8_t busIndex, uint32_t id,
                 const uint8_t *data);

  State mState;
  uint32_t mLastSent[FrameCount] = {};
};
