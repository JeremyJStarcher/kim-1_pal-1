#include <stdint.h>

#include <emscripten/emscripten.h>
#include <emscripten.h>

#include "serial_display.h"

void serout(uint8_t value)
{
    EM_ASM({
        serialPrint($0);
    },
           value);
}

void serial_scands()
{
}
