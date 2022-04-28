#include <stdio.h>
#include <stdlib.h>

#include "MemIoRom.h"
uint8_t MemIoRom::read(uint16_t address)
{
    unsigned int offset = address - this->start_range;

#ifdef __AVR__
    uint8_t ret = (pgm_read_byte_near(this->data + offset));
    return ret;
#else
    return this->data[offset];
#endif
}

void MemIoRom::install(
    uint16_t start_range,
    uint16_t end_range,
    const uint8_t data[] PROGMEM)
{
    this->start_range = start_range;
    this->end_range = end_range;

    this->data = data;
}
