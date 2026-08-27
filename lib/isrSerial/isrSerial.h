#ifndef ISR_SERIAL_H
#define ISR_SERIAL_H

// Interrupt-driven serial port for classic AVR devices with PORTD, PCINT2,
// and Timer2. It is designed as a replacement for SoftwareSerial when the
// application must keep interrupts enabled during transmission.
//
// Default pins are RX=D4 and TX=D5. Override them at compile time with:
//   -DISR_SERIAL_RX_PIN=<0..7>
//   -DISR_SERIAL_TX_PIN=<0..7>
// Both pins must belong to PORTD (D0 through D7 on an Arduino Nano).
//
// This library owns Timer2 and PCINT2. Do not use it with SoftwareSerial,
// tone(), or PWM on D3/D11. Only one isrSerial instance is available.

#include <Arduino.h>

class IsrSerial : public Stream {
public:
  void begin(unsigned long baud);
  void end();

  int available() override;
  int read() override;
  int peek() override;
  size_t write(uint8_t data) override;
  void flush() override;
  using Print::write;

  // Free bytes in the TX buffer. Use this to avoid a blocking write.
  int availableForWrite() override;

  // Returns and clears the RX overflow flag.
  bool overflow();

  // True while one or more bytes are being transmitted.
  bool txBusy();
};

// Timer2 and PCINT2 interrupt handlers are global, so only one instance exists.
extern IsrSerial isrSerial;

#endif
