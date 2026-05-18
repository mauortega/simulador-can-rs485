// ============================================================================
// imu_handler.cpp — Implementação do handler BNO085
// ============================================================================

#include "imu_handler.h"
#include "utils.h"
#include <esp_task_wdt.h>

SPIClass SPI_BNO(HSPI);
Adafruit_BNO08x bno(BNO_RST);
TaskHandle_t hImuTask = nullptr;

bool initBNO085() {
  serialLogln("[IMU ] Inicializando BNO085 (HSPI)...");
  pinMode(BNO_CS, OUTPUT);
  digitalWrite(BNO_CS, HIGH);
  pinMode(BNO_INT, INPUT_PULLUP);
  pinMode(BNO_RST, OUTPUT);
  digitalWrite(BNO_RST, HIGH);

  SPI_BNO.begin(BNO_SCK, BNO_MISO, BNO_MOSI, BNO_CS);
  SPI_BNO.setHwCs(false);
  SPI_BNO.setDataMode(SPI_MODE3);
  SPI_BNO.setBitOrder(MSBFIRST);
  SPI_BNO.setFrequency(IMU_SPI_FREQ_HZ);

  digitalWrite(BNO_RST, LOW);
  delay(10);
  digitalWrite(BNO_RST, HIGH);
  delay(300);

  if (!bno.begin_SPI(BNO_CS, BNO_INT, &SPI_BNO)) {
    serialLogln("[IMU ] BNO085 não encontrado via SPI.");
    return false;
  }
  if (!bno.enableReport(SH2_GAME_ROTATION_VECTOR, IMU_REPORT_INTERVAL_US)) {
    serialLogln("[IMU ] Falha ao habilitar GAME_ROTATION_VECTOR.");
    return false;
  }
  serialLogln("[IMU ] OK — leituras iniciadas.");
  return true;
}

void imuTask(void *pvParameters) {
  (void)pvParameters;
  uint32_t lastPrintMs = 0;
  const esp_err_t addErr = esp_task_wdt_add(nullptr);
  if (addErr != ESP_OK) {
    serialLogf("[IMU_TASK] Falha ao registrar no WDT (err=0x%X)\n",
               (unsigned int)addErr);
    fatalStop("IMU_TASK sem registro no WDT.");
  }

  // imuEvent é local à task — sem variável global compartilhada
  sh2_SensorValue_t imuEvent;

  for (;;) {
    const uint32_t nowMs = millis();

    if (bno.getSensorEvent(&imuEvent) &&
        imuEvent.sensorId == SH2_GAME_ROTATION_VECTOR) {

      if (nowMs - lastPrintMs >= IMU_PRINT_PERIOD_MS) {
        lastPrintMs = nowMs;

        float yaw, pitch, roll;
        quatToEuler(imuEvent.un.gameRotationVector.real,
                    imuEvent.un.gameRotationVector.i,
                    imuEvent.un.gameRotationVector.j,
                    imuEvent.un.gameRotationVector.k, &yaw, &pitch, &roll);

        char ts[12];
        fmtTimestamp(ts, nowMs);
        serialLogf("[IMU ][%s] Yaw:%7.2f° | Pitch:%7.2f° | Roll:%7.2f°\n", ts,
                   yaw, pitch, roll);
      }
    }

    const esp_err_t resetErr = esp_task_wdt_reset();
    if (resetErr != ESP_OK) {
      serialLogf("[IMU_TASK] Falha no feed do WDT (err=0x%X)\n",
                 (unsigned int)resetErr);
      fatalStop("IMU_TASK falhou ao alimentar WDT.");
    }

    vTaskDelay(pdMS_TO_TICKS(IMU_TASK_PERIOD_MS));
  }
}
