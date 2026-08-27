# Contexto de Desenvolvimento

Ultima atualizacao: 2026-08-27

## Repositorio e branch

- Repositorio: `https://github.com/mauortega/simulador-can-rs485.git`
- Branch desta versao: `simulador-atmega`
- O firmware ESP32 original permanece preservado nas outras branches.
- Projeto PlatformIO para Arduino Nano/ATmega328P, framework Arduino.
- Ambiente: `nanoatmega328`.
- Ultimo commit funcional antes desta atualizacao: `50d68c5`.

## Hardware atual

- Microcontrolador: ATmega328P.
- Uma porta CAN com MCP2515 e cristal de 8 MHz.
- MCP2515 em modo normal, com transmissao one-shot.
- Transceiver RS485 com `DE` e `/RE` unidos e controlados pelo firmware.

## Pinagem

| Funcao | Pino |
|---|---:|
| MCP2515 INT | D3 |
| RS485 TX/DI | D4 |
| RS485 RX/RO | D5 |
| RS485 DE + /RE | D6 |
| MCP2515 CS | D8 |
| SPI MOSI | D11 |
| SPI MISO | D12 |
| SPI SCK | D13 |
| LED atividade RS485 | A0 |
| LED CAN 250 | A1 |
| LED CAN 500 | A2 |

Os LEDs sao ativos em nivel alto. Cada LED externo deve usar resistor de cerca
de 330 ohms e ter o catodo ligado ao GND.

## CAN

- Velocidade inicial: 250 kbit/s.
- `can250`: seleciona 250 kbit/s e simulacao J1939/VW; acende A1.
- `can500`: seleciona 500 kbit/s e simulacao Mercedes; acende A2.
- Os comandos sao recebidos pela serial USB a 115200 baud.
- Os comandos aceitam maiusculas e minusculas e devem ser seguidos de Enter.
- Somente um LED de velocidade permanece aceso.
- O equipamento conectado pode estar em listen-only; o simulador transmite em
  modo normal.

## RS485

- SoftwareSerial a 19200 baud.
- Pacote fixo de 29 bytes transmitido a cada 2 segundos.
- Antes do envio, D6 vai para HIGH para habilitar o transmissor.
- Ao terminar o envio, D6 volta para LOW para habilitar recepcao.
- A0 acende por aproximadamente 120 ms apos cada pacote transmitido.
- O LED A0 indica atividade de transmissao, nao confirmacao do receptor.

## Compilacao e memoria

Comando validado:

```powershell
pio run
```

Uso atual:

- Flash: 19.738 de 30.720 bytes (64,3%).
- RAM: 1.573 de 2.048 bytes (76,8%).

A RAM esta relativamente alta. Evitar buffers grandes e manter textos de log
em flash usando `F()`.

## Gravacao

- Porta usada nos testes mais recentes: `COM11`.
- Assinatura esperada: `0x1e950f` (ATmega328P).
- Uma placa testada usa bootloader antigo a 57600 baud.
- A placa usada mais recentemente usa bootloader a 115200 baud.
- O `platformio.ini` ainda esta configurado para COM11 e 57600 baud.
- Para a placa de 115200, alterar `upload_speed` ou chamar o `avrdude`
  diretamente com `-b115200`.

## Ambiente Windows

- O CH340 da COM11 usa o driver legado WCH 3.5.2019.1 (`oem82.inf`).
- Esse driver resolveu o erro `can't set com-state` encontrado com drivers WCH
  mais novos.
- O indicador vermelho `No Solution` no VS Code vem da extensao C# Dev Kit
  (`ms-dotnettools.csdevkit`) e nao afeta o PlatformIO. Recarregar a janela apos
  desabilitar a extensao remove o indicador.

## Estado validado

- Compilacao concluida com sucesso.
- Firmware com controle RS485 em D6 gravado com sucesso na COM11 a 115200 baud.
- Branch publicada no GitHub.
- README atualizado com pinagem, LEDs, CAN, RS485 e gravacao.
