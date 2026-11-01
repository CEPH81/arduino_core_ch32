/*
 * Arduino.h - nucleo Arduino para WCH CH58x (CH582, CH583) y CH59x (CH592),
 * RISC-V con el SDK oficial de WCH (openwch). Parte de arduino_core_ch32 (fork de CEPH81).
 *
 * Patas: PA0..PA15 = 0..15, PB0..PB23 = 16..39.
 */
#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ch5xx_sdk.h"
#include "binary.h"
#include "itoa.h"
#include "wiring_constants.h"
#include "pins_arduino.h"

#undef interrupts
#undef noInterrupts
#define interrupts()   __asm volatile("csrsi mstatus, 0x8")
#define noInterrupts() __asm volatile("csrci mstatus, 0x8")

#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)
#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())
#define microsecondsToClockCycles(a) ((a) * clockCyclesPerMicrosecond())

#define digitalPinToPort(p)    ((p) < 16 ? 0 : 1)
#define digitalPinToBitMask(p) (1UL << ((p) < 16 ? (p) : (p) - 16))
#define digitalPinToInterrupt(p) (p)
#define NOT_A_PIN 0xFF
#define NOT_AN_INTERRUPT -1

#ifdef __cplusplus
extern "C" {
#endif

void init(void);
void initVariant(void);
void setup(void);
void loop(void);
void yield(void);

void pinMode(uint32_t pin, uint32_t mode);
void digitalWrite(uint32_t pin, uint32_t val);
int digitalRead(uint32_t pin);
void digitalToggle(uint32_t pin);

int analogRead(uint32_t pin);
void analogReadResolution(int bits);
void analogReference(uint8_t mode);
void analogWrite(uint32_t pin, int value);
void analogWriteResolution(int bits);
void analogWriteFrequency(uint32_t freq);
float temperatureRead(void);               /* sensor interno, grados C */
uint32_t analogReadVccMilliVolts(void);    /* tension de alimentacion (VBAT) */
uint32_t analogReadMilliVolts(uint32_t pin);

uint32_t millis(void);
uint32_t micros(void);
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

typedef void (*voidFuncPtr)(void);
void attachInterrupt(uint32_t pin, voidFuncPtr callback, uint32_t mode);
void detachInterrupt(uint32_t pin);

void shiftOut(uint32_t dataPin, uint32_t clockPin, uint32_t bitOrder, uint32_t val);
uint32_t shiftIn(uint32_t dataPin, uint32_t clockPin, uint32_t bitOrder);
uint32_t pulseIn(uint32_t pin, uint32_t state, uint32_t timeout);
uint32_t pulseInLong(uint32_t pin, uint32_t state, uint32_t timeout);

uint32_t getCpuFrequencyMhz(void);

#ifdef __cplusplus
}
#endif

#define LSBFIRST 0
#define MSBFIRST 1

#ifdef __cplusplus
#include "WCharacter.h"
#include "WString.h"
#include "WMath.h"
#include "Stream.h"
#include "HardwareSerial.h"

uint32_t pulseIn(uint32_t pin, uint32_t state);
#endif

#endif
