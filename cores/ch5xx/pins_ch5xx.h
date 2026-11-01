/*
 * pins_ch5xx.h - nombres de las patas de los CH58x/CH59x (todas las variantes).
 * PA0..PA15 = 0..15, PB0..PB23 = 16..39. Cada variante indica cuales tiene su placa.
 */
#ifndef PINS_CH5XX_H
#define PINS_CH5XX_H

enum {
    PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PB16, PB17, PB18, PB19, PB20, PB21, PB22, PB23,
    CH5XX_NUM_PINS
};

/* entradas analogicas: A0 = AIN0 ... (CH58x y CH59x: misma correspondencia) */
#define A0  PA4
#define A1  PA5
#define A2  PA12
#define A3  PA13
#define A4  PA14
#define A5  PA15
#define A6  PA3
#define A7  PA2
#define A8  PA1
#define A9  PA0
#define A10 PA6
#define A11 PA7
#define A12 PA8
#define A13 PA9

/* puertos serie por defecto (patas sin remapear) */
#define PIN_SERIAL0_RX PB4
#define PIN_SERIAL0_TX PB7
#define PIN_SERIAL1_RX PA8   /* Serial = UART1, como en los ejemplos de WCH */
#define PIN_SERIAL1_TX PA9
#define PIN_SERIAL2_RX PA6
#define PIN_SERIAL2_TX PA7
#define PIN_SERIAL3_RX PA4
#define PIN_SERIAL3_TX PA5

/* I2C y SPI0 */
#define PIN_WIRE_SDA PB12
#define PIN_WIRE_SCL PB13
#define PIN_SPI_SS   PA12
#define PIN_SPI_SCK  PA13
#define PIN_SPI_MOSI PA14
#define PIN_SPI_MISO PA15
#define SS   PIN_SPI_SS
#define SCK  PIN_SPI_SCK
#define MOSI PIN_SPI_MOSI
#define MISO PIN_SPI_MISO
#define SDA  PIN_WIRE_SDA
#define SCL  PIN_WIRE_SCL

#endif
