#pragma once
// ============================================================================
// can_handler.h — Gerenciamento de 4x MCP2515 via ACAN2515
// Inclui: arrays de controladores, ISRs, estatísticas, bus-off recovery
// ============================================================================

#include "config.h"
#include <ACAN2515.h>
#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


// ---- Número de barramentos CAN ----
#define CAN_BUS_COUNT 4

// ---- Estatísticas por barramento ----
struct CanStats {
  uint32_t rxFramesTotal = 0;
  uint32_t rxFramesSinceStats = 0;
  uint32_t txFramesTotal = 0;
  uint32_t txErrCount = 0;
  uint32_t suppressedLogs = 0;
  uint32_t busOffRecoveries = 0;
  uint32_t lastStatsMs = 0;
  uint32_t lastErrorPollMs = 0;
  uint32_t logWindowStartMs = 0;
  uint16_t logsInWindow = 0;
  uint8_t busOffTries = 0;
  bool busOff = false;
  uint32_t lastBusOffAttemptMs = 0;
};

// ---- Instâncias globais ----
extern SPIClass SPI_CAN;
extern ACAN2515 canBus[CAN_BUS_COUNT];
extern CanStats canStats[CAN_BUS_COUNT];
extern TaskHandle_t hCanTask;

// ---- ISRs ----
// O atributo IRAM é aplicado apenas na definição (can_handler.cpp)
// para evitar conflito de seção entre declaração e definição.
void can0ISR();
void can1ISR();
void can2ISR();
void can3ISR();

// ---- Inicialização ----
// Inicializa o SPI_CAN e os 4 barramentos.
// Chama fatalStop() se algum falhar.
void initAllCAN();

// ---- Task FreeRTOS ----
void canTask(void *pvParameters);
