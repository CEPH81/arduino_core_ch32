/*
 * main.cpp - arranque del programa Arduino en los CH58x/CH59x.
 */
#include "Arduino.h"

extern "C" {
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
}

static void constructores(void)
{
    for (void (**f)(void) = __init_array_start; f < __init_array_end; f++)
        (*f)();
}

int main(void)
{
    init();
    constructores();       /* objetos globales de C++ (Serial...) */
    initVariant();
    setup();
    for (;;) {
        loop();
        serialEventRun();
    }
    return 0;
}
