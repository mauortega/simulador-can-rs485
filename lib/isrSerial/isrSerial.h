#ifndef ISR_SERIAL_H
#define ISR_SERIAL_H

// Serial por interrupção em D4 (RX) / D5 (TX) — substituto do SoftwareSerial.
//
// Motivo: o SoftwareSerial transmite com interrupções desligadas (~1 ms por
// byte a 9600). Enquanto isso o USART0 (RS485 do validador) não recebe, e um
// frame único como o STD 0x30 de seleção de linha pode se perder.
//
// Este driver usa o Timer2 (CTC, 3 ticks por bit) para TX e PCINT2 para
// detectar o start bit no RX. As ISRs duram poucos µs; o RX do RS485
// continua sendo atendido durante a transmissão para a Raspberry.
//
// Restrições:
// - NÃO pode ser linkado junto com o SoftwareSerial: os dois definem
//   ISR(PCINT2_vect). Ao adotar esta lib, remover o SoftwareSerial do host.
// - Pinos fixos em PORTD (D0–D7), padrão D4/D5 (mapa NX7000). Override em
//   tempo de compilação: -DISR_SERIAL_RX_PIN / -DISR_SERIAL_TX_PIN.
// - Ocupa o Timer2 (sem tone() / PWM em D3 e D11 — este projeto não usa).
// - Baud testado: 9600 (alvo da Raspberry). 19200 é o limite prático.
// - write() bloqueia se o buffer de TX encher, mas com interrupções ATIVAS:
//   o ring do USART0 segue enchendo. Use availableForWrite() para fatiar
//   prints longos sem bloquear.

#include <Arduino.h>

class IsrSerial : public Stream
{
public:
  void begin(unsigned long baud);
  void end();

  virtual int available();
  virtual int read();
  virtual int peek();
  virtual size_t write(uint8_t data);
  virtual void flush();
  using Print::write;

  // Espaço livre no buffer de TX (para enviar sem bloquear).
  virtual int availableForWrite();

  // true se algum byte de RX foi descartado desde a última chamada.
  bool overflow();

  // true enquanto há bytes saindo pelo TX.
  bool txBusy();
};

// Instância única: as ISRs (Timer2 / PCINT2) são globais.
extern IsrSerial isrSerial;

#endif
