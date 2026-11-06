/*
 * Tone.cpp - tone() / noTone() en cualquier pata con el TMR0 (interrupcion que
 * cambia la pata dos veces por periodo). Una sola nota a la vez, como en AVR.
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 */
#include "Arduino.h"

static volatile uint8_t tone_pin = 0xFF;
static volatile uint32_t tone_toggles;   /* 0 = sin fin */

static void tone_stop(void)
{
    TMR0_ITCfg(DISABLE, TMR0_3_IT_CYC_END);
    TMR0_Disable();
    PFIC_DisableIRQ(TMR0_IRQn);
}

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
    uint32_t half;
    if (pin >= CH5XX_NUM_PINS) return;
    if (frequency == 0) { noTone(pin); return; }
    if (tone_pin != 0xFF && tone_pin != pin) noTone(tone_pin);
    tone_stop();
    tone_pin = pin;
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    /* medio periodo en ciclos (el TMR0 cuenta a Fsys) */
    half = GetSysClock() / (2UL * frequency);
    if (half < 50) half = 50;
    tone_toggles = duration ? (uint32_t)((2ULL * frequency * duration) / 1000ULL) : 0;
    if (duration && tone_toggles == 0) tone_toggles = 2;
    TMR0_TimerInit(half);
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_EnableIRQ(TMR0_IRQn);
}

void noTone(uint8_t pin)
{
    tone_stop();
    if (tone_pin != 0xFF) digitalWrite(tone_pin, LOW);
    (void)pin;
    tone_pin = 0xFF;
}

extern "C" __INTERRUPT __HIGH_CODE void TMR0_IRQHandler(void)
{
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    if (tone_pin == 0xFF) return;
    digitalToggle(tone_pin);
    if (tone_toggles && --tone_toggles == 0) {
        tone_stop();
        digitalWrite(tone_pin, LOW);
        tone_pin = 0xFF;
    }
}
