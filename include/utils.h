#pragma once
// ============================================================================
// utils.h — Funções utilitárias de logging e matemática
// ============================================================================

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdarg.h>


// ---- Mutex compartilhado de log (inicializado em main) ----
extern SemaphoreHandle_t gSerialMutex;

// ---- Logging thread-safe ----
// Nota: serialLogf() NÃO deve ser chamado de contexto ISR.
void serialLogf(const char *fmt, ...);
void serialLogln(const char *msg);

// Retorna timestamp formatado "mm:ss.mmm" a partir de millis()
// Buffer deve ter ao menos 12 bytes.
void fmtTimestamp(char *buf, uint32_t ms);

// ---- Matemática ----
float clampf(float x, float lo, float hi);

// Converte quaternion (qr, qi, qj, qk) → ângulos de Euler em graus
void quatToEuler(float qr, float qi, float qj, float qk, float *yaw,
                 float *pitch, float *roll);

// ---- Controle de falha fatal ----
void fatalStop(const char *msg);
