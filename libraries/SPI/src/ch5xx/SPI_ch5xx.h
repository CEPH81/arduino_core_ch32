/*
 * SPI_ch5xx.h - biblioteca SPI para los WCH CH58x, CH59x y CH57x (SPI0, maestro).
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * Patas de SPI0: SCK PA13, MOSI PA14, MISO PA15 (SS PA12, lo maneja el sketch)
 *            o, remapeado con SPI.setPins(PB13, PB15, PB14): SCK PB13, MISO PB15, MOSI PB14.
 * El SPI0 por hardware hace los modos 0 y 3; los modos 1 y 2 se hacen por software.
 * Reloj: Fsys / divisor (2..255); a 60 MHz, hasta 30 MHz.
 */
#ifndef SPI_ch5xx_h
#define SPI_ch5xx_h

#include "Arduino.h"

#define SPI_HAS_TRANSACTION 1
#define SPI_HAS_NOTUSINGINTERRUPT 1

#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

/* divisores clasicos de AVR (referidos a 16 MHz, como en otros nucleos) */
#define SPI_CLOCK_DIV2   2
#define SPI_CLOCK_DIV4   4
#define SPI_CLOCK_DIV8   8
#define SPI_CLOCK_DIV16  16
#define SPI_CLOCK_DIV32  32
#define SPI_CLOCK_DIV64  64
#define SPI_CLOCK_DIV128 128

class SPISettings {
public:
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
        : _clock(clock), _bitOrder(bitOrder), _dataMode(dataMode) {}
    SPISettings() : _clock(4000000), _bitOrder(MSBFIRST), _dataMode(SPI_MODE0) {}
private:
    uint32_t _clock;
    uint8_t _bitOrder;
    uint8_t _dataMode;
    friend class SPIClass;
};

class SPIClass {
public:
    SPIClass();
    bool setPins(uint8_t sck, uint8_t miso, uint8_t mosi);   /* antes de begin() */
    void setSCLK(uint8_t p) { _sck = p; }
    void setMISO(uint8_t p) { _miso = p; }
    void setMOSI(uint8_t p) { _mosi = p; }
    void setSSEL(uint8_t p) { (void)p; }   /* SS lo maneja el sketch */

    void begin();
    void end();

    void beginTransaction(SPISettings settings);
    void endTransaction();

    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    void transfer(void *buf, size_t count);                      /* in situ */
    void transfer(const void *txbuf, void *rxbuf, size_t count); /* rxbuf puede ser NULL */

    void setBitOrder(uint8_t order);
    void setDataMode(uint8_t mode);
    void setClockDivider(uint8_t div);   /* respecto a 16 MHz, como AVR */
    void setFrequency(uint32_t hz);

    void usingInterrupt(int) {}
    void notUsingInterrupt(int) {}
    void attachInterrupt() {}
    void detachInterrupt() {}

private:
    uint8_t _sck, _miso, _mosi;
    uint8_t _mode, _order;
    uint32_t _clock;
    bool _hw;        /* SPI0 por hardware (modos 0 y 3, patas de SPI0) */
    bool _begun;
    uint32_t _half_us;
    void apply();
    uint8_t sw_transfer(uint8_t b);
};

extern SPIClass SPI;

#endif
