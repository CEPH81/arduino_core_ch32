# CH57x / CH58x / CH59x en Arduino (fork de CEPH81 — MikrocontrollerCephs)

Este fork de [openwch/arduino_core_ch32](https://github.com/openwch/arduino_core_ch32) añade los
microcontroladores **BLE RISC-V de WCH**, con un núcleo propio (`cores/ch5xx`) sobre los SDK
oficiales de WCH. Los CH32 de openwch siguen igual.

> **Estado: en desarrollo.** Compila para todas las placas; falta probarlo en hardware.

| Placa (menú) | Serie | Chip | Flash | RAM | SDK |
|---|---|---|---|---|---|
| CH58x → WeAct Studio CH582F | CH58x | CH582F | 448 KB | 32 KB | openwch/ch583 |
| CH58x → CH583M / CH582M | CH58x | CH583M | 448 KB | 32 KB | openwch/ch583 |
| CH59x → WeAct Studio CH592F | CH59x | CH592F | 448 KB | 26 KB | openwch/ch592 |
| CH57x → WeAct Studio CH573F | CH573 | CH573F | 448 KB | 18 KB | CH573EVT (WCH, copia de WeAct Studio) |
| CH57x → CH571F | CH573 | CH571F | 192 KB | 18 KB | ídem |

CH570/CH572 (openwch/ch570): en preparación (tienen un solo puerto GPIO y una sola UART).

## Qué funciona

- `pinMode` (INPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT), `digitalWrite/Read/Toggle`.
  Patas: `PA0`…`PA15`, `PB0`…`PB23`.
- `millis`, `micros`, `delay`, `delayMicroseconds` (SysTick de 64 bits).
- `Serial` = UART1 (TX **PA9**, RX **PA8**), `Serial0` (PB7/PB4), `Serial2` (PA7/PA6),
  `Serial3` (PA5/PA4); `printf`, `serialEvent()`.
- `analogRead(A0…A13)` (ADC de 12 bits; 10 bits por defecto, `analogReadResolution(12)`),
  `analogReadMilliVolts`, `analogReadVccMilliVolts`, `temperatureRead`.
- `analogWrite` en PA12, PA13, PB0, PB4, PB6, PB7, PB14, PB23 (PWM4–PWM11, 8 bits),
  `analogWriteFrequency`.
- `attachInterrupt` en cualquier pata (RISING, FALLING, CHANGE, LOW, HIGH).
- Reloj: menú **Clock Select** (60 MHz por defecto; 80 MHz en CH58x).

En las placas de WeAct el LED está en **PA8** (se enciende en bajo: `LED_ON`/`LED_OFF`) y el botón
BOOT en **PB22** (`KEY_BUILTIN`). PA8 es también el RX de `Serial`.

## Carga

- **WCH-ISP (USB)**: mantenga pulsado BOOT, conecte el USB y suba. Usa
  [wchisp](https://github.com/ch32-rs/wchisp) (incluido en `tools/wchisp`). En Windows hace falta
  el driver WinUSB para el dispositivo de arranque de WCH (con Zadig) y el runtime de Visual C++.
- **WCH-Link / WCH-LinkE** (modo RISC-V, openocd de WCH): por probar con estos chips.

## Licencias

SDK de WCH: Apache 2.0 (openwch). CH573EVT: de WCH, copia publicada por WeAct Studio (MIT).
wchisp: MIT/Apache 2.0. Núcleo `cores/ch5xx`: LGPL 2.1, como Arduino.
