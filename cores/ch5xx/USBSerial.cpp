/*
 * USBSerial.cpp - CDC ACM sobre el USB de dispositivo de los CH57x/CH58x/CH59x.
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * El controlador USB de WCH (igual en CH573, CH583 y CH592) trabaja por DMA:
 * cada endpoint tiene un bufer en RAM (R16_UEPx_DMA), una longitud de envio
 * (R8_UEPx_T_LEN) y un registro de respuesta (R8_UEPx_CTRL: ACK/NAK/STALL y
 * bits de alternancia DATA0/DATA1). Una sola interrupcion (USB_IRQHandler)
 * avisa de SETUP/IN/OUT, reinicio del bus y suspension.
 */
#include "Arduino.h"
#include "USBSerial.h"

/* ------------------------------------------------------------ descriptores */
#define EP0_SIZE 64
#define CDC_DATA_SIZE 64

static const uint8_t dev_descr[18] = {
    18, 0x01, 0x00, 0x02,          /* USB 2.0 */
    0xEF, 0x02, 0x01,              /* clase miscelanea con IAD */
    EP0_SIZE,
    USB_VID & 0xFF, USB_VID >> 8,
    USB_PID & 0xFF, USB_PID >> 8,
    0x00, 0x01,                    /* version 1.00 */
    1, 2, 3,                       /* fabricante, producto, numero de serie */
    1
};

static const uint8_t cfg_descr[75] = {
    9, 0x02, 75, 0, 2, 1, 0, 0x80, 50,          /* configuracion: 2 interfaces, 100 mA */
    8, 0x0B, 0, 2, 0x02, 0x02, 0x01, 0,          /* IAD: interfaces 0-1, CDC ACM */
    9, 0x04, 0, 0, 1, 0x02, 0x02, 0x01, 0,       /* interfaz 0: comunicaciones */
    5, 0x24, 0x00, 0x10, 0x01,                   /* cabecera CDC 1.10 */
    5, 0x24, 0x01, 0x00, 0x01,                   /* gestion de llamadas */
    4, 0x24, 0x02, 0x02,                         /* ACM: line coding y line state */
    5, 0x24, 0x06, 0x00, 0x01,                   /* union: control 0, datos 1 */
    7, 0x05, 0x81, 0x03, 8, 0, 16,               /* EP1 IN, interrupcion, 8 bytes */
    9, 0x04, 1, 0, 2, 0x0A, 0x00, 0x00, 0,       /* interfaz 1: datos */
    7, 0x05, 0x02, 0x02, CDC_DATA_SIZE, 0, 0,    /* EP2 OUT bulk */
    7, 0x05, 0x82, 0x02, CDC_DATA_SIZE, 0, 0     /* EP2 IN bulk */
};

static const uint8_t lang_descr[4] = {4, 0x03, 0x09, 0x04};
static uint8_t str_buf[2 + 2 * 32];   /* descriptor de cadena armado al pedirlo */

/* ------------------------------------------------------------ bufer de los endpoints */
/* EP0 (64) + EP4 OUT (64) + EP4 IN (64), como espera USB_DeviceInit() del SDK */
static __attribute__((aligned(4))) uint8_t ep0_ram[64 + 64 + 64];
static __attribute__((aligned(4))) uint8_t ep1_ram[64 + 64];
static __attribute__((aligned(4))) uint8_t ep2_ram[64 + 64];
static __attribute__((aligned(4))) uint8_t ep3_ram[64 + 64];
#define EP1_IN  (ep1_ram + 64)
#define EP2_OUT (ep2_ram)
#define EP2_IN  (ep2_ram + 64)

/* ------------------------------------------------------------ estado */
static volatile bool usb_started, usb_configured;
static volatile uint8_t setup_req, setup_type, set_addr;
static volatile uint16_t setup_len;
static const uint8_t *ep0_src;
static volatile uint8_t line_state;                 /* bit0 DTR, bit1 RTS */
static uint8_t line_coding[7] = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8};   /* 115200 8N1 */

static uint8_t rx_buf[USB_CDC_RX_BUFFER_SIZE];
static volatile uint16_t rx_head, rx_tail;
static volatile bool rx_paused;                     /* EP2 OUT en NAK por bufer lleno */
static uint8_t tx_buf[USB_CDC_TX_BUFFER_SIZE];
static volatile uint16_t tx_head, tx_tail;
static volatile bool tx_busy;
static volatile uint8_t tx_last;                    /* longitud del ultimo paquete enviado */

static uint16_t rx_count(void) { return (uint16_t)((rx_head - rx_tail + USB_CDC_RX_BUFFER_SIZE) % USB_CDC_RX_BUFFER_SIZE); }
static uint16_t rx_free(void) { return (uint16_t)(USB_CDC_RX_BUFFER_SIZE - 1 - rx_count()); }
static uint16_t tx_count(void) { return (uint16_t)((tx_head - tx_tail + USB_CDC_TX_BUFFER_SIZE) % USB_CDC_TX_BUFFER_SIZE); }

static inline void ep2_out_res(uint8_t res) { R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_R_RES) | res; }

/* Envia el siguiente paquete de tx_buf por EP2 IN (con la interrupcion USB parada) */
static void tx_kick(void)
{
    uint8_t n = 0;
    if (tx_busy || !usb_configured) return;
    while (n < CDC_DATA_SIZE && tx_tail != tx_head) {
        EP2_IN[n++] = tx_buf[tx_tail];
        tx_tail = (uint16_t)((tx_tail + 1) % USB_CDC_TX_BUFFER_SIZE);
    }
    /* un paquete lleno sin mas datos se cierra con uno vacio (ZLP) */
    if (n == 0 && tx_last != CDC_DATA_SIZE) return;
    tx_last = n;
    tx_busy = true;
    R8_UEP2_T_LEN = n;
    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
}

static uint8_t make_string(uint8_t idx)
{
    char tmp[17];
    const char *s;
    uint8_t n = 0;
    if (idx == 1) s = USB_MANUFACTURER;
    else if (idx == 2) s = USB_PRODUCT;
    else {
        /* numero de serie: ID unico de 64 bits del chip en hexadecimal */
        uint8_t id[8];
        static const char hx[] = "0123456789ABCDEF";
        GET_UNIQUE_ID(id);
        for (uint8_t i = 0; i < 8; i++) {
            tmp[2 * i] = hx[id[i] >> 4];
            tmp[2 * i + 1] = hx[id[i] & 15];
        }
        tmp[16] = 0;
        s = tmp;
    }
    while (s[n] && n < 32) {
        str_buf[2 + 2 * n] = (uint8_t)s[n];
        str_buf[3 + 2 * n] = 0;
        n++;
    }
    str_buf[0] = (uint8_t)(2 + 2 * n);
    str_buf[1] = 0x03;
    return str_buf[0];
}

/* Siguiente trozo de la fase de datos IN de EP0 */
static void ep0_send_chunk(void)
{
    uint8_t n = setup_len > EP0_SIZE ? EP0_SIZE : (uint8_t)setup_len;
    memcpy(ep0_ram, ep0_src, n);
    ep0_src += n;
    setup_len = (uint16_t)(setup_len - n);
    R8_UEP0_T_LEN = n;
}

static void handle_setup(void)
{
    PUSB_SETUP_REQ req = (PUSB_SETUP_REQ)ep0_ram;
    uint16_t len = req->wLength, value = req->wValue;
    const uint8_t *src = NULL;
    uint16_t avail = 0;
    bool err = false, data_out = false;

    setup_type = req->bRequestType;
    setup_req = req->bRequest;

    if ((setup_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS) {
        switch (setup_req) {
        case 0x20:                    /* SET_LINE_CODING: 7 bytes en la fase OUT */
            data_out = true;
            break;
        case 0x21:                    /* GET_LINE_CODING */
            src = line_coding;
            avail = 7;
            break;
        case 0x22:                    /* SET_CONTROL_LINE_STATE */
            line_state = (uint8_t)value;
            break;
        case 0x23:                    /* SEND_BREAK: se acepta y se ignora */
            break;
        default:
            err = true;
        }
    } else if ((setup_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD) {
        switch (setup_req) {
        case USB_GET_DESCRIPTOR:
            switch (value >> 8) {
            case USB_DESCR_TYP_DEVICE: src = dev_descr; avail = sizeof dev_descr; break;
            case USB_DESCR_TYP_CONFIG: src = cfg_descr; avail = sizeof cfg_descr; break;
            case USB_DESCR_TYP_STRING:
                if ((value & 0xFF) == 0) { src = lang_descr; avail = sizeof lang_descr; }
                else if ((value & 0xFF) <= 3) { avail = make_string((uint8_t)value); src = str_buf; }
                else err = true;
                break;
            default: err = true;      /* calificador de dispositivo, etc.: no es de alta velocidad */
            }
            break;
        case USB_SET_ADDRESS:
            set_addr = (uint8_t)value;
            break;
        case USB_GET_CONFIGURATION:
            ep0_ram[0] = usb_configured ? 1 : 0;
            src = ep0_ram;
            avail = 1;
            break;
        case USB_SET_CONFIGURATION:
            usb_configured = (value & 0xFF) != 0;
            if (usb_configured) {
                R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
                R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
                tx_busy = false;
                tx_last = 0;
                rx_paused = false;
            }
            break;
        case USB_GET_INTERFACE:
            ep0_ram[0] = 0;
            src = ep0_ram;
            avail = 1;
            break;
        case USB_SET_INTERFACE:
            break;
        case USB_GET_STATUS:
            ep0_ram[0] = 0;
            ep0_ram[1] = 0;
            src = ep0_ram;
            avail = 2;
            break;
        case USB_CLEAR_FEATURE:
            if ((setup_type & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
                switch (req->wIndex & 0xFF) {
                case 0x82: R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; tx_busy = false; break;
                case 0x02: R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK; break;
                case 0x81: R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; break;
                default: err = true;
                }
            }
            break;
        case USB_SET_FEATURE:
            break;
        default:
            err = true;
        }
    } else {
        err = true;
    }

    if (err) {
        setup_req = 0xFF;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
        return;
    }
    if (data_out) {
        /* esperar la fase de datos OUT */
        R8_UEP0_T_LEN = 0;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        return;
    }
    if (src && (setup_type & USB_REQ_TYP_IN)) {
        /* fase de datos IN */
        if (src == ep0_ram) {
            /* dato armado en el propio bufer: copia antes de que se sobrescriba */
            static uint8_t small[2];
            memcpy(small, ep0_ram, 2);
            src = small;
        }
        ep0_src = src;
        setup_len = len < avail ? len : avail;
        ep0_send_chunk();
    } else {
        /* sin datos: estado con paquete vacio (DATA1) */
        setup_len = 0;
        R8_UEP0_T_LEN = 0;
    }
    R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
}

extern "C" __INTERRUPT __HIGH_CODE void USB_IRQHandler(void)
{
    uint8_t fg = R8_USB_INT_FG;

    if (fg & RB_UIF_TRANSFER) {
        uint8_t st = R8_USB_INT_ST;
        uint8_t token = st & MASK_UIS_TOKEN, ep = st & MASK_UIS_ENDP;
        if (token != MASK_UIS_TOKEN) {   /* no es reposo */
            switch (token | ep) {
            case UIS_TOKEN_SETUP | 0:
                handle_setup();
                break;
            case UIS_TOKEN_IN | 0:
                if (setup_req == USB_SET_ADDRESS && (setup_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD) {
                    R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | set_addr;
                    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    setup_req = 0xFF;
                } else if (setup_len) {
                    ep0_send_chunk();
                    R8_UEP0_CTRL ^= RB_UEP_T_TOG;
                } else {
                    R8_UEP0_T_LEN = 0;
                    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                }
                break;
            case UIS_TOKEN_OUT | 0:
                if ((st & RB_UIS_TOG_OK) && setup_req == 0x20 &&
                    (setup_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS) {
                    memcpy(line_coding, ep0_ram, 7);
                    setup_req = 0xFF;
                    R8_UEP0_T_LEN = 0;
                    R8_UEP0_CTRL = RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;   /* estado: IN vacio DATA1 */
                } else {
                    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                  /* fin de la fase de estado */
                }
                break;
            case UIS_TOKEN_OUT | 2:
                if (st & RB_UIS_TOG_OK) {
                    uint8_t n = R8_USB_RX_LEN;
                    for (uint8_t i = 0; i < n; i++) {
                        uint16_t h = (uint16_t)((rx_head + 1) % USB_CDC_RX_BUFFER_SIZE);
                        if (h == rx_tail) break;   /* no deberia pasar: hay control de flujo */
                        rx_buf[rx_head] = EP2_OUT[i];
                        rx_head = h;
                    }
                    /* sin sitio para otro paquete: NAK hasta que el sketch lea */
                    if (rx_free() < CDC_DATA_SIZE) {
                        rx_paused = true;
                        ep2_out_res(UEP_R_RES_NAK);
                    }
                }
                break;
            case UIS_TOKEN_IN | 2:
                R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                tx_busy = false;
                tx_kick();
                break;
            case UIS_TOKEN_IN | 1:
                R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                break;
            default:
                break;
            }
        }
        R8_USB_INT_FG = RB_UIF_TRANSFER;
    }
    if (fg & RB_UIF_BUS_RST) {
        R8_USB_DEV_AD = 0;
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
        R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
        R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
        usb_configured = false;
        line_state = 0;
        tx_busy = false;
        R8_USB_INT_FG = RB_UIF_BUS_RST;
    }
    if (fg & RB_UIF_SUSPEND) {
        R8_USB_INT_FG = RB_UIF_SUSPEND;
    }
}

/* ------------------------------------------------------------ API */
void USBSerial::begin(unsigned long)
{
    if (usb_started) return;
    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;
    pEP0_RAM_Addr = ep0_ram;
    pEP1_RAM_Addr = ep1_ram;
    pEP2_RAM_Addr = ep2_ram;
    pEP3_RAM_Addr = ep3_ram;
    USB_DeviceInit();
    PFIC_EnableIRQ(USB_IRQn);
    usb_started = true;
}

void USBSerial::end()
{
    PFIC_DisableIRQ(USB_IRQn);
    R8_USB_CTRL = RB_UC_CLR_ALL | RB_UC_RESET_SIE;
    R8_USB_CTRL = 0;
    R8_UDEV_CTRL = 0;
    usb_started = false;
    usb_configured = false;
}

int USBSerial::available() { return rx_count(); }

int USBSerial::peek()
{
    return rx_head == rx_tail ? -1 : rx_buf[rx_tail];
}

int USBSerial::read()
{
    uint8_t c;
    if (rx_head == rx_tail) return -1;
    c = rx_buf[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1) % USB_CDC_RX_BUFFER_SIZE);
    if (rx_paused && rx_free() >= CDC_DATA_SIZE) {
        PFIC_DisableIRQ(USB_IRQn);
        rx_paused = false;
        ep2_out_res(UEP_R_RES_ACK);
        PFIC_EnableIRQ(USB_IRQn);
    }
    return c;
}

int USBSerial::availableForWrite()
{
    return USB_CDC_TX_BUFFER_SIZE - 1 - tx_count();
}

void USBSerial::flush()
{
    uint32_t t0 = millis();
    while ((tx_count() || tx_busy) && usb_configured && (line_state & 1)) {
        PFIC_DisableIRQ(USB_IRQn);
        tx_kick();
        PFIC_EnableIRQ(USB_IRQn);
        if (millis() - t0 > 100) break;
    }
}

size_t USBSerial::write(uint8_t c)
{
    return write(&c, 1);
}

size_t USBSerial::write(const uint8_t *buf, size_t n)
{
    size_t done = 0;
    uint32_t t0 = millis();
    /* sin PC conectado no se bloquea: se descarta */
    if (!usb_configured) return 0;
    while (done < n) {
        uint16_t h = (uint16_t)((tx_head + 1) % USB_CDC_TX_BUFFER_SIZE);
        if (h == tx_tail) {
            /* bufer lleno: esperar al PC (hasta 50 ms si nadie lee) */
            PFIC_DisableIRQ(USB_IRQn);
            tx_kick();
            PFIC_EnableIRQ(USB_IRQn);
            /* sin programa abierto en el PC (DTR) no se espera */
            if (!usb_configured || !(line_state & 1) || millis() - t0 > 50) break;
            continue;
        }
        tx_buf[tx_head] = buf[done++];
        tx_head = h;
        t0 = millis();
    }
    PFIC_DisableIRQ(USB_IRQn);
    tx_kick();
    PFIC_EnableIRQ(USB_IRQn);
    return done;
}

USBSerial::operator bool() { return usb_configured && (line_state & 1); }
bool USBSerial::dtr() { return line_state & 1; }
bool USBSerial::rts() { return (line_state >> 1) & 1; }
bool USBSerial::configured() { return usb_configured; }
uint32_t USBSerial::baud()
{
    return (uint32_t)line_coding[0] | ((uint32_t)line_coding[1] << 8) | ((uint32_t)line_coding[2] << 16) |
           ((uint32_t)line_coding[3] << 24);
}
uint8_t USBSerial::stopbits() { return line_coding[4]; }
uint8_t USBSerial::paritytype() { return line_coding[5]; }
uint8_t USBSerial::numbits() { return line_coding[6]; }

USBSerial SerialUSB;
