#pragma once
// ============================================================================
// config.h — Configurações centralizadas do projeto CAN_teste_03
// ESP32 DevKit + 4x MCP2515 (CAN) + BNO085 (IMU)
// ============================================================================

// ===================== Flags de compilação =====================
#define ENABLE_CAN1_TX_TEST 0 // 1 = habilita TX periódico no CAN1

// ===================== GPIO — BNO085 (HSPI) =====================
#define BNO_CS 4
#define BNO_INT 5 // ideal: pull-up externo ~10kΩ
#define BNO_SCK 14
#define BNO_MISO 12
#define BNO_MOSI 13
#define BNO_RST 15

// ===================== GPIO — MCP2515 (VSPI) =====================
#define CAN_SCK 18
#define CAN_MISO 19
#define CAN_MOSI 23

// Chip Selects
#define CAN1_CS 22
#define CAN2_CS 21
#define CAN3_CS 16
#define CAN4_CS 17

// Interrupções
#define CAN1_INT 32
#define CAN2_INT 25
#define CAN3_INT 26
#define CAN4_INT 27

// ===================== GPIO — Status =====================
#define STATUS_LED_PIN 2 // LED embutido no ESP32 DOIT DevKit V1

// ===================== CAN — Parâmetros =====================
#define CAN_QUARTZ_HZ 8000000UL // Cristal do MCP2515: 8 MHz
#define CAN1_BITRATE 500000UL   // CAN1: 500 kbps
#define CAN2_BITRATE 250000UL   // CAN2-4: 250 kbps
#define CAN3_BITRATE 250000UL
#define CAN4_BITRATE 250000UL

#define CAN1_TX_ID 0x123
#define CAN1_TX_PERIOD_MS 1000

// ===================== CAN — Limites de processamento =====================
#define CAN_MAX_RX_PER_ITER 16  // frames máx por iteração por barramento
#define CAN_LOG_MAX_PER_SEC 20  // limite de logs por barramento por segundo
#define CAN_TASK_BUDGET_US 2000 // orçamento de tempo por iteração (~2 ms)

// ===================== CAN — Bus-off Recovery =====================
#define CAN_BUSOFF_RETRY_MS 500 // intervalo de tentativa de recovery (ms)
#define CAN_BUSOFF_MAX_TRIES 5  // nº máximo de tentativas antes de fatalStop

// ===================== IMU — Parâmetros =====================
#define IMU_REPORT_INTERVAL_US                                                 \
  20000                         // intervalo do relatório BNO085: 20 ms (50 Hz)
#define IMU_PRINT_PERIOD_MS 100 // imprime YPR a cada ≥100 ms (~10 Hz)
#define IMU_SPI_FREQ_HZ 500000  // 500 kHz — robusto para HSPI

// ===================== Tarefas FreeRTOS =====================
#define CAN_TASK_PERIOD_MS 1
#define IMU_TASK_PERIOD_MS 2
#define CAN_TASK_STACK_WORDS 4096
#define IMU_TASK_STACK_WORDS 4096
#define CAN_TASK_PRIORITY 3
#define IMU_TASK_PRIORITY 2
#define STATS_PERIOD_MS 2000

// ===================== Logging =====================
#define LOG_MUTEX_TIMEOUT_MS 50 // timeout para aquisição do mutex de log
