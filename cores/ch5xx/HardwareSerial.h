/*
 * HardwareSerial.h - UART0..UART3 de los CH58x/CH59x.
 *   Serial1 = UART1 (TX PA9, RX PA8), como el puerto de depuracion de WCH;
 *             Serial es Serial1, o SerialUSB con el menu "Serial = USB CDC"
 *   Serial0 = UART0 (TX PB7, RX PB4)   Serial2 = UART2 (TX PA7, RX PA6)
 *   Serial3 = UART3 (TX PA5, RX PA4)
 */
#ifndef HardwareSerial_h
#define HardwareSerial_h

#include <inttypes.h>
#include "Stream.h"

#define SERIAL_8N1 0x03
#define SERIAL_8N2 0x07
#define SERIAL_8E1 0x1B
#define SERIAL_8O1 0x0B
#define SERIAL_7N1 0x02

#ifndef SERIAL_RX_BUFFER_SIZE
#define SERIAL_RX_BUFFER_SIZE 128
#endif

class HardwareSerial : public Stream {
public:
    HardwareSerial(uint8_t uart, uint8_t rx, uint8_t tx);
    void begin(unsigned long baud, uint8_t config = SERIAL_8N1);
    void end();
    int available() override;
    int peek() override;
    int read() override;
    int availableForWrite() override;
    void flush() override;
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buf, size_t n) override;
    using Print::write;
    operator bool() { return true; }

    void _rx_isr();   /* interno: lo llama la interrupcion de la UART */

private:
    uint8_t _uart, _rx, _tx;
    volatile uint8_t *_reg;
    volatile uint16_t _head, _tail;
    uint8_t _buf[SERIAL_RX_BUFFER_SIZE];
};

extern HardwareSerial Serial1;
extern HardwareSerial Serial0;
extern HardwareSerial Serial2;
extern HardwareSerial Serial3;

extern void serialEventRun(void) __attribute__((weak));

#endif
