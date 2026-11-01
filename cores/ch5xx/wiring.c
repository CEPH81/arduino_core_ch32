/*
 * wiring.c - init(): reloj del sistema (menu "Reloj") y temporizador del sistema.
 */
#include "Arduino.h"

void ch5xx_systick_start(void);

static void reloj(void)
{
#if F_CPU == 80000000 && defined(CH58x)
    SetSysClock(CLK_SOURCE_PLL_80MHz);
#elif F_CPU == 60000000
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#elif F_CPU == 48000000
    SetSysClock(CLK_SOURCE_PLL_48MHz);
#elif F_CPU == 32000000
    SetSysClock(CLK_SOURCE_PLL_32MHz);
#elif F_CPU == 24000000
    SetSysClock(CLK_SOURCE_PLL_24MHz);
#else
#error "Frecuencia no soportada"
#endif
}

void init(void)
{
    reloj();
    ch5xx_systick_start();
}

__attribute__((weak)) void initVariant(void) {}
