/*
 * HardwareSerial.cpp - puertos serie con recepcion por interrupcion y FIFO de envio.
 * Registros de cada UART (base 0x40003000 + 0x400 * n): MCR +0, IER +1, FCR +2, LCR +3,
 * IIR +4, LSR +5, RBR/THR +8, RFC +10, TFC +11, DL +12 (16 bits), DIV +14.
 */
#include "Arduino.h"

enum { MCR = 0, IER = 1, FCR = 2, LCR = 3, IIR = 4, LSR = 5, RBR = 8, THR = 8, RFC = 10, TFC = 11, DLL = 12, DLM = 13, DIV = 14 };

static HardwareSerial *uarts[4];

HardwareSerial::HardwareSerial(uint8_t uart, uint8_t rx, uint8_t tx)
    : _uart(uart), _rx(rx), _tx(tx), _head(0), _tail(0)
{
    _reg = (volatile uint8_t *)(0x40003000UL + 0x400UL * uart);
}

static const IRQn_Type irq[4] = {UART0_IRQn, UART1_IRQn, UART2_IRQn, UART3_IRQn};

void HardwareSerial::begin(unsigned long baud, uint8_t config)
{
    uarts[_uart] = this;
    digitalWrite(_tx, HIGH);
    pinMode(_rx, INPUT_PULLUP);
    pinMode(_tx, OUTPUT);
    uint32_t x = 10 * GetSysClock() / 8 / baud;
    x = (x + 5) / 10;
    _reg[DIV] = 1;
    *(volatile uint16_t *)(_reg + DLL) = (uint16_t)x;
    _reg[FCR] = (2 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;  /* aviso con 4 bytes */
    _reg[LCR] = config;
    _head = _tail = 0;
    _reg[IER] = RB_IER_TXD_EN | RB_IER_RECV_RDY | RB_IER_LINE_STAT;
    _reg[MCR] |= RB_MCR_INT_OE;
    PFIC_EnableIRQ(irq[_uart]);
}

void HardwareSerial::end()
{
    flush();
    PFIC_DisableIRQ(irq[_uart]);
    _reg[IER] = 0;
}

void HardwareSerial::_rx_isr()
{
    uint8_t iir = _reg[IIR] & RB_IIR_INT_MASK;
    if (iir == UART_II_LINE_STAT) {
        (void)_reg[LSR];
        return;
    }
    while (_reg[RFC]) {
        uint8_t c = _reg[RBR];
        uint16_t n = (_head + 1) % SERIAL_RX_BUFFER_SIZE;
        if (n != _tail) {
            _buf[_head] = c;
            _head = n;
        }
    }
}

int HardwareSerial::available()
{
    return (SERIAL_RX_BUFFER_SIZE + _head - _tail) % SERIAL_RX_BUFFER_SIZE;
}

int HardwareSerial::peek()
{
    return _head == _tail ? -1 : _buf[_tail];
}

int HardwareSerial::read()
{
    if (_head == _tail) return -1;
    uint8_t c = _buf[_tail];
    _tail = (_tail + 1) % SERIAL_RX_BUFFER_SIZE;
    return c;
}

int HardwareSerial::availableForWrite()
{
    return 8 - _reg[TFC];
}

size_t HardwareSerial::write(uint8_t c)
{
    while (_reg[TFC] >= 8)
        ;
    _reg[THR] = c;
    return 1;
}

size_t HardwareSerial::write(const uint8_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++) write(buf[i]);
    return n;
}

void HardwareSerial::flush()
{
    while (!(_reg[LSR] & RB_LSR_TX_ALL_EMP))
        ;
}

HardwareSerial Serial1(1, PIN_SERIAL1_RX, PIN_SERIAL1_TX);
HardwareSerial Serial0(0, PIN_SERIAL0_RX, PIN_SERIAL0_TX);
HardwareSerial Serial2(2, PIN_SERIAL2_RX, PIN_SERIAL2_TX);
HardwareSerial Serial3(3, PIN_SERIAL3_RX, PIN_SERIAL3_TX);

extern "C" {
__INTERRUPT __HIGH_CODE void UART0_IRQHandler(void) { if (uarts[0]) uarts[0]->_rx_isr(); }
__INTERRUPT __HIGH_CODE void UART1_IRQHandler(void) { if (uarts[1]) uarts[1]->_rx_isr(); }
__INTERRUPT __HIGH_CODE void UART2_IRQHandler(void) { if (uarts[2]) uarts[2]->_rx_isr(); }
__INTERRUPT __HIGH_CODE void UART3_IRQHandler(void) { if (uarts[3]) uarts[3]->_rx_isr(); }
}

/* serialEvent(): si el sketch la define, se llama despues de cada loop() */
void serialEvent() __attribute__((weak));
void serialEvent0() __attribute__((weak));
void serialEvent1() __attribute__((weak));
void serialEvent2() __attribute__((weak));
void serialEvent3() __attribute__((weak));

void serialEventRun(void)
{
    if (serialEvent && Serial.available()) serialEvent();   /* Serial1 o SerialUSB */
    if (serialEvent1 && Serial1.available()) serialEvent1();
    if (serialEvent0 && Serial0.available()) serialEvent0();
    if (serialEvent2 && Serial2.available()) serialEvent2();
    if (serialEvent3 && Serial3.available()) serialEvent3();
}
