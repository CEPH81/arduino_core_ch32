/*
 * wiring_time.c - millis(), micros(), delay() con el SysTick de 64 bits del nucleo
 * QingKe, contando libre a F_CPU (sin interrupciones).
 * En el CH573 (QingKe V3A) los bits de control tienen otros nombres y la cuenta
 * puede ser descendente: se comprueba al arrancar.
 */
#include "Arduino.h"

#ifndef SysTick_CTLR_STE
#define SysTick_CTLR_STE   SysTick_CTRL_ENABLE_Msk
#define SysTick_CTLR_STCLK SysTick_CTRL_CLKSOURCE_Msk
#endif

static uint8_t descendente;

static inline uint64_t cuenta(void)
{
    volatile uint32_t *c = (volatile uint32_t *)&SysTick->CNT;
    uint32_t h, l;
    do {
        h = c[1];
        l = c[0];
    } while (h != c[1]);
    return ((uint64_t)h << 32) | l;
}

static inline uint64_t ticks(void)
{
    uint64_t c = cuenta();
    return descendente ? ~c : c;
}

void ch5xx_systick_start(void)
{
    SysTick->CTLR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = 0xFFFFFFFFFFFFFFFFULL;
    SysTick->CTLR = SysTick_CTLR_STCLK | SysTick_CTLR_STE;  /* HCLK, sin recarga ni interrupcion */
    uint64_t a = cuenta();
    for (volatile int i = 0; i < 100; i++)
        ;
    descendente = cuenta() < a;
}

uint32_t millis(void)
{
    return (uint32_t)(ticks() / (F_CPU / 1000));
}

uint32_t micros(void)
{
    return (uint32_t)(ticks() / (F_CPU / 1000000));
}

void delay(uint32_t ms)
{
    uint64_t fin = ticks() + (uint64_t)ms * (F_CPU / 1000);
    while (ticks() < fin)
        yield();
}

void delayMicroseconds(uint32_t us)
{
    uint64_t fin = ticks() + (uint64_t)us * (F_CPU / 1000000);
    while (ticks() < fin)
        ;
}

uint32_t getCpuFrequencyMhz(void)
{
    return GetSysClock() / 1000000;
}

__attribute__((weak)) void yield(void) {}
