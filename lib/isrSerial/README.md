# isrSerial

`isrSerial` is an interrupt-driven software serial port for classic AVR
devices. It keeps global interrupts enabled while transmitting, unlike the AVR
implementation of `SoftwareSerial`.

## Supported targets

The library is intended for AVR devices that provide `PORTD`, `PCINT2`, and
`Timer2`. It is validated on the ATmega328P used by Arduino Nano boards.

Only one `isrSerial` instance exists because the Timer2 and PCINT2 interrupt
handlers are global.

## Limitations

- Do not use with `SoftwareSerial`; both define the PCINT2 interrupt handler.
- Do not use `tone()` or PWM on D3/D11; Timer2 is reserved by this library.
- RX and TX must be in PORTD, D0 through D7 on Arduino Nano.
- Avoid D0/D1 when the USB serial interface is in use.
- 9600 baud is the recommended speed. 19200 baud is supported in this project
  but should be tested with the target wiring and CPU clock.

## Installation

Copy this folder to the project's `lib/isrSerial` directory. PlatformIO will
discover it automatically through `library.json`.

## Pin configuration

The defaults are RX=D4 and TX=D5. Override the pins in `platformio.ini`:

```ini
build_flags =
  -DISR_SERIAL_RX_PIN=5
  -DISR_SERIAL_TX_PIN=4
```

## Example

```cpp
#include <isrSerial.h>

void setup() {
  isrSerial.begin(9600);
}

void loop() {
  isrSerial.println(F("hello"));
  while (isrSerial.available()) {
    int value = isrSerial.read();
    // Process value.
  }
}
```

`write()` can wait when the 64-byte TX buffer is full. Check
`availableForWrite()` before sending long data. Use `txBusy()` to determine
whether bytes are still leaving the TX pin, and `overflow()` to read and clear
the RX overflow indicator.
