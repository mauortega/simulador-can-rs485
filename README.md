# Simulador CAN + RS485 para ATmega328P

Versao do simulador portada para Arduino Nano/ATmega328P. O firmware utiliza uma
porta CAN com MCP2515 e permite alternar, pela serial, entre os perfis de 250 e
500 kbit/s.

Esta versao esta na branch `simulador-atmega`. O software original para ESP32
permanece preservado nas demais branches.

## Hardware

- Arduino Nano ou placa compativel com ATmega328P
- MCP2515 com cristal de 8 MHz
- Transceiver CAN
- Transceiver RS485 com controle automatico de direcao
- Dois LEDs e dois resistores de aproximadamente 330 ohms

## Pinagem

| Funcao | Pino ATmega/Nano | Observacao |
|---|---:|---|
| MCP2515 INT | D3 | Interrupcao CAN |
| RS485 TX | D4 | Transmissao somente |
| RS485 RX | D5 | Reservado pelo `SoftwareSerial` |
| MCP2515 CS | D8 | Chip select da unica porta CAN |
| SPI MOSI | D11 | SPI de hardware |
| SPI MISO | D12 | SPI de hardware |
| SPI SCK | D13 | SPI de hardware |
| LED de estado | A0 | Pisca durante a execucao |
| LED CAN 250 | A1 | Aceso no modo 250 kbit/s |
| LED CAN 500 | A2 | Aceso no modo 500 kbit/s |

Ligue o anodo de cada LED indicador ao pino correspondente por meio de um
resistor de aproximadamente 330 ohms. Ligue o catodo ao GND. Os LEDs sao ativos
em nivel alto e apenas um LED de velocidade permanece aceso por vez.

O barramento CAN deve ter terminacao adequada de 120 ohms nas duas extremidades.
CAN-H, CAN-L e GND devem ser compartilhados com o equipamento conectado.

## Modos CAN

Ao ligar, o simulador inicia em 250 kbit/s.

| Comando serial | Velocidade | Perfil transmitido | LED |
|---|---:|---|---|
| `can250` | 250 kbit/s | J1939/VW | A1 |
| `can500` | 500 kbit/s | Mercedes | A2 |

Envie o comando seguido de Enter no monitor serial configurado para 115200
baud. Maiusculas e minusculas sao aceitas, portanto `CAN250`, `Can500` e
`can500` sao equivalentes.

Quando o comando e aceito, o MCP2515 e reinicializado na nova velocidade e o
LED correspondente e atualizado. O controlador trabalha em modo normal e
transmite quadros CAN em modo one-shot; ele nao opera em listen-only.

## RS485

O RS485 transmite em D4 a 19200 baud, a cada 2 segundos. O firmware pressupoe
um transceiver com controle automatico de direcao. D5 fica reservado como RX,
mas pode permanecer desconectado quando apenas a transmissao for usada.

## Compilacao

Requer PlatformIO. Na raiz do projeto, execute:

```powershell
pio run
```

O ambiente padrao e `nanoatmega328`, definido em `platformio.ini`.

## Gravacao

A porta configurada atualmente e `COM11`. Confirme a porta no Gerenciador de
Dispositivos antes de gravar.

Para placas com bootloader antigo do Arduino Nano, use 57600 baud, que e a
configuracao padrao deste projeto:

```powershell
pio run --target upload
```

Algumas placas usam bootloader novo a 115200 baud. Nesse caso, altere
temporariamente `upload_speed` em `platformio.ini` para `115200` e execute o
mesmo comando de upload.

Os bootloaders testados usam o protocolo `arduino` e o dispositivo
`ATmega328P`. Uma gravacao correta apresenta a assinatura `0x1e950f`.

## Uso de memoria

Na compilacao validada desta versao:

- Flash: 19.674 de 30.720 bytes (64,0%)
- RAM: 1.573 de 2.048 bytes (76,8%)

Devido ao limite de RAM do ATmega328P, novas funcionalidades devem evitar
buffers grandes e textos armazenados em SRAM.
