#pragma once
// ============================================================================
// imu_handler.h — Gerenciamento do sensor BNO085 via SPI (HSPI)
// ============================================================================

#include "config.h"
#include <Adafruit_BNO08x.h>
#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


extern SPIClass SPI_BNO;
extern Adafruit_BNO08x bno;
extern TaskHandle_t hImuTask;

// Inicializa o BNO085: configura HSPI, reset e habilita GAME_ROTATION_VECTOR.
// Chama fatalStop() em caso de falha.
bool initBNO085();

// Task FreeRTOS da IMU
void imuTask(void *pvParameters);
