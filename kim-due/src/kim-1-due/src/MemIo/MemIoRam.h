#ifndef MEM_IO_RAM_H
#define MEM_IO_RAM_H

#include <stdint.h>
#include "MemIoBase.h"

#ifdef TARGETWEB
#include "../fake-progmen.h"
#include <emscripten/emscripten.h>
#else
#include <avr/pgmspace.h>
#endif

class MemIoRam : public MemIoBase
{
private:
    uint8_t *data;

public:
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    void install(
        uint16_t start_range,
        uint16_t end_range,
        uint8_t data[]);
};

#endif