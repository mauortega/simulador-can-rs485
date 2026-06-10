# Contexto de Desenvolvimento — CAN_teste_03

Documento vivo para preservar o contexto do trabalho. Atualizado conforme avançamos.

Última atualização: 2026-06-10

## Visão geral do projeto

Firmware para **ESP32 DOIT DevKit V1** com dois ambientes PlatformIO:

- `esp32doit-devkit-v1` (Arduino): aplicação principal.
- `esp32doit-devkit-v1-baremetal` (ESP-IDF): entrypoint mínimo que pisca o LED.

Hardware/funcionalidades da aplicação principal:

- **4x MCP2515** (CAN) via VSPI, biblioteca `ACAN2515`.
- **BNO085** (IMU) via HSPI, biblioteca `Adafruit_BNO08x`.
- Duas tasks FreeRTOS: `CAN_TASK` e `IMU_TASK`, ambas alimentando o Watchdog (5s).
- Logging centralizado e protegido por mutex.

## Estrutura

- `src/main.cpp` — setup, criação de tasks, watchdog, heartbeat do LED.
- `src/can_handler.cpp` / `include/can_handler.h` — 4x MCP2515, ISRs, estatísticas, bus-off.
- `src/imu_handler.cpp` / `include/imu_handler.h` — BNO085 e conversão para Euler.
- `src/utils.cpp` / `include/utils.h` — logging thread-safe, timestamp, matemática, fatalStop.
- `include/config.h` — pinos, bitrates, tempos e limites centralizados.
- `src/main_baremetal.c` — blink ESP-IDF.

## Git

- Branch principal: `main` (renomeada de `master`).
- Branch de trabalho: `dev` (estamos aqui).
- Commits:
  - `c662e71` Projeto inicial.
  - `a0b087b` Ajusta mutex de log e recuperação bus-off.

## Trabalho realizado

1. **Mutex de log** (`utils.cpp`): helper `logTryLock()`. Escrita na Serial só ocorre com mutex
   adquirido (ou no boot, antes do mutex existir). Em timeout, descarta o log para evitar
   intercalar mensagens de tasks diferentes.
2. **Bus-off recovery** (`can_handler.cpp`): helper `beginBus(idx)` reutilizado em init e
   recuperação. Na recuperação faz `end()` + `beginBus()` (reinit real do MCP2515) a cada
   `CAN_BUSOFF_RETRY_MS`; após sucesso força releitura do EFLG; mantém `fatalStop()` ao atingir
   `CAN_BUSOFF_MAX_TRIES`.

## Pendências / em aberto

- `src/CMakeLists.txt`: alteração local **não commitada** que adiciona `can_handler.cpp` e
  `utils.cpp` ao `idf_component_register`. Decisão atual: manter fora dos commits por enquanto.
  Revisar se o ambiente ESP-IDF realmente deve compilar esses fontes Arduino.
- **Memória/flash**: build ESP-IDF avisou divergência (esperado 4MB, detectado 2MB). Validar
  tamanho real da flash e as partições (`huge_app.csv`). Adiado a pedido do usuário.
- Validar em bancada: comportamento real de bus-off e carga alta de mensagens CAN.

## Memória persistente

- Regra do Cursor em `.cursor/rules/contexto-dev.mdc` (`alwaysApply: true`) instrui o agente a
  ler e manter este `CONTEXTO_DEV.md` automaticamente a cada tarefa.

## Notas de ambiente

- Shell: PowerShell (use `;` em vez de `&&` para encadear comandos).
- Builds validados: ambos os ambientes compilam com sucesso.
