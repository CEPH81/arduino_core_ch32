/*
 * Wire_ch5xx.cpp - I2C para WCH CH58x / CH59x (hardware) y CH57x (software).
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * El I2C de los CH58x/CH59x es igual al de los STM32F1 (mismos registros y
 * eventos): CTRL1/CTRL2, OADDR1, DATAR, STAR1/STAR2, CKCFGR, RTR.
 */
#if defined(CH58x) || defined(CH59x) || defined(CH573)

#include "Wire_ch5xx.h"

#if defined(CH58x) || defined(CH59x)
#define WIRE_HAS_HW 1
#ifndef RB_I2C_MUST1
#define RB_I2C_MUST1 0x4000   /* OADDR1 bit 14: debe quedar en 1 (falta en el SDK del CH592) */
#endif
#else
#define WIRE_HAS_HW 0
#endif

#if WIRE_HAS_HW
static TwoWire *wire_isr_obj = nullptr;
#endif

TwoWire::TwoWire()
    : _sda(PIN_WIRE_SDA), _scl(PIN_WIRE_SCL), _hw(false), _slave(false), _transmitting(false),
      _timeout_flag(false), _timeout_reset(false), _clock(100000), _timeout_us(25000),
      _txAddress(0), _rxIndex(0), _rxLength(0), _txLength(0), _txIndex(0),
      _onReceive(nullptr), _onRequest(nullptr), _half_us(5), _sw_started(false)
{
}

bool TwoWire::setPins(uint8_t sda, uint8_t scl)
{
    if (sda >= CH5XX_NUM_PINS || scl >= CH5XX_NUM_PINS) return false;
    _sda = sda;
    _scl = scl;
    return true;
}

/* ------------------------------------------------------------ patas */
static inline void pin_dir_out(uint8_t p)
{
    if (p < 16) R32_PA_DIR |= 1UL << p; else R32_PB_DIR |= 1UL << (p - 16);
}
static inline void pin_dir_in(uint8_t p)
{
    if (p < 16) R32_PA_DIR &= ~(1UL << p); else R32_PB_DIR &= ~(1UL << (p - 16));
}
static inline bool pin_read(uint8_t p)
{
    return p < 16 ? (R32_PA_PIN >> p) & 1 : (R32_PB_PIN >> (p - 16)) & 1;
}

/* ------------------------------------------------------------ comun */
void TwoWire::begin()
{
    _slave = false;
    _rxIndex = _rxLength = 0;
    _txLength = 0;
#if WIRE_HAS_HW
    _hw = (_sda == PB12 && _scl == PB13) || (_sda == PB20 && _scl == PB21);
#else
    _hw = false;
#endif
    if (_hw) {
        hw_init();
    } else {
        /* software: el latch en 0 y la direccion hace de drenador abierto */
        pinMode(_sda, INPUT_PULLUP);
        pinMode(_scl, INPUT_PULLUP);
        digitalWrite(_sda, LOW);
        digitalWrite(_scl, LOW);
        setClock(_clock);
        _sw_started = false;
    }
}

void TwoWire::begin(uint8_t address)
{
#if WIRE_HAS_HW
    begin();
    if (!_hw) return;   /* esclavo solo con I2C por hardware */
    _slave = true;
    wire_isr_obj = this;
    R16_I2C_OADDR1 = RB_I2C_MUST1 | ((uint16_t)address << 1);
    R16_I2C_CTRL1 |= RB_I2C_ACK;
    R16_I2C_CTRL2 |= RB_I2C_ITEVTEN | RB_I2C_ITBUFEN | RB_I2C_ITERREN;
    PFIC_EnableIRQ(I2C_IRQn);
#else
    (void)address;
    begin();
#endif
}

void TwoWire::end()
{
#if WIRE_HAS_HW
    if (_hw) {
        PFIC_DisableIRQ(I2C_IRQn);
        R16_I2C_CTRL2 &= ~(RB_I2C_ITEVTEN | RB_I2C_ITBUFEN | RB_I2C_ITERREN);
        R16_I2C_CTRL1 &= ~RB_I2C_PE;
        wire_isr_obj = nullptr;
    }
#endif
    pinMode(_sda, INPUT);
    pinMode(_scl, INPUT);
    _slave = false;
}

void TwoWire::setClock(uint32_t hz)
{
    if (hz < 1000) hz = 1000;
    if (hz > 400000) hz = 400000;
    _clock = hz;
#if WIRE_HAS_HW
    if (_hw) {
        uint16_t own = R16_I2C_OADDR1 & 0x3FF;
        I2C_Init(I2C_Mode_I2C, hz, I2C_DutyCycle_2, I2C_Ack_Enable, I2C_AckAddr_7bit, own);
        return;
    }
#endif
    /* medio periodo; la llamada y los accesos a las patas ya cuestan ~1 us */
    _half_us = 500000UL / hz;
    _half_us = _half_us > 1 ? _half_us - 1 : 0;
}

void TwoWire::setWireTimeout(uint32_t timeout_us, bool reset_with_timeout)
{
    _timeout_us = timeout_us;
    _timeout_reset = reset_with_timeout;
}

void TwoWire::beginTransmission(uint8_t address)
{
    _transmitting = true;
    _txAddress = address;
    _txLength = 0;
}

uint8_t TwoWire::endTransmission(bool sendStop)
{
    uint8_t r;
    if (!_transmitting) return 4;
    _transmitting = false;
#if WIRE_HAS_HW
    if (_hw) r = hw_write(_txAddress, _txBuf, _txLength, sendStop);
    else
#endif
        r = sw_write(_txAddress, _txBuf, _txLength, sendStop);
    _txLength = 0;
    return r;
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t quantity, bool sendStop)
{
    uint16_t n;
    if (quantity > WIRE_BUFFER_LENGTH) quantity = WIRE_BUFFER_LENGTH;
    if (!quantity) return 0;
#if WIRE_HAS_HW
    if (_hw) n = hw_read(address, _rxBuf, (uint16_t)quantity, sendStop);
    else
#endif
        n = sw_read(address, _rxBuf, (uint16_t)quantity, sendStop);
    _rxIndex = 0;
    _rxLength = n;
    return (uint8_t)n;
}

/* lectura con direccion interna (registro de memoria): como los nucleos STM32/ESP32 */
uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop)
{
    if (isize) {
        if (isize > 4) isize = 4;
        beginTransmission(address);
        while (isize--) write((uint8_t)(iaddress >> (isize * 8)));
        if (endTransmission(false)) return 0;
    }
    return requestFrom(address, (size_t)quantity, (bool)sendStop);
}

size_t TwoWire::write(uint8_t data)
{
    if (_transmitting || _slave) {
        if (_txLength >= WIRE_BUFFER_LENGTH) {
            setWriteError();
            return 0;
        }
        _txBuf[_txLength++] = data;
        return 1;
    }
    return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
    size_t i;
    for (i = 0; i < quantity; i++)
        if (!write(data[i])) break;
    return i;
}

int TwoWire::available() { return _rxLength - _rxIndex; }
int TwoWire::read() { return _rxIndex < _rxLength ? _rxBuf[_rxIndex++] : -1; }
int TwoWire::peek() { return _rxIndex < _rxLength ? _rxBuf[_rxIndex] : -1; }
void TwoWire::flush() { _rxIndex = _rxLength = 0; _txLength = 0; }
void TwoWire::onReceive(void (*cb)(int)) { _onReceive = cb; }
void TwoWire::onRequest(void (*cb)(void)) { _onRequest = cb; }

/* ------------------------------------------------------------ hardware */
#if WIRE_HAS_HW
void TwoWire::hw_init()
{
    if (_sda == PB20) R16_PIN_ALTERNATE |= RB_PIN_I2C;
    else R16_PIN_ALTERNATE &= ~RB_PIN_I2C;
    /* como los ejemplos de WCH: entradas con pull-up; el I2C toma las patas */
    GPIOB_ModeCfg((1UL << (_sda - 16)) | (1UL << (_scl - 16)), GPIO_ModeIN_PU);
    I2C_Init(I2C_Mode_I2C, _clock, I2C_DutyCycle_2, I2C_Ack_Enable, I2C_AckAddr_7bit, 0);
}

/* Bus colgado (un esclavo sujetando SDA): 9 pulsos de SCL y reinicio */
void TwoWire::hw_recover()
{
    uint8_t i;
    R16_I2C_CTRL1 &= ~RB_I2C_PE;
    if (_scl >= 16) {
        GPIOB_ModeCfg(1UL << (_scl - 16), GPIO_ModeOut_PP_5mA);
        for (i = 0; i < 9 && !pin_read(_sda); i++) {
            digitalWrite(_scl, LOW);
            delayMicroseconds(5);
            digitalWrite(_scl, HIGH);
            delayMicroseconds(5);
        }
    }
    hw_init();
}

bool TwoWire::hw_wait(uint32_t event, uint32_t t0)
{
    while (!I2C_CheckEvent(event)) {
        if (R16_I2C_STAR1 & (RB_I2C_AF | RB_I2C_ARLO | RB_I2C_BERR)) return false;
        if (_timeout_us && (micros() - t0) > _timeout_us) {
            _timeout_flag = true;
            return false;
        }
    }
    return true;
}

uint8_t TwoWire::hw_write(uint8_t addr, const uint8_t *d, uint16_t n, bool stop)
{
    uint32_t t0 = micros();
    uint8_t r = 0;
    /* bus ocupado por otro maestro (no por un start repetido nuestro) */
    while ((R16_I2C_STAR2 & RB_I2C_BUSY) && !(R16_I2C_STAR2 & RB_I2C_MSL)) {
        if (_timeout_us && (micros() - t0) > _timeout_us) {
            _timeout_flag = true;
            if (_timeout_reset) hw_recover();
            return 5;
        }
    }
    R16_I2C_STAR1 &= ~(RB_I2C_AF | RB_I2C_ARLO | RB_I2C_BERR);
    R16_I2C_CTRL1 |= RB_I2C_START;
    if (!hw_wait(I2C_EVENT_MASTER_MODE_SELECT, t0)) { r = _timeout_flag ? 5 : 4; goto fin; }
    I2C_Send7bitAddress(addr << 1, I2C_Direction_Transmitter);
    if (!hw_wait(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, t0)) {
        r = (R16_I2C_STAR1 & RB_I2C_AF) ? 2 : (_timeout_flag ? 5 : 4);
        goto fin;
    }
    for (uint16_t i = 0; i < n; i++) {
        R16_I2C_DATAR = d[i];
        if (!hw_wait(I2C_EVENT_MASTER_BYTE_TRANSMITTED, t0)) {
            r = (R16_I2C_STAR1 & RB_I2C_AF) ? 3 : (_timeout_flag ? 5 : 4);
            goto fin;
        }
    }
    if (stop) R16_I2C_CTRL1 |= RB_I2C_STOP;
    return 0;
fin:
    R16_I2C_STAR1 &= ~(RB_I2C_AF | RB_I2C_ARLO | RB_I2C_BERR);
    R16_I2C_CTRL1 |= RB_I2C_STOP;
    if (r == 5 && _timeout_reset) hw_recover();
    return r;
}

uint16_t TwoWire::hw_read(uint8_t addr, uint8_t *d, uint16_t n, bool stop)
{
    uint32_t t0 = micros();
    uint16_t i = 0;
    while ((R16_I2C_STAR2 & RB_I2C_BUSY) && !(R16_I2C_STAR2 & RB_I2C_MSL)) {
        if (_timeout_us && (micros() - t0) > _timeout_us) {
            _timeout_flag = true;
            if (_timeout_reset) hw_recover();
            return 0;
        }
    }
    R16_I2C_STAR1 &= ~(RB_I2C_AF | RB_I2C_ARLO | RB_I2C_BERR);
    R16_I2C_CTRL1 |= RB_I2C_ACK;
    R16_I2C_CTRL1 |= RB_I2C_START;
    if (!hw_wait(I2C_EVENT_MASTER_MODE_SELECT, t0)) goto fin;
    I2C_Send7bitAddress(addr << 1, I2C_Direction_Receiver);
    /* un solo byte: sin ACK antes de borrar ADDR (lo borra CheckEvent) */
    if (n == 1) R16_I2C_CTRL1 &= ~RB_I2C_ACK;
    if (!hw_wait(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, t0)) goto fin;
    for (i = 0; i < n; i++) {
        if (i == n - 1) {
            /* ultimo byte: NACK y STOP mientras se recibe */
            R16_I2C_CTRL1 &= ~RB_I2C_ACK;
            if (stop) R16_I2C_CTRL1 |= RB_I2C_STOP;
        }
        if (!hw_wait(I2C_EVENT_MASTER_BYTE_RECEIVED, t0)) goto fin;
        d[i] = (uint8_t)R16_I2C_DATAR;
    }
    R16_I2C_CTRL1 |= RB_I2C_ACK;
    return n;
fin:
    R16_I2C_STAR1 &= ~(RB_I2C_AF | RB_I2C_ARLO | RB_I2C_BERR);
    R16_I2C_CTRL1 |= RB_I2C_STOP;
    R16_I2C_CTRL1 |= RB_I2C_ACK;
    if (_timeout_flag && _timeout_reset) hw_recover();
    return i;
}

/* Esclavo: igual que en los STM32F1 */
void TwoWire::_isr()
{
    uint16_t s1 = R16_I2C_STAR1, s2;
    if (s1 & RB_I2C_ADDR) {
        s2 = R16_I2C_STAR2;   /* leer STAR1 y STAR2 borra ADDR */
        if (s2 & RB_I2C_TRA) {
            /* el maestro lee: se piden los datos al sketch */
            _txLength = 0;
            _txIndex = 0;
            if (_onRequest) _onRequest();
            R16_I2C_DATAR = _txIndex < _txLength ? _txBuf[_txIndex++] : 0xFF;
        } else {
            _rxLength = 0;
            _rxIndex = 0;
        }
        return;
    }
    if (s1 & RB_I2C_RxNE) {
        uint8_t b = (uint8_t)R16_I2C_DATAR;
        if (_rxLength < WIRE_BUFFER_LENGTH) _rxBuf[_rxLength++] = b;
    }
    if ((s1 & RB_I2C_TxE) && (R16_I2C_STAR2 & RB_I2C_TRA))
        R16_I2C_DATAR = _txIndex < _txLength ? _txBuf[_txIndex++] : 0xFF;
    if (s1 & RB_I2C_AF) {
        /* NACK del maestro tras el ultimo byte: fin de la lectura */
        R16_I2C_STAR1 &= ~RB_I2C_AF;
        _txLength = 0;
    }
    if (s1 & RB_I2C_STOPF) {
        /* borrar STOPF: leer STAR1 (hecho) y escribir CTRL1 */
        R16_I2C_CTRL1 |= RB_I2C_PE;
        if (_rxLength && _onReceive) {
            _rxIndex = 0;
            _onReceive(_rxLength);
        }
    }
    if (s1 & (RB_I2C_BERR | RB_I2C_ARLO | RB_I2C_OVR))
        R16_I2C_STAR1 &= ~(RB_I2C_BERR | RB_I2C_ARLO | RB_I2C_OVR);
}

extern "C" __INTERRUPT __HIGH_CODE void I2C_IRQHandler(void)
{
    if (wire_isr_obj) wire_isr_obj->_isr();
}
#else
void TwoWire::hw_init() {}
void TwoWire::hw_recover() {}
#endif /* WIRE_HAS_HW */

/* ------------------------------------------------------------ software */
void TwoWire::sw_sda(bool high)
{
    if (high) pin_dir_in(_sda); else pin_dir_out(_sda);
}

void TwoWire::sw_scl(bool high)
{
    if (high) {
        uint32_t t0 = micros();
        pin_dir_in(_scl);
        /* estiramiento del reloj: el esclavo puede sujetar SCL */
        while (!pin_read(_scl)) {
            if (_timeout_us && (micros() - t0) > _timeout_us) {
                _timeout_flag = true;
                return;
            }
        }
    } else {
        pin_dir_out(_scl);
    }
}

bool TwoWire::sw_start(bool repeated)
{
    if (repeated) {
        sw_sda(true);
        delayMicroseconds(_half_us);
        sw_scl(true);
        delayMicroseconds(_half_us);
    } else if (!pin_read(_sda) || !pin_read(_scl)) {
        return false;   /* bus ocupado */
    }
    sw_sda(false);
    delayMicroseconds(_half_us);
    sw_scl(false);
    _sw_started = true;
    return true;
}

void TwoWire::sw_stop()
{
    sw_sda(false);
    delayMicroseconds(_half_us);
    sw_scl(true);
    delayMicroseconds(_half_us);
    sw_sda(true);
    delayMicroseconds(_half_us);
    _sw_started = false;
}

bool TwoWire::sw_byte_out(uint8_t b)
{
    bool ack;
    for (uint8_t i = 0; i < 8; i++) {
        sw_sda(b & 0x80);
        b <<= 1;
        delayMicroseconds(_half_us);
        sw_scl(true);
        delayMicroseconds(_half_us);
        sw_scl(false);
    }
    sw_sda(true);
    delayMicroseconds(_half_us);
    sw_scl(true);
    ack = !pin_read(_sda);
    delayMicroseconds(_half_us);
    sw_scl(false);
    return ack;
}

uint8_t TwoWire::sw_byte_in(bool ack)
{
    uint8_t b = 0;
    sw_sda(true);
    for (uint8_t i = 0; i < 8; i++) {
        delayMicroseconds(_half_us);
        sw_scl(true);
        b = (uint8_t)((b << 1) | pin_read(_sda));
        delayMicroseconds(_half_us);
        sw_scl(false);
    }
    sw_sda(!ack);
    delayMicroseconds(_half_us);
    sw_scl(true);
    delayMicroseconds(_half_us);
    sw_scl(false);
    sw_sda(true);
    return b;
}

uint8_t TwoWire::sw_write(uint8_t addr, const uint8_t *d, uint16_t n, bool stop)
{
    _timeout_flag = false;
    if (!sw_start(_sw_started)) return 4;
    if (!sw_byte_out((uint8_t)(addr << 1))) { sw_stop(); return _timeout_flag ? 5 : 2; }
    for (uint16_t i = 0; i < n; i++)
        if (!sw_byte_out(d[i])) { sw_stop(); return _timeout_flag ? 5 : 3; }
    if (stop) sw_stop();
    return 0;
}

uint16_t TwoWire::sw_read(uint8_t addr, uint8_t *d, uint16_t n, bool stop)
{
    uint16_t i;
    _timeout_flag = false;
    if (!sw_start(_sw_started)) return 0;
    if (!sw_byte_out((uint8_t)((addr << 1) | 1))) { sw_stop(); return 0; }
    for (i = 0; i < n; i++) d[i] = sw_byte_in(i < n - 1);
    if (stop) sw_stop();
    return n;
}

TwoWire Wire;

#endif /* CH58x || CH59x || CH573 */
