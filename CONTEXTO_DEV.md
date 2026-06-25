# Contexto de Desenvolvimento

Última atualização: 2026-06-25

## Estado Atual

- Projeto PlatformIO para ESP32 (`esp32doit-devkit-v1`) usando framework Arduino.
- O firmware compila com `pio run`.
- Branch de trabalho atual: `dev`, publicada em `origin/dev`.
- Último commit publicado: `862f4c8 Ajusta envio RS485 e tensão do alternador`.

## Histórico Recente

- Usuário confirmou em equipamento real que tudo está funcionando após as correções de RS485 e alternador.
- Commit `862f4c8` criado e enviado para `origin/dev`.
- Ajustado o frame J1939/VW `0x18FEF700` em `src/J1939Simulator.cpp` para enviar `altVoltage` também nos bytes 4/5, que o parser VW do equipamento lê como alternador.
- Removido `Serial2.flush()` de `src/Rs485Application.cpp`; o envio RS485 agora checa `availableForWrite()` antes de enfileirar o pacote para evitar bloqueio de ~15 ms no loop e reduzir jitter CAN.
- Criada a branch local `dev` a partir de `simulador-can-rs485`; mudanças não commitadas foram preservadas no working tree.
- Corrigida a dependência explícita de `src/main.cpp` adicionando `#include <Arduino.h>`.
- Gerado `compile_commands.json` via `pio run -t compiledb` para o analisador C/C++ resolver includes do PlatformIO.
- Adicionada configuração `.clangd` apontando para o banco de compilação da raiz.
- `compile_commands.json` foi adicionado ao `.gitignore` por ser artefato local gerado.

## Pendências

- Se o Cursor continuar mostrando diagnóstico antigo de `Arduino.h`, recarregar a janela ou reiniciar o servidor `clangd` para ele reler `compile_commands.json`.
- `CONTEXTO_DEV.md` foi atualizado após o commit/push e ainda precisa ser commitado se o usuário quiser registrar este resumo no Git.
