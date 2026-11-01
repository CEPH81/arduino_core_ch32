/* wiring_shift.c - shiftOut(), shiftIn(), pulseIn(). */
#include "Arduino.h"

void shiftOut(uint32_t dataPin, uint32_t clockPin, uint32_t bitOrder, uint32_t val)
{
    for (int i = 0; i < 8; i++) {
        if (bitOrder == LSBFIRST) digitalWrite(dataPin, (val >> i) & 1);
        else digitalWrite(dataPin, (val >> (7 - i)) & 1);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint32_t shiftIn(uint32_t dataPin, uint32_t clockPin, uint32_t bitOrder)
{
    uint32_t v = 0;
    for (int i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        if (bitOrder == LSBFIRST) v |= digitalRead(dataPin) << i;
        else v |= digitalRead(dataPin) << (7 - i);
        digitalWrite(clockPin, LOW);
    }
    return v;
}

uint32_t pulseInLong(uint32_t pin, uint32_t state, uint32_t timeout)
{
    uint32_t t0 = micros(), ini;
    while (digitalRead(pin) == (int)state) if (micros() - t0 > timeout) return 0;
    while (digitalRead(pin) != (int)state) if (micros() - t0 > timeout) return 0;
    ini = micros();
    while (digitalRead(pin) == (int)state) if (micros() - t0 > timeout) return 0;
    return micros() - ini;
}

uint32_t pulseIn(uint32_t pin, uint32_t state, uint32_t timeout)
{
    return pulseInLong(pin, state, timeout);
}
