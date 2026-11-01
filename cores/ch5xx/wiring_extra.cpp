/* wiring_extra.cpp - funciones con valores por defecto de C++. */
#include "Arduino.h"

uint32_t pulseIn(uint32_t pin, uint32_t state) { return pulseInLong(pin, state, 1000000UL); }
