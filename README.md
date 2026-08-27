# Simulador CAN + RS485 para Arduino Nano

Porta do firmware ESP32 para Arduino Nano com ATmega328P e MCP2515 de 8 MHz.

## Ligacoes

| Funcao | Pino Nano |
|---|---:|
| SPI SCK | D13 |
| SPI MISO | D12 |
| SPI MOSI | D11 |
| MCP2515 CS (250 kbit/s) | D8 |
| MCP2515 INT | D3 |
| RS485 TX | D4 |
| LED de estado | A0 |

O RS485 continua TX-only e pressupoe um transceiver com direcao automatica. D5
fica reservado como RX do `SoftwareSerial`, mas pode permanecer desconectado.

Compilacao: `pio run`. Gravacao: `pio run -t upload`.

## Selecao da velocidade CAN

No monitor serial a 115200 baud, envie `can250` ou `can500` seguido de Enter. Os
comandos aceitam letras maiusculas e minusculas, por exemplo `CAN250`, `Can500`
ou `can250`. O MCP2515 sera reinicializado imediatamente na velocidade
escolhida. Ao ligar, a velocidade inicial e 250 kbit/s.
