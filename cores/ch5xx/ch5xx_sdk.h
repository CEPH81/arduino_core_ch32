/*
 * ch5xx_sdk.h - SDK de WCH (openwch/ch583, openwch/ch592) para el nucleo Arduino.
 * Series: CH58x (CH582, CH583), CH59x (CH591, CH592) y CH573 (CH571, CH573).
 * Las define boards.txt. SDK de openwch y, para el CH573, el de WeAct Studio.
 */
#ifndef CH5XX_SDK_H
#define CH5XX_SDK_H

#if defined(CH58x)
#include "CH58x_common.h"
#elif defined(CH59x)
#include "CH59x_common.h"
#elif defined(CH573)
#include "CH57x_common.h"
#else
#error "Serie no soportada: defina CH58x, CH59x o CH573"
#endif

/* el SDK del CH573 no trae estas macros */
#ifndef __INTERRUPT
#define __INTERRUPT __attribute__((interrupt("WCH-Interrupt-fast")))
#endif
#ifndef __HIGH_CODE
#define __HIGH_CODE __attribute__((section(".highcode")))
#endif

/* el SDK define min/max como macros: en C++ chocan con std::min/max */
#ifdef __cplusplus
#undef min
#undef max
#endif

#endif
