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

## Instalación (Gestor de placas, Arduino IDE 2.x)

1. **Archivo → Preferencias → URLs adicionales del gestor de placas**:
   ```
   https://raw.githubusercontent.com/CEPH81/arduino_core_ch32/ch5xx/package_ch32v_ceph81_index.json
   ```
2. **Herramientas → Placa → Gestor de placas**, busque **CH32V + CH5xx** e instale. Se
   descargan el compilador RISC-V y openocd de WCH (los mismos del paquete oficial de openwch).
3. Las placas aparecen en **CH32V + CH5xx (fork MikrocontrollerCephs)**: los CH32 de openwch y
   los CH5xx. Puede convivir con el paquete oficial de WCH.

## Qué funciona

- `pinMode` (INPUT, INPUT_PULLUP, INPUT_PULLDOWN, OUTPUT), `digitalWrite/Read/Toggle`.
  Patas: `PA0`…`PA15`, `PB0`…`PB23`.
- `millis`, `micros`, `delay`, `delayMicroseconds` (SysTick de 64 bits).
- `Serial1` = UART1 (TX **PA9**, RX **PA8**), `Serial0` (PB7/PB4), `Serial2` (PA7/PA6),
  `Serial3` (PA5/PA4); `printf`, `serialEvent()`. `Serial` es `Serial1`, o el USB con el menú
  **USB support → Serial = USB CDC**.
- `SerialUSB`: puerto COM por el USB de la placa (CDC, sin driver en Windows 10/11, Linux y macOS).
- **Wire** (I2C): por hardware en CH58x/CH59x, maestro y esclavo (SDA **PB12**, SCL **PB13**, o
  `Wire.setPins(PB20, PB21)`); por software en el CH573 (no tiene I2C) o en otras patas.
  `setClock` hasta 400 kHz, `setWireTimeout`.
- **SPI**: SPI0 maestro (SCK **PA13**, MOSI **PA14**, MISO **PA15**; o `SPI.setPins(PB13, PB15, PB14)`),
  modos 0 y 3 por hardware hasta Fsys/2, modos 1 y 2 por software.
- **EEPROM**: 1024 bytes en la Data-Flash (`begin`, `read/write`, `get/put`, `commit`).
- `tone()` / `noTone()` en cualquier pata (TMR0); `sleepUntilInterrupt()` (WFI).
- `analogRead(A0…A13)` (ADC de 12 bits; 10 bits por defecto, `analogReadResolution(12)`),
  `analogReadMilliVolts`, `analogReadVccMilliVolts`, `temperatureRead`.
- `analogWrite` en PA12, PA13, PB0, PB4, PB6, PB7, PB14, PB23 (PWM4–PWM11, 8 bits),
  `analogWriteFrequency`.
- `attachInterrupt` en cualquier pata (RISING, FALLING, CHANGE, LOW, HIGH).
- Reloj: menú **Clock Select** (60 MHz por defecto; 80 MHz en CH58x).

Pendiente: BLE, bajo consumo profundo (RTC), CH570/CH572.

En las placas de WeAct el LED está en **PA8** (se enciende en bajo: `LED_ON`/`LED_OFF`) y el botón
BOOT en **PB22** (`KEY_BUILTIN`). PA8 es también el RX de `Serial`.

## Carga

- **WCH-ISP (USB)**: mantenga pulsado BOOT, conecte el USB y suba. Usa
  [wchisp](https://github.com/ch32-rs/wchisp) (incluido en `tools/wchisp`). En Windows hace falta
  el driver WinUSB para el dispositivo de arranque de WCH (con Zadig) y el runtime de Visual C++.
- **WCH-Link / WCH-LinkE** (modo RISC-V, openocd de WCH): conecte 3V3, GND, **TIO** (SWDIO) y
  **TCK** (SWCLK). En los CH58x/CH59x/CH57x la interfaz de depuracion de 2 hilos viene
  **desactivada de fabrica**: activela una vez por USB/ISP con
  `wchisp config enable-debug` (o con WCHISPTool: *Enable 2-Wire Debug Interface*). Por probar.

## Licencias

SDK de WCH: Apache 2.0 (openwch). CH573EVT: de WCH, copia publicada por WeAct Studio (MIT).
wchisp: MIT/Apache 2.0. Núcleo `cores/ch5xx`: LGPL 2.1, como Arduino.
