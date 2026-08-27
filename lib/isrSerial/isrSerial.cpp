#include "isrSerial.h"

#ifndef __AVR__
#error "isrSerial requires an AVR target."
#endif

#include <avr/interrupt.h>

#if !defined(PORTD) || !defined(PIND) || !defined(PCICR) || \
    !defined(PCMSK2) || !defined(PCIFR) || !defined(PCIE2) || \
    !defined(PCIF2) || !defined(TCCR2A) || !defined(TCCR2B) || \
    !defined(OCR2A) || !defined(TIMSK2)
#error "isrSerial requires PORTD, PCINT2, and Timer2 support."
#endif

// Pinos em PORTD (D0–D7). No 328P o bit do PCINT2x e o bit do PORTD
// coincidem com o número do pino digital.
#ifndef ISR_SERIAL_RX_PIN
#define ISR_SERIAL_RX_PIN 4 // PD4 / PCINT20
#endif
#ifndef ISR_SERIAL_TX_PIN
#define ISR_SERIAL_TX_PIN 5 // PD5
#endif

#if ISR_SERIAL_RX_PIN < 0 || ISR_SERIAL_RX_PIN > 7
#error "ISR_SERIAL_RX_PIN must be a PORTD pin from D0 to D7."
#endif

#if ISR_SERIAL_TX_PIN < 0 || ISR_SERIAL_TX_PIN > 7
#error "ISR_SERIAL_TX_PIN must be a PORTD pin from D0 to D7."
#endif

#if ISR_SERIAL_RX_PIN == ISR_SERIAL_TX_PIN
#error "ISR_SERIAL_RX_PIN and ISR_SERIAL_TX_PIN must be different."
#endif

#define ISR_RX_MASK _BV(ISR_SERIAL_RX_PIN)
#define ISR_TX_MASK _BV(ISR_SERIAL_TX_PIN)

// Potências de 2: índice avança com máscara, sem divisão na ISR.
#define ISR_TX_BUFF 64
#define ISR_RX_BUFF 32

// A ISR do Timer2 roda a 3x o baud. O start bit chega em fase qualquer do
// timer (jitter de até 1 tick); amostrar 5 ticks depois do edge cai no
// centro do primeiro bit de dados para qualquer fase.
#define ISR_TICKS_PER_BIT 3
#define ISR_RX_START_TICKS 5

static uint8_t txBuf[ISR_TX_BUFF];
static volatile uint8_t txHead = 0;
static volatile uint8_t txTail = 0;
static volatile uint16_t txFrame = 0; // start(0) + 8 dados + stop(1), LSB primeiro
static volatile uint8_t txBits = 0;
static volatile uint8_t txTick = 0;
static volatile bool txActive = false;

static uint8_t rxBuf[ISR_RX_BUFF];
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static volatile uint8_t rxShift = 0;
static volatile uint8_t rxBitCnt = 0;
static volatile uint8_t rxTick = 0;
static volatile bool rxActive = false;
static volatile bool rxOverflowFlag = false;

IsrSerial isrSerial;

static inline void timerIrqOn()
{
  if (!(TIMSK2 & _BV(OCIE2A)))
  {
    TIFR2 = _BV(OCF2A);
    TIMSK2 |= _BV(OCIE2A);
  }
}

// Chamar com interrupções travadas (ou de dentro da ISR).
static inline void txLoadFrame()
{
  uint8_t data = txBuf[txTail];
  txTail = (txTail + 1) & (ISR_TX_BUFF - 1);
  txFrame = ((uint16_t)data << 1) | 0x0200; // bit0 = start (0), bit9 = stop (1)
  txBits = 10;
  txActive = true;
}

ISR(TIMER2_COMPA_vect)
{
  // Amostra o pino logo na entrada para reduzir jitter do RX.
  uint8_t pin = PIND;

  if (rxActive && --rxTick == 0)
  {
    if (rxBitCnt < 8)
    {
      rxShift >>= 1;
      if (pin & ISR_RX_MASK)
      {
        rxShift |= 0x80;
      }
      rxBitCnt++;
      rxTick = ISR_TICKS_PER_BIT;
    }
    else
    {
      // Centro do stop bit: alto = frame válido.
      if (pin & ISR_RX_MASK)
      {
        uint8_t next = (rxHead + 1) & (ISR_RX_BUFF - 1);
        if (next != rxTail)
        {
          rxBuf[rxHead] = rxShift;
          rxHead = next;
        }
        else
        {
          rxOverflowFlag = true;
        }
      }
      rxActive = false;
      // Descarta edges dos bits de dados ocorridos durante o frame.
      // (Limpa o flag do grupo PCINT2 inteiro — nada mais pode usar PORTD.)
      PCIFR = _BV(PCIF2);
      PCMSK2 |= ISR_RX_MASK;
    }
  }

  if (txActive && --txTick == 0)
  {
    txTick = ISR_TICKS_PER_BIT;
    if (txFrame & 1)
    {
      PORTD |= ISR_TX_MASK;
    }
    else
    {
      PORTD &= ~ISR_TX_MASK;
    }
    txFrame >>= 1;
    if (--txBits == 0)
    {
      if (txTail != txHead)
      {
        txLoadFrame(); // stop atual dura 1 bit; próximo start no tick seguinte
      }
      else
      {
        txActive = false;
      }
    }
  }

  if (!txActive && !rxActive)
  {
    TIMSK2 &= ~_BV(OCIE2A);
  }
}

ISR(PCINT2_vect)
{
  // Só interessa o start bit (borda de descida com RX ocioso).
  if (!rxActive && !(PIND & ISR_RX_MASK))
  {
    rxActive = true;
    rxBitCnt = 0;
    rxShift = 0;
    rxTick = ISR_RX_START_TICKS;
    PCMSK2 &= ~ISR_RX_MASK; // PCINT pausado até o fim do frame
    timerIrqOn();
  }
}

void IsrSerial::begin(unsigned long baud)
{
  pinMode(ISR_SERIAL_TX_PIN, OUTPUT);
  digitalWrite(ISR_SERIAL_TX_PIN, HIGH); // linha ociosa
  pinMode(ISR_SERIAL_RX_PIN, INPUT_PULLUP);

  txHead = txTail = 0;
  rxHead = rxTail = 0;
  txActive = false;
  rxActive = false;
  rxOverflowFlag = false;

  // Timer2 CTC, prescaler 8, 3 ticks por bit (divisão arredondada).
  uint32_t ticks = (F_CPU / 8UL + (ISR_TICKS_PER_BIT * baud) / 2) /
                   (ISR_TICKS_PER_BIT * baud);
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);
  OCR2A = (uint8_t)(ticks - 1);
  TIMSK2 &= ~_BV(OCIE2A); // liga só quando TX/RX ativo

  PCMSK2 |= ISR_RX_MASK;
  PCIFR = _BV(PCIF2);
  PCICR |= _BV(PCIE2);
}

void IsrSerial::end()
{
  flush();
  TIMSK2 &= ~_BV(OCIE2A);
  PCMSK2 &= ~ISR_RX_MASK;
  rxActive = false;
  txActive = false;
}

int IsrSerial::available()
{
  return (uint8_t)((rxHead - rxTail) & (ISR_RX_BUFF - 1));
}

int IsrSerial::read()
{
  if (rxHead == rxTail)
  {
    return -1;
  }
  uint8_t data = rxBuf[rxTail];
  rxTail = (rxTail + 1) & (ISR_RX_BUFF - 1);
  return data;
}

int IsrSerial::peek()
{
  if (rxHead == rxTail)
  {
    return -1;
  }
  return rxBuf[rxTail];
}

size_t IsrSerial::write(uint8_t data)
{
  uint8_t next = (txHead + 1) & (ISR_TX_BUFF - 1);

  // Buffer cheio: espera a ISR drenar. Interrupções seguem ativas, o
  // USART0 (RS485) continua recebendo nesse meio tempo.
  while (next == txTail)
  {
  }

  txBuf[txHead] = data;
  txHead = next;

  uint8_t sreg = SREG;
  cli();
  if (!txActive)
  {
    txLoadFrame();
    txTick = 1; // start bit na próxima passada da ISR
    timerIrqOn();
  }
  SREG = sreg;

  return 1;
}

void IsrSerial::flush()
{
  while (txActive)
  {
  }
}

int IsrSerial::availableForWrite()
{
  return (uint8_t)((txTail - txHead - 1) & (ISR_TX_BUFF - 1));
}

bool IsrSerial::overflow()
{
  bool value = rxOverflowFlag;
  rxOverflowFlag = false;
  return value;
}

bool IsrSerial::txBusy()
{
  return txActive;
}
