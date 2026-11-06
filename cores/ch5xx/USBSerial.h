/*
 * USBSerial.h - puerto serie por USB (CDC ACM) para los CH57x/CH58x/CH59x.
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * SerialUSB usa el USB de la placa (el mismo conector de la carga por WCH-ISP).
 * Windows 10/11, Linux y macOS lo reconocen sin driver (clase CDC).
 * Con el menu "Serial = USB CDC", Serial es SerialUSB.
 *
 * Endpoints: EP0 control, EP1 IN interrupcion (avisos CDC), EP2 OUT/IN bulk (datos).
 */
#ifndef USBSerial_h
#define USBSerial_h

#include <inttypes.h>
#include "Stream.h"

#ifndef USB_CDC_RX_BUFFER_SIZE
#define USB_CDC_RX_BUFFER_SIZE 256
#endif
#ifndef USB_CDC_TX_BUFFER_SIZE
#define USB_CDC_TX_BUFFER_SIZE 256
#endif
#ifndef USB_VID
#define USB_VID 0x1A86          /* WCH */
#endif
#ifndef USB_PID
#define USB_PID 0xFE0C          /* CDC del nucleo CH5xx de Arduino */
#endif
#ifndef USB_MANUFACTURER
#define USB_MANUFACTURER "WCH"
#endif
#ifndef USB_PRODUCT
#define USB_PRODUCT "CH5xx Arduino"
#endif

class USBSerial : public Stream {
public:
    void begin(unsigned long baud = 115200);   /* el valor se ignora: lo fija el PC */
    void begin(unsigned long baud, uint8_t config) { (void)config; begin(baud); }
    void end();
    int available() override;
    int peek() override;
    int read() override;
    int availableForWrite() override;
    void flush() override;
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buf, size_t n) override;
    using Print::write;

    /* true si hay un programa abierto en el PC (DTR activo) */
    operator bool();
    bool dtr();
    bool rts();
    uint32_t baud();       /* velocidad que puso el PC (solo informativa) */
    uint8_t stopbits();
    uint8_t paritytype();
    uint8_t numbits();
    bool configured();     /* enumerado por el PC */
};

extern USBSerial SerialUSB;

#endif
