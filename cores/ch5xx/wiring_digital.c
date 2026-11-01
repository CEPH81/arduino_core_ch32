/*
 * wiring_digital.c - pinMode(), digitalWrite(), digitalRead() con los registros
 * del SDK (R32_Px_DIR/OUT/CLR/PIN/PU/PD_DRV).
 */
#include "Arduino.h"

void pinMode(uint32_t pin, uint32_t mode)
{
    GPIOModeTypeDef m;
    if (pin >= CH5XX_NUM_PINS) return;
    switch (mode) {
    case OUTPUT:         m = GPIO_ModeOut_PP_5mA; break;
    case OUTPUT_OD:      m = GPIO_ModeOut_PP_20mA; break;   /* sin drenador abierto: salida de 20 mA */
    case INPUT_PULLUP:   m = GPIO_ModeIN_PU; break;
    case INPUT_PULLDOWN: m = GPIO_ModeIN_PD; break;
    default:             m = GPIO_ModeIN_Floating; break;   /* INPUT, INPUT_ANALOG */
    }
    if (pin < 16) GPIOA_ModeCfg(1UL << pin, m);
    else          GPIOB_ModeCfg(1UL << (pin - 16), m);
}

void digitalWrite(uint32_t pin, uint32_t val)
{
    if (pin < 16) {
        if (val) R32_PA_OUT |= 1UL << pin;
        else     R32_PA_CLR = 1UL << pin;
    } else if (pin < CH5XX_NUM_PINS) {
        if (val) R32_PB_OUT |= 1UL << (pin - 16);
        else     R32_PB_CLR = 1UL << (pin - 16);
    }
}

void digitalToggle(uint32_t pin)
{
    if (pin < 16) R32_PA_OUT ^= 1UL << pin;
    else if (pin < CH5XX_NUM_PINS) R32_PB_OUT ^= 1UL << (pin - 16);
}

int digitalRead(uint32_t pin)
{
    if (pin < 16) return (R32_PA_PIN >> pin) & 1;
    if (pin < CH5XX_NUM_PINS) return (R32_PB_PIN >> (pin - 16)) & 1;
    return LOW;
}
