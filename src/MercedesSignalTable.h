#pragma once

#include <Arduino.h>

namespace MercedesTable {

struct SignalSpec {
  uint32_t msgId;
  uint8_t pos;
  uint8_t len;
  float factor;
  float offset;
  float min;
  float max;
  const char *unit;
  const char *name;
  bool avg;
};

// Tabela fornecida pelo usuario. Nao adicionar sinais Mercedes fora desta lista.
static constexpr SignalSpec Gear{0x22C, 34, 6, 1.0f, 0.0f, 0.0f, 63.0f,
                                 "gear", "Gear", false};
static constexpr SignalSpec HandBrake{0x250, 28, 2, 1.0f, 0.0f, 0.0f, 3.0f,
                                      "on/off", "HandBrake", false};
static constexpr SignalSpec FootBrake{0x250, 30, 2, 1.0f, 0.0f, 0.0f, 3.0f,
                                      "on/off", "FootBrake", false};
static constexpr SignalSpec Rpm{0x250, 48, 16, 0.16f, 0.0f, 0.0f, 8000.0f,
                                "Rpm", "RPM", false};
static constexpr SignalSpec Speed{0x250, 32, 16, 0.005f, 0.0f, 0.0f, 250.0f,
                                  "km/h", "Speed", false};
static constexpr SignalSpec Retarder{0x304, 18, 2, 1.0f, 0.0f, 0.0f, 3.0f,
                                     "on/off", "Retarder", false};
static constexpr SignalSpec GasPedalPos{0x450, 48, 8, 0.4f, 0.0f, 0.0f,
                                        100.0f, "%", "GasPedalPos", false};
static constexpr SignalSpec EnvTemp{0x550, 56, 8, 0.5f, -50.0f, -50.0f,
                                    75.0f, "oC", "EnvTemp", false};
static constexpr SignalSpec WaterTemp{0x554, 8, 8, 1.0f, -50.0f, -50.0f,
                                      200.0f, "oC", "WaterTemp", false};
static constexpr SignalSpec EngOilPress{0x554, 32, 8, 0.04f, 0.0f, 0.0f,
                                        10.0f, "bar", "EngOilPress", false};
static constexpr SignalSpec AirPressure{0x5A0, 40, 8, 0.08f, 0.0f, 0.0f,
                                        20.0f, "bar", "AirPressure", false};
static constexpr SignalSpec PowerVoltage{0x5A0, 56, 8, 0.2f, 0.0f, 0.0f,
                                         50.0f, "V", "PowerVoltage", false};
static constexpr SignalSpec DieselVol{0x6A0, 40, 8, 0.4f, 0.0f, 0.0f,
                                      100.0f, "%", "DieselVol", true};
static constexpr SignalSpec TripTotal{0x6B5, 0, 32, 5.0f, 0.0f, 0.0f,
                                      3875540000.0f, "m", "TripTotal",
                                      false};
static constexpr SignalSpec DieselConsAvg{0x65E, 0, 16, 4.0f, 0.0f, 0.0f,
                                          200000.0f, "ml/100km",
                                          "DieselConsAvg", false};
static constexpr SignalSpec AdBlueConsAvg{0x65E, 16, 16, 0.4f, 0.0f, 0.0f,
                                          20000.0f, "ml/100km",
                                          "AdBlueConsAvg", false};
static constexpr SignalSpec AdBlueVol{0x65F, 8, 8, 0.4f, 0.0f, 0.0f, 100.0f,
                                      "%", "AdBlueVol", true};

} // namespace MercedesTable
