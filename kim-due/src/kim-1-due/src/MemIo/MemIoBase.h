#ifndef MEM_IO_BASE_H
#define MEM_IO_BASE_H

#include <stdint.h>

class MemIoBase
{
public:
    // Public for speed.
    uint16_t start_range;
    uint16_t end_range;

    MemIoBase();
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    bool inRange(uint16_t x);
};

#endif
