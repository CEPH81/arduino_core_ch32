/*
 * WInterrupts.c - attachInterrupt() en cualquier pata de PA y PB.
 * Modos: RISING, FALLING, LOW (nivel bajo), HIGH (nivel alto). CHANGE se
 * emula cambiando de flanco en cada interrupcion.
 */
#include "Arduino.h"

static voidFuncPtr fA[16], fB[24];
static uint8_t cambioA[16], cambioB[24];

static void configurar(uint32_t pin, GPIOITModeTpDef m)
{
    if (pin < 16) GPIOA_ITModeCfg(1UL << pin, m);
    else          GPIOB_ITModeCfg(1UL << (pin - 16), m);
}

void attachInterrupt(uint32_t pin, voidFuncPtr callback, uint32_t mode)
{
    GPIOITModeTpDef m;
    uint8_t cambio = 0;
    if (pin >= CH5XX_NUM_PINS) return;
    switch (mode) {
    case RISING:  m = GPIO_ITMode_RiseEdge; break;
    case FALLING: m = GPIO_ITMode_FallEdge; break;
    case HIGH:    m = GPIO_ITMode_HighLevel; break;
    case LOW:     m = GPIO_ITMode_LowLevel; break;
    default:      /* CHANGE: primero el flanco contrario al nivel actual */
        cambio = 1;
        m = digitalRead(pin) ? GPIO_ITMode_FallEdge : GPIO_ITMode_RiseEdge;
        break;
    }
    if (pin < 16) { fA[pin] = callback; cambioA[pin] = cambio; }
    else          { fB[pin - 16] = callback; cambioB[pin - 16] = cambio; }
    configurar(pin, m);
    PFIC_EnableIRQ(pin < 16 ? GPIO_A_IRQn : GPIO_B_IRQn);
}

void detachInterrupt(uint32_t pin)
{
    if (pin < 16) { R16_PA_INT_EN &= ~(1U << pin); fA[pin] = 0; }
    else if (pin < CH5XX_NUM_PINS) {
        uint32_t b = pin - 16;
        /* PB22/PB23 usan los bits 8 y 9 de los registros de interrupcion de PB */
        R16_PB_INT_EN &= ~(uint16_t)((b & 0xFF) | ((b >= 22) ? (1U << (b - 14)) : 0));
        fB[b] = 0;
        R16_PB_INT_EN &= ~(uint16_t)(1U << (b < 16 ? b : (b - 14)));
    }
}

__INTERRUPT __HIGH_CODE void GPIOA_IRQHandler(void)
{
    uint16_t f = R16_PA_INT_IF;
    R16_PA_INT_IF = f;
    while (f) {
        int i = __builtin_ctz(f);
        f &= f - 1;
        if (cambioA[i]) configurar(i, digitalRead(i) ? GPIO_ITMode_FallEdge : GPIO_ITMode_RiseEdge);
        if (fA[i]) fA[i]();
    }
}

__INTERRUPT __HIGH_CODE void GPIOB_IRQHandler(void)
{
    uint16_t f = R16_PB_INT_IF;
    R16_PB_INT_IF = f;
    while (f) {
        int i = __builtin_ctz(f);
        f &= f - 1;
        /* bits 8 y 9 pueden ser PB8/PB9 o PB22/PB23 (segun R16_PIN_ALTERNATE) */
        int b = i;
        if ((i == 8 || i == 9) && fB[i] == 0) b = i + 14;
        if (cambioB[b]) configurar(16 + b, digitalRead(16 + b) ? GPIO_ITMode_FallEdge : GPIO_ITMode_RiseEdge);
        if (fB[b]) fB[b]();
    }
}
