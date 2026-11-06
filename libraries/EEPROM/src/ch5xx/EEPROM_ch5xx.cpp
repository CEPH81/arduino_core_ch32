/*
 * EEPROM_ch5xx.cpp - EEPROM en la Data-Flash de los WCH CH57x/CH58x/CH59x
 * con EEPROM_READ / EEPROM_ERASE / EEPROM_WRITE de la ROM de WCH (libISP5xx).
 */
#if defined(CH58x) || defined(CH59x) || defined(CH573)

#include "EEPROM_ch5xx.h"

void EEPROMClass::begin(size_t size)
{
    if (size == 0) size = EEPROM_SIZE;
    /* multiplo de la pagina de borrado (256 bytes) */
    size = (size + EEPROM_PAGE_SIZE - 1) / EEPROM_PAGE_SIZE * EEPROM_PAGE_SIZE;
    if (size > 0x4000) size = 0x4000;   /* deja libre el final para BLE */
    if (_data && _size != size) end();
    if (!_data) {
        _data = (uint8_t *)malloc(size);
        if (!_data) { _size = 0; return; }
    }
    _size = size;
    noInterrupts();
    EEPROM_READ(0, _data, _size);
    interrupts();
    _dirty = false;
}

uint8_t EEPROMClass::read(int const idx)
{
    if (!_data || idx < 0 || (size_t)idx >= _size) return 0xFF;
    return _data[idx];
}

void EEPROMClass::write(int const idx, uint8_t const val)
{
    if (!_data || idx < 0 || (size_t)idx >= _size) return;
    if (_data[idx] != val) {
        _data[idx] = val;
        _dirty = true;
    }
}

void EEPROMClass::erase(void)
{
    if (!_data) return;
    memset(_data, 0xFF, _size);
    _dirty = true;
}

bool EEPROMClass::commit(void)
{
    uint32_t r;
    if (!_data) return false;
    if (!_dirty) return true;
    /* la ROM de WCH no admite interrupciones mientras graba la flash */
    noInterrupts();
    r = EEPROM_ERASE(0, _size);
    if (r == 0) r = EEPROM_WRITE(0, _data, _size);
    interrupts();
    if (r == 0) _dirty = false;
    return r == 0;
}

bool EEPROMClass::end(void)
{
    bool ok = commit();
    if (_data) free(_data);
    _data = nullptr;
    _size = 0;
    return ok;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
EEPROMClass EEPROM;
#endif

#endif
