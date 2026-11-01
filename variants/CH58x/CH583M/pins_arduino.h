/*
 * pins_arduino.h - CH583M (placa generica).
 * Patas: PA0..PA15 = 0..15, PB0..PB23 = 16..39 (ver cores/ch5xx/pins_ch5xx.h).
 * LED en PA8, encendido en bajo (como las placas de WeAct); boton BOOT en PB22.
 * Ojo: PA8 es tambien RX de Serial (UART1): con Serial.begin() el LED queda apagado.
 */
#ifndef Pins_Arduino_h
#define Pins_Arduino_h
#include "pins_ch5xx.h"

#define LED_BUILTIN PA8
#define LED_ON      LOW
#define LED_OFF     HIGH
#define KEY_BUILTIN PB22   /* boton BOOT, pulsado = LOW */
#endif
