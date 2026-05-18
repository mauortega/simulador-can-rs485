// ============================================================================
// main.cpp — ESP32 DevKit: 4x MCP2515 (CAN) + BNO085 (IMU) — REFATORADO
// Usa módulos: config.h, utils, can_handler, imu_handler
// ============================================================================

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "can_handler.h"
#include "config.h"
#include "imu_handler.h"
#include "utils.h"

// ===================== Watchdog =====================
// Timeout do watchdog: 5 segundos
#define WDT_TIMEOUT_SEC 5

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);
  delay(300);

  // Mutex de log (definido em utils.cpp como extern)
  gSerialMutex = xSemaphoreCreateMutex();
  if (gSerialMutex == nullptr) {
    // Sem mutex ainda, log direto
    Serial.println("[FATAL] Falha ao criar mutex de log.");
    while (true) {
    }
  }

  serialLogln("\n=== CAN_teste_03: BNO085 + 4x MCP2515 (refatorado) ===");

  // LED de status
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  // CAN
  initAllCAN();

  // IMU
  if (!initBNO085()) {
    fatalStop("IMU indisponível.");
  }

  // ---- Watchdog Timer ----
  esp_task_wdt_config_t wdtCfg = {
      .timeout_ms = WDT_TIMEOUT_SEC * 1000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
      .trigger_panic = true,
  };
  const esp_err_t wdtCfgErr = esp_task_wdt_reconfigure(&wdtCfg);
  if (wdtCfgErr != ESP_OK) {
    serialLogf("[FATAL] Falha ao configurar Task WDT (err=0x%X)\n",
               (unsigned int)wdtCfgErr);
    fatalStop("Task WDT não configurado.");
  }

  // ---- Task CAN ----
  BaseType_t ret = xTaskCreate(canTask, "CAN_TASK", CAN_TASK_STACK_WORDS,
                               nullptr, CAN_TASK_PRIORITY, &hCanTask);
  if (ret != pdPASS)
    fatalStop("Falha ao criar CAN_TASK.");

  // ---- Task IMU ----
  ret = xTaskCreate(imuTask, "IMU_TASK", IMU_TASK_STACK_WORDS, nullptr,
                    IMU_TASK_PRIORITY, &hImuTask);
  if (ret != pdPASS)
    fatalStop("Falha ao criar IMU_TASK.");

  serialLogln(
      "\n>> Inicialização concluída. Tasks FreeRTOS + WDT iniciados.\n");
}

// ===================== Loop =====================
// Apenas pisca o LED de status (heartbeat).
// O feed do watchdog é feito nas tasks CAN/IMU.
static uint32_t lastLedToggleMs = 0;
static bool ledState = false;

void loop() {
  const uint32_t now = millis();
  if (now - lastLedToggleMs >= 500) {
    lastLedToggleMs = now;
    ledState = !ledState;
    digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
  }
  vTaskDelay(pdMS_TO_TICKS(100));
}
