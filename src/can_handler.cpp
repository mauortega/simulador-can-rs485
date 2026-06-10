// ============================================================================
// can_handler.cpp — Implementação do gerenciamento de 4x MCP2515
// ============================================================================

#include "can_handler.h"
#include "utils.h"
#include <esp_task_wdt.h>
#include <freertos/task.h>

// ---- Instâncias globais ----
SPIClass SPI_CAN(VSPI);

// Tabela de configuração de cada barramento
struct CanBusConfig {
  uint8_t cs;
  uint8_t intPin;
  uint32_t bitrate;
  ACAN2515Settings::RequestedMode mode;
  const char *name;
};

static const CanBusConfig kBusConfig[CAN_BUS_COUNT] = {
    {CAN1_CS, CAN1_INT, CAN1_BITRATE, ACAN2515Settings::NormalMode, "CAN1"},
    {CAN2_CS, CAN2_INT, CAN2_BITRATE, ACAN2515Settings::ListenOnlyMode, "CAN2"},
    {CAN3_CS, CAN3_INT, CAN3_BITRATE, ACAN2515Settings::ListenOnlyMode, "CAN3"},
    {CAN4_CS, CAN4_INT, CAN4_BITRATE, ACAN2515Settings::ListenOnlyMode, "CAN4"},
};

static void (*const kISRs[CAN_BUS_COUNT])() = {can0ISR, can1ISR, can2ISR,
                                               can3ISR};

static constexpr uint8_t MCP2515_EFLG_TXBO = 0x20;
static constexpr uint32_t CAN_ERROR_POLL_MS = 100;

// Controladores e estatísticas
ACAN2515 canBus[CAN_BUS_COUNT] = {
    ACAN2515(CAN1_CS, SPI_CAN, CAN1_INT),
    ACAN2515(CAN2_CS, SPI_CAN, CAN2_INT),
    ACAN2515(CAN3_CS, SPI_CAN, CAN3_INT),
    ACAN2515(CAN4_CS, SPI_CAN, CAN4_INT),
};
CanStats canStats[CAN_BUS_COUNT];
TaskHandle_t hCanTask = nullptr;

// ---- ISRs ----
void IRAM_ATTR can0ISR() { canBus[0].isr(); }
void IRAM_ATTR can1ISR() { canBus[1].isr(); }
void IRAM_ATTR can2ISR() { canBus[2].isr(); }
void IRAM_ATTR can3ISR() { canBus[3].isr(); }

// ---- Helper: nome do modo ----
static const char *modeName(ACAN2515Settings::RequestedMode m) {
  switch (m) {
  case ACAN2515Settings::NormalMode:
    return "NORMAL";
  case ACAN2515Settings::ListenOnlyMode:
    return "LISTEN-ONLY";
  case ACAN2515Settings::LoopBackMode:
    return "LOOPBACK";
  default:
    return "DESCONHECIDO";
  }
}

// ---- Helper: (re)inicializa um barramento com sua configuração fixa ----
// Retorna 0 em sucesso ou o código de erro de ACAN2515::begin().
static uint16_t beginBus(uint8_t idx) {
  ACAN2515Settings settings(CAN_QUARTZ_HZ, kBusConfig[idx].bitrate);
  settings.mRequestedMode = kBusConfig[idx].mode;
  return canBus[idx].begin(settings, kISRs[idx]);
}

// ---- Helper: imprime frame CAN com timestamp ----
static void printCANMsg(const char *tag, const CANMessage &msg) {
  char ts[12];
  fmtTimestamp(ts, millis());
  serialLogf("[%s][%s] ID=0x%03X  DLC=%u  DATA:", tag, ts, msg.id, msg.len);
  for (uint8_t i = 0; i < msg.len; i++)
    serialLogf(" %02X", msg.data[i]);
  serialLogf("\n");
}

// ---- Helper: dreno de RX com limite e bus-off recovery ----
static void drainCAN(uint8_t idx, uint32_t nowMs) {
  const char *tag = kBusConfig[idx].name;
  ACAN2515 &can = canBus[idx];
  CanStats &st = canStats[idx];

  // ---- Poll de flags de erro ----
  if (nowMs - st.lastErrorPollMs >= CAN_ERROR_POLL_MS) {
    st.lastErrorPollMs = nowMs;
    const uint8_t eflg = can.errorFlagRegister();
    const bool txBusOff = (eflg & MCP2515_EFLG_TXBO) != 0;

    if (txBusOff && !st.busOff) {
      st.busOff = true;
      st.busOffTries = 0;
      st.lastBusOffAttemptMs = nowMs;
      serialLogf("[%s] Detectado BUS-OFF (EFLG=0x%02X).\n", tag, eflg);
    } else if (!txBusOff && st.busOff) {
      st.busOff = false;
      st.busOffTries = 0;
      st.busOffRecoveries++;
      serialLogf("[%s] BUS-OFF encerrado (EFLG=0x%02X).\n", tag, eflg);
    }
  }

  // ---- Bus-off recovery ----
  // Reinicializa o controlador (end + begin) para limpar TXBO e contadores
  // de erro. Em modo normal o MCP2515 também recupera sozinho após 128x11
  // bits recessivos, mas o reinit garante estado limpo e determinístico.
  if (st.busOff) {
    if (nowMs - st.lastBusOffAttemptMs >= CAN_BUSOFF_RETRY_MS) {
      st.lastBusOffAttemptMs = nowMs;
      st.busOffTries++;
      if (st.busOffTries > CAN_BUSOFF_MAX_TRIES) {
        serialLogf(
            "[%s] Bus-off: máximo de tentativas atingido. Sistema parado.\n",
            tag);
        fatalStop("Bus-off irrecuperável.");
      }

      serialLogf("[%s] Bus-off recovery — tentativa %u/%u (reinit MCP2515)\n",
                 tag, st.busOffTries, CAN_BUSOFF_MAX_TRIES);

      can.end();
      const uint16_t err = beginBus(idx);
      if (err != 0) {
        serialLogf("[%s] Reinit falhou (ERR=0x%X) — nova tentativa em %lu ms\n",
                   tag, err, (unsigned long)CAN_BUSOFF_RETRY_MS);
      } else {
        // Força releitura do EFLG na próxima janela de poll para confirmar
        // a saída do bus-off e contabilizar o recovery.
        st.lastErrorPollMs = nowMs - CAN_ERROR_POLL_MS;
      }
    }
    return; // Não processar RX enquanto em bus-off
  }

  // ---- Janela de rate-limit de logs ----
  if (nowMs - st.logWindowStartMs >= 1000UL) {
    st.logWindowStartMs = nowMs;
    st.logsInWindow = 0;
  }

  // ---- Recepção ----
  CANMessage m;
  uint16_t processed = 0;
  while (processed < CAN_MAX_RX_PER_ITER && can.receive(m)) {
    st.rxFramesTotal++;
    st.rxFramesSinceStats++;
    processed++;

    if (st.logsInWindow < CAN_LOG_MAX_PER_SEC) {
      st.logsInWindow++;
      printCANMsg(tag, m);
    } else {
      st.suppressedLogs++;
    }
  }

  // ---- Estatísticas periódicas ----
  if (nowMs - st.lastStatsMs >= STATS_PERIOD_MS) {
    st.lastStatsMs = nowMs;
    char ts[12];
    fmtTimestamp(ts, nowMs);
    serialLogf(
        "[%s][%s] RX_total=%lu | RX_%lus=%lu | TX_total=%lu | TX_erros=%lu"
        " | logs_suprimidos=%lu | busoff_recoveries=%lu\n",
        tag, ts, (unsigned long)st.rxFramesTotal,
        (unsigned long)(STATS_PERIOD_MS / 1000UL),
        (unsigned long)st.rxFramesSinceStats, (unsigned long)st.txFramesTotal,
        (unsigned long)st.txErrCount, (unsigned long)st.suppressedLogs,
        (unsigned long)st.busOffRecoveries);
    st.rxFramesSinceStats = 0;
    st.suppressedLogs = 0;
  }
}

// ---- Inicialização de todos os barramentos ----
void initAllCAN() {
  SPI_CAN.begin(CAN_SCK, CAN_MISO, CAN_MOSI, -1);

  // Configura CS e INT pins
  const uint8_t csPins[] = {CAN1_CS, CAN2_CS, CAN3_CS, CAN4_CS};
  const uint8_t intPins[] = {CAN1_INT, CAN2_INT, CAN3_INT, CAN4_INT};
  for (uint8_t i = 0; i < CAN_BUS_COUNT; i++) {
    pinMode(csPins[i], OUTPUT);
    digitalWrite(csPins[i], HIGH);
    pinMode(intPins[i], INPUT);
    if (digitalPinToInterrupt(intPins[i]) == NOT_AN_INTERRUPT) {
      serialLogf("[%s] ERRO: GPIO %d não suporta interrupção.\n",
                 kBusConfig[i].name, intPins[i]);
      fatalStop("Pino de interrupção CAN inválido.");
    }
  }

  // Inicializa cada barramento
  bool allOk = true;
  for (uint8_t i = 0; i < CAN_BUS_COUNT; i++) {
    const char *name = kBusConfig[i].name;

    serialLogf("\n[%s] ===== Inicializando MCP2515 =====\n", name);
    serialLogf("[%s] Clock : %lu Hz | Bitrate : %lu kbps | Modo: %s\n", name,
               CAN_QUARTZ_HZ, kBusConfig[i].bitrate / 1000UL,
               modeName(kBusConfig[i].mode));

    uint16_t err = beginBus(i);
    if (err == 0) {
      serialLogf("[%s] OK\n", name);
    } else {
      serialLogf("[%s] FALHOU (ERR=0x%X)\n", name, err);
      allOk = false;
    }
  }

  if (!allOk) {
    fatalStop("Inicialização CAN incompleta.");
  }
}

// ---- Task FreeRTOS ----
void canTask(void *pvParameters) {
  (void)pvParameters;
  const esp_err_t addErr = esp_task_wdt_add(nullptr);
  if (addErr != ESP_OK) {
    serialLogf("[CAN_TASK] Falha ao registrar no WDT (err=0x%X)\n",
               (unsigned int)addErr);
    fatalStop("CAN_TASK sem registro no WDT.");
  }

  for (;;) {
    const uint32_t nowMs = millis();
    const uint32_t loopStartUs = micros();

    for (uint8_t i = 0; i < CAN_BUS_COUNT; i++) {
      drainCAN(i, nowMs);
    }

#if ENABLE_CAN1_TX_TEST
    static uint32_t lastTxMs = 0;
    if (nowMs - lastTxMs >= CAN1_TX_PERIOD_MS) {
      lastTxMs = nowMs;
      CANMessage tx;
      tx.id = CAN1_TX_ID;
      tx.len = 8;
      for (uint8_t i = 0; i < 8; i++)
        tx.data[i] = i;
      if (canBus[0].tryToSend(tx)) {
        canStats[0].txFramesTotal++;
      } else {
        canStats[0].txErrCount++;
        serialLogf("[CAN1] TX ID=0x%03X FALHOU (fila cheia?)\n", tx.id);
      }
    }
#endif

    // Yield se ultrapassou orçamento de tempo
    if ((micros() - loopStartUs) >= CAN_TASK_BUDGET_US) {
      taskYIELD();
    }

    const esp_err_t resetErr = esp_task_wdt_reset();
    if (resetErr != ESP_OK) {
      serialLogf("[CAN_TASK] Falha no feed do WDT (err=0x%X)\n",
                 (unsigned int)resetErr);
      fatalStop("CAN_TASK falhou ao alimentar WDT.");
    }

    vTaskDelay(pdMS_TO_TICKS(CAN_TASK_PERIOD_MS));
  }
}
