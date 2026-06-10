// ============================================================================
// utils.cpp — Implementação dos utilitários de logging e matemática
// ============================================================================

#include "utils.h"
#include "config.h"
#include <math.h>

SemaphoreHandle_t gSerialMutex = nullptr;

// ---- Logging ----

// Tenta adquirir o mutex de log. Retorna true se a escrita pode prosseguir.
// Antes do mutex existir (boot inicial) a escrita é permitida sem lock.
// Se o mutex existe mas não foi adquirido no timeout, a escrita é abortada
// para não intercalar mensagens de tasks diferentes na Serial.
static bool logTryLock(bool *outHasMutex) {
  if (gSerialMutex == nullptr) {
    *outHasMutex = false;
    return true;
  }
  *outHasMutex = true;
  return xSemaphoreTake(gSerialMutex, pdMS_TO_TICKS(LOG_MUTEX_TIMEOUT_MS)) ==
         pdTRUE;
}

void serialLogf(const char *fmt, ...) {
  // Nunca chamar de ISR
  if (xPortInIsrContext())
    return;

  bool hasMutex = false;
  if (!logTryLock(&hasMutex))
    return;

  va_list args;
  va_start(args, fmt);
  Serial.vprintf(fmt, args);
  va_end(args);

  if (hasMutex)
    xSemaphoreGive(gSerialMutex);
}

void serialLogln(const char *msg) {
  if (xPortInIsrContext())
    return;

  bool hasMutex = false;
  if (!logTryLock(&hasMutex))
    return;

  Serial.println(msg);

  if (hasMutex)
    xSemaphoreGive(gSerialMutex);
}

void fmtTimestamp(char *buf, uint32_t ms) {
  uint32_t totalSec = ms / 1000;
  uint32_t minutes = totalSec / 60;
  uint32_t seconds = totalSec % 60;
  uint32_t millis_r = ms % 1000;
  snprintf(buf, 12, "%02lu:%02lu.%03lu", (unsigned long)minutes,
           (unsigned long)seconds, (unsigned long)millis_r);
}

// ---- Matemática ----

float clampf(float x, float lo, float hi) {
  return (x < lo) ? lo : ((x > hi) ? hi : x);
}

void quatToEuler(float qr, float qi, float qj, float qk, float *yaw,
                 float *pitch, float *roll) {
  const float sqi = qi * qi;
  const float sqj = qj * qj;
  const float sqk = qk * qk;
  const float sqr = qr * qr;
  const float norm = sqi + sqj + sqk + sqr;

  if (norm < 1.0e-9f) {
    *yaw = 0.0f;
    *pitch = 0.0f;
    *roll = 0.0f;
    return;
  }

  const float numYaw = 2.0f * (qi * qj + qk * qr);
  const float denYaw = (sqi - sqj - sqk + sqr);
  const float numPitch = -2.0f * (qi * qk - qj * qr);
  const float denRoll = (-sqi - sqj + sqk + sqr);
  const float numRoll = 2.0f * (qj * qk + qi * qr);

  *yaw = atan2f(numYaw, denYaw) * RAD_TO_DEG;
  *pitch = asinf(clampf(numPitch / norm, -1.0f, 1.0f)) * RAD_TO_DEG;
  *roll = atan2f(numRoll, denRoll) * RAD_TO_DEG;
}

// ---- Falha fatal ----

void fatalStop(const char *msg) {
  serialLogf("[FATAL] %s\n", msg);
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
