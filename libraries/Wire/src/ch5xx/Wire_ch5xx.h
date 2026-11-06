/*
 * Wire_ch5xx.h - biblioteca Wire (I2C) para los WCH CH58x, CH59x y CH57x.
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * - CH58x / CH59x: I2C por hardware (maestro y esclavo con interrupcion).
 *     SDA PB12, SCL PB13 (por defecto)  o  SDA PB20, SCL PB21 (remapeado).
 * - CH57x (CH573/CH571, sin I2C por hardware) o cualquier otro par de patas:
 *     I2C por software, solo maestro, hasta ~400 kHz.
 * Wire.setPins(sda, scl) antes de Wire.begin() elige las patas.
 */
#ifndef Wire_ch5xx_h
#define Wire_ch5xx_h

#include "Arduino.h"
#include "Stream.h"

#ifndef BUFFER_LENGTH
#define BUFFER_LENGTH 32
#endif
#ifndef WIRE_BUFFER_LENGTH
#define WIRE_BUFFER_LENGTH 128
#endif
#define WIRE_HAS_END 1
#define WIRE_HAS_TIMEOUT 1

class TwoWire : public Stream {
public:
    TwoWire();

    bool setPins(uint8_t sda, uint8_t scl);   /* antes de begin() */
    void setSDA(uint8_t sda) { _sda = sda; }
    void setSCL(uint8_t scl) { _scl = scl; }

    void begin();                     /* maestro */
    void begin(uint8_t address);      /* esclavo (solo I2C por hardware) */
    void begin(int address) { begin((uint8_t)address); }
    void end();
    void setClock(uint32_t hz);
    void setWireTimeout(uint32_t timeout_us = 25000, bool reset_with_timeout = false);
    bool getWireTimeoutFlag() { return _timeout_flag; }
    void clearWireTimeoutFlag() { _timeout_flag = false; }

    void beginTransmission(uint8_t address);
    void beginTransmission(int address) { beginTransmission((uint8_t)address); }
    /* 0 = OK, 1 = datos demasiado largos, 2 = NACK en la direccion,
     * 3 = NACK en un dato, 4 = otro error, 5 = tiempo agotado */
    uint8_t endTransmission(bool sendStop = true);
    uint8_t endTransmission(uint8_t sendStop) { return endTransmission((bool)sendStop); }

    uint8_t requestFrom(uint8_t address, size_t quantity, bool sendStop = true);
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) { return requestFrom(address, (size_t)quantity, (bool)sendStop); }
    uint8_t requestFrom(int address, int quantity) { return requestFrom((uint8_t)address, (size_t)quantity, true); }
    uint8_t requestFrom(int address, int quantity, int sendStop) { return requestFrom((uint8_t)address, (size_t)quantity, (bool)sendStop); }
    uint8_t requestFrom(uint8_t address, uint8_t quantity) { return requestFrom(address, (size_t)quantity, true); }
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop);

    size_t write(uint8_t data) override;
    size_t write(const uint8_t *data, size_t quantity) override;
    int available() override;
    int read() override;
    int peek() override;
    void flush() override;

    void onReceive(void (*cb)(int));
    void onRequest(void (*cb)(void));

    inline size_t write(unsigned long n) { return write((uint8_t)n); }
    inline size_t write(long n) { return write((uint8_t)n); }
    inline size_t write(unsigned int n) { return write((uint8_t)n); }
    inline size_t write(int n) { return write((uint8_t)n); }
    using Print::write;

    void _isr();   /* interno: interrupcion del I2C (modo esclavo) */

private:
    uint8_t _sda, _scl;
    bool _hw;              /* I2C por hardware */
    bool _slave;
    bool _transmitting;
    bool _timeout_flag, _timeout_reset;
    uint32_t _clock, _timeout_us;
    uint8_t _txAddress;
    uint8_t _rxBuf[WIRE_BUFFER_LENGTH];
    uint8_t _txBuf[WIRE_BUFFER_LENGTH];
    volatile uint16_t _rxIndex, _rxLength;
    volatile uint16_t _txLength;
    uint16_t _txIndex;   /* esclavo: siguiente byte a enviar */
    void (*_onReceive)(int);
    void (*_onRequest)(void);

    /* I2C por hardware */
    bool hw_wait(uint32_t event, uint32_t t0);
    uint8_t hw_write(uint8_t addr, const uint8_t *d, uint16_t n, bool stop);
    uint16_t hw_read(uint8_t addr, uint8_t *d, uint16_t n, bool stop);
    void hw_init();
    void hw_recover();
    /* I2C por software */
    uint32_t _half_us;
    void sw_sda(bool high);
    void sw_scl(bool high);
    bool sw_start(bool repeated);
    void sw_stop();
    bool sw_byte_out(uint8_t b);   /* true = ACK */
    uint8_t sw_byte_in(bool ack);
    uint8_t sw_write(uint8_t addr, const uint8_t *d, uint16_t n, bool stop);
    uint16_t sw_read(uint8_t addr, uint8_t *d, uint16_t n, bool stop);
    bool _sw_started;
};

extern TwoWire Wire;

#endif
