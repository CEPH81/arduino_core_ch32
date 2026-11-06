/*
 * SPI_ch5xx.cpp - SPI0 maestro de los WCH CH58x / CH59x / CH57x.
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * Intercambio de un byte (modo BUFFER, como SPI0_MasterSendByte del SDK):
 * se escribe R8_SPI0_BUFFER, se espera RB_SPI_FREE y se lee lo recibido.
 */
#if defined(CH58x) || defined(CH59x) || defined(CH573)

#include "SPI_ch5xx.h"

SPIClass::SPIClass()
    : _sck(PIN_SPI_SCK), _miso(PIN_SPI_MISO), _mosi(PIN_SPI_MOSI), _mode(SPI_MODE0),
      _order(MSBFIRST), _clock(4000000), _hw(true), _begun(false), _half_us(0)
{
}

bool SPIClass::setPins(uint8_t sck, uint8_t miso, uint8_t mosi)
{
    if (sck >= CH5XX_NUM_PINS || miso >= CH5XX_NUM_PINS || mosi >= CH5XX_NUM_PINS) return false;
    _sck = sck;
    _miso = miso;
    _mosi = mosi;
    if (_begun) begin();
    return true;
}

static bool spi0_pins_a(uint8_t sck, uint8_t miso, uint8_t mosi)
{
    return sck == PA13 && miso == PA15 && mosi == PA14;
}
static bool spi0_pins_b(uint8_t sck, uint8_t miso, uint8_t mosi)
{
    return sck == PB13 && miso == PB15 && mosi == PB14;
}

void SPIClass::begin()
{
    _begun = true;
    apply();
}

void SPIClass::end()
{
    if (_hw) R8_SPI0_CTRL_MOD = RB_SPI_ALL_CLEAR;
    pinMode(_sck, INPUT);
    pinMode(_mosi, INPUT);
    pinMode(_miso, INPUT);
    _begun = false;
}

void SPIClass::apply()
{
    bool a = spi0_pins_a(_sck, _miso, _mosi), b = spi0_pins_b(_sck, _miso, _mosi);
    /* el SPI0 solo hace los modos 0 y 3 */
    _hw = (a || b) && (_mode == SPI_MODE0 || _mode == SPI_MODE3);

    /* reposo del reloj: bajo en modos 0/1, alto en 2/3 */
    digitalWrite(_sck, (_mode & 2) ? HIGH : LOW);
    pinMode(_sck, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT_PULLUP);

    if (_hw) {
        uint32_t div = (F_CPU + _clock - 1) / _clock;
        if (div < 2) div = 2;
        if (div > 255) div = 255;
        if (b) R16_PIN_ALTERNATE |= RB_PIN_SPI0;
        else R16_PIN_ALTERNATE &= ~RB_PIN_SPI0;
        R8_SPI0_CTRL_MOD = RB_SPI_ALL_CLEAR;
        R8_SPI0_CTRL_MOD = RB_SPI_MOSI_OE | RB_SPI_SCK_OE | (_mode == SPI_MODE3 ? RB_SPI_MST_SCK_MOD : 0);
        R8_SPI0_CTRL_CFG = RB_SPI_AUTO_IF | (_order == LSBFIRST ? RB_SPI_BIT_ORDER : 0) |
                           (div == 2 ? RB_SPI_MST_DLY_EN : 0);   /* a Fsys/2 hay que retrasar la entrada */
        R8_SPI0_CLOCK_DIV = (uint8_t)div;
    } else {
        R8_SPI0_CTRL_MOD = RB_SPI_ALL_CLEAR;
        _half_us = 500000UL / (_clock ? _clock : 1);
    }
}

void SPIClass::beginTransaction(SPISettings s)
{
    uint32_t clk = s._clock ? s._clock : 1;
    uint8_t mode = s._dataMode & 3;
    /* solo se reconfigura si cambia algo (las bibliotecas llaman en cada acceso) */
    if (_begun && clk == _clock && s._bitOrder == _order && mode == _mode) return;
    _clock = clk;
    _order = s._bitOrder;
    _mode = mode;
    apply();
}

void SPIClass::endTransaction() {}

uint8_t SPIClass::sw_transfer(uint8_t out)
{
    uint8_t in = 0;
    bool cpha = _mode & 1, idle = (_mode & 2) != 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = _order == LSBFIRST ? (out >> i) & 1 : (out >> (7 - i)) & 1;
        uint8_t got = 0;
        if (!cpha) digitalWrite(_mosi, bit);
        delayMicroseconds(_half_us);
        digitalWrite(_sck, !idle);              /* primer flanco */
        if (cpha) digitalWrite(_mosi, bit);
        else got = digitalRead(_miso);
        delayMicroseconds(_half_us);
        digitalWrite(_sck, idle);               /* segundo flanco */
        if (cpha) got = digitalRead(_miso);
        if (_order == LSBFIRST) in |= got << i;
        else in |= got << (7 - i);
    }
    return in;
}

uint8_t SPIClass::transfer(uint8_t data)
{
    if (!_hw) return sw_transfer(data);
    R8_SPI0_CTRL_MOD &= ~RB_SPI_FIFO_DIR;
    R8_SPI0_BUFFER = data;
    while (!(R8_SPI0_INT_FLAG & RB_SPI_FREE))
        ;
    return R8_SPI0_BUFFER;
}

uint16_t SPIClass::transfer16(uint16_t data)
{
    uint16_t r;
    if (_order == LSBFIRST) {
        r = transfer((uint8_t)data);
        r |= (uint16_t)transfer((uint8_t)(data >> 8)) << 8;
    } else {
        r = (uint16_t)transfer((uint8_t)(data >> 8)) << 8;
        r |= transfer((uint8_t)data);
    }
    return r;
}

void SPIClass::transfer(void *buf, size_t count)
{
    uint8_t *p = (uint8_t *)buf;
    while (count--) {
        *p = transfer(*p);
        p++;
    }
}

void SPIClass::transfer(const void *txbuf, void *rxbuf, size_t count)
{
    const uint8_t *t = (const uint8_t *)txbuf;
    uint8_t *r = (uint8_t *)rxbuf;
    while (count--) {
        uint8_t v = transfer(t ? *t++ : 0xFF);
        if (r) *r++ = v;
    }
}

void SPIClass::setBitOrder(uint8_t order) { _order = order; if (_begun) apply(); }
void SPIClass::setDataMode(uint8_t mode) { _mode = mode & 3; if (_begun) apply(); }
void SPIClass::setFrequency(uint32_t hz) { _clock = hz ? hz : 1; if (_begun) apply(); }
void SPIClass::setClockDivider(uint8_t div) { setFrequency(16000000UL / (div ? div : 1)); }

SPIClass SPI;

#endif /* CH58x || CH59x || CH573 */
