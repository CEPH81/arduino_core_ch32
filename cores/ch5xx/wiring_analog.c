/*
 * wiring_analog.c - analogRead() (ADC de 12 bits), analogWrite() (PWM4-PWM11),
 * temperatureRead() y analogReadVccMilliVolts().
 *
 * ADC: ganancia 1/4 (-12 dB), entrada de 0 V a VIO33; tension = (ADC/512 - 3) * 1,05 V.
 * analogRead() toma como fondo de escala 3,3 V. Por defecto da 10 bits (0-1023) como Arduino; analogReadResolution(12).
 * PWM: 8 bits, frecuencia = F_CPU / divisor / 256 (por defecto ~1 kHz).
 */
#include "Arduino.h"

static uint8_t lectura_bits = 10;
static uint8_t escritura_bits = 8;
static int16_t adc_offset;
static uint8_t adc_listo;

static const int8_t canal_adc[16] = {
    /* PA0..PA15 -> AIN */ 9, 8, 7, 6, 0, 1, 10, 11, 12, 13, -1, -1, 2, 3, 4, 5};

void analogReadResolution(int bits) { if (bits >= 1 && bits <= 16) lectura_bits = bits; }
void analogWriteResolution(int bits) { if (bits >= 1 && bits <= 16) escritura_bits = bits; }
void analogReference(uint8_t mode) { (void)mode; }

static uint16_t convertir(uint8_t ch)
{
    if (!adc_listo) {
        ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_1_4);
        adc_offset = ADC_DataCalib_Rough();
        adc_listo = 1;
    }
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_1_4);
    ADC_ChannelCfg(ch);
    int32_t v = (int32_t)ADC_ExcutSingleConver() + adc_offset;
    if (v < 0) v = 0;
    if (v > 4095) v = 4095;
    return (uint16_t)v;
}

int analogRead(uint32_t pin)
{
    if (pin >= 16 || canal_adc[pin] < 0) return 0;
    GPIOA_ModeCfg(1UL << pin, GPIO_ModeIN_Floating);
    uint32_t v = convertir(canal_adc[pin]);
    /* con 1/4 de ganancia: 0 V = 1536 y 3,3 V = 1536 + 3300 * 512 / 1050 = 3145;
       se escala para que 0..3,3 V sea 0..4095 */
    v = v > 1536 ? v - 1536 : 0;
    if (v > 1609) v = 1609;
    v = v * 4095 / 1609;
    return lectura_bits >= 12 ? (int)(v << (lectura_bits - 12)) : (int)(v >> (12 - lectura_bits));
}

uint32_t analogReadMilliVolts(uint32_t pin)
{
    if (pin >= 16 || canal_adc[pin] < 0) return 0;
    GPIOA_ModeCfg(1UL << pin, GPIO_ModeIN_Floating);
    int32_t mv = (int32_t)convertir(canal_adc[pin]) * 1050 / 512 - 3150;
    return mv < 0 ? 0 : (uint32_t)mv;
}

float temperatureRead(void)
{
    ADC_InterTSSampInit();
    uint16_t v = ADC_ExcutSingleConver();
    adc_listo = 0;
    return (float)adc_to_temperature_celsius(v);
}

uint32_t analogReadVccMilliVolts(void)
{
    ADC_InterBATSampInit();
    int32_t v = (int32_t)ADC_ExcutSingleConver();
    adc_listo = 0;
    /* VBAT con ganancia 1/4 */
    int32_t mv = v * 1050 / 512 - 3150;
    return mv < 0 ? 0 : (uint32_t)mv;
}

/* ---------- PWM ---------- */
static const struct { uint8_t pin, ch; } pwm[] = {
    {PA12, CH_PWM4}, {PA13, CH_PWM5}, {PB0, CH_PWM6}, {PB4, CH_PWM7},
    {PB6, CH_PWM8}, {PB7, CH_PWM9}, {PB14, CH_PWM10}, {PB23, CH_PWM11},
};
static uint8_t pwm_listo;
static uint32_t pwm_freq = 1000;

void analogWriteFrequency(uint32_t freq)
{
    if (freq == 0) return;
    pwm_freq = freq;
    pwm_listo = 0;
}

void analogWrite(uint32_t pin, int value)
{
    uint8_t ch = 0;
    for (unsigned i = 0; i < sizeof(pwm) / sizeof(pwm[0]); i++)
        if (pwm[i].pin == pin) ch = pwm[i].ch;
    uint32_t max = (1UL << escritura_bits) - 1;
    if (value < 0) value = 0;
    if ((uint32_t)value > max) value = max;
    if (!ch) {                       /* sin PWM: salida digital */
        pinMode(pin, OUTPUT);
        digitalWrite(pin, (uint32_t)value >= max / 2 + 1);
        return;
    }
    if (!pwm_listo) {
        uint32_t div = F_CPU / 256 / pwm_freq;
        if (div < 1) div = 1;
        if (div > 255) div = 255;
        PWMX_CLKCfg(div);
        PWMX_CycleCfg(PWMX_Cycle_256);
        pwm_listo = 1;
    }
    uint32_t d = (uint32_t)value * 256 / (max + 1);
    pinMode(pin, OUTPUT);
    if (d == 0) {
        PWMX_ACTOUT(ch, 0, High_Level, DISABLE);
        digitalWrite(pin, LOW);
    } else {
        PWMX_ACTOUT(ch, d > 255 ? 255 : d, High_Level, ENABLE);
    }
}
