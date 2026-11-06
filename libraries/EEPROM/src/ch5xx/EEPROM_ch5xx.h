/*
 * EEPROM_ch5xx.h - EEPROM en la Data-Flash de los WCH CH57x/CH58x/CH59x.
 * Parte de arduino_core_ch32 (fork de CEPH81 - MikrocontrollerCephs). LGPL 2.1.
 *
 * Misma API que la EEPROM de los CH32 de este paquete: se trabaja sobre una copia
 * en RAM y commit() la graba. La Data-Flash tiene 32 KB (paginas de 256 bytes,
 * unas 100 000 grabaciones); se usan los primeros EEPROM_SIZE bytes
 * (por defecto 1024; el final de la Data-Flash lo usa la pila BLE de WCH).
 */
#ifndef EEPROM_ch5xx_h
#define EEPROM_ch5xx_h

#include <Arduino.h>

#ifndef EEPROM_SIZE
#define EEPROM_SIZE 1024
#endif

class EEPROMClass {
public:
    EEPROMClass(void) {}
    ~EEPROMClass(void) { end(); }

    void begin(size_t size = EEPROM_SIZE);
    uint8_t *getDataPtr() { _dirty = true; return _data; }
    uint8_t const *getConstDataPtr() const { return _data; }
    uint32_t ReadOptionBytes(void) { return 0; }   /* solo en los CH32 */

    uint8_t read(int const idx);
    void write(int const idx, uint8_t const val);   /* requiere commit() */
    void update(int const idx, uint8_t const val) { write(idx, val); }
    void erase(void);                               /* requiere commit() */
    bool commit(void);
    bool end(void);

    template <typename T> T &get(int const address, T &t)
    {
        if (!_data || address < 0 || address + sizeof(T) > _size) return t;
        memcpy((uint8_t *)&t, _data + address, sizeof(T));
        return t;
    }
    template <typename T> const T &put(int const address, const T &t)
    {
        if (!_data || address < 0 || address + sizeof(T) > _size) return t;
        if (memcmp(_data + address, (const uint8_t *)&t, sizeof(T)) != 0) {
            _dirty = true;
            memcpy(_data + address, (const uint8_t *)&t, sizeof(T));
        }
        return t;
    }

    size_t length() { return _size; }
    uint8_t &operator[](int const address) { return getDataPtr()[address]; }
    uint8_t const &operator[](int const address) const { return getConstDataPtr()[address]; }

protected:
    uint8_t *_data = nullptr;
    bool _dirty = false;
    size_t _size = 0;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
extern EEPROMClass EEPROM;
#endif

#endif
