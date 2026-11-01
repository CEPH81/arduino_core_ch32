/*
 * syscalls.c - _sbrk() para malloc/printf en las series cuyo SDK no lo trae (CH573).
 * El monton va desde el final de .bss hasta la pila (deja 512 bytes de margen).
 */
#include "Arduino.h"
#include <stddef.h>

#if defined(CH573)
void *_sbrk(ptrdiff_t incr)
{
    extern char _end[];
    extern char _eusrstack[];
    static char *brk = _end;
    char *lim = _eusrstack - 1024;     /* pila (512) + margen */
    if (brk + incr < _end || brk + incr > lim)
        return (void *)-1;
    brk += incr;
    return brk - incr;
}
#endif
