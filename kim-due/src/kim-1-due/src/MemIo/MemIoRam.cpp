#include <stdio.h>
#include <stdlib.h>

#include "MemIoRam.h"
uint8_t MemIoRam::read(uint16_t address)
{
    unsigned int offset = address - this->start_range;
    return this->data[offset];
}

void MemIoRam::write(uint16_t address, uint8_t value)
{
    unsigned int offset = address - this->start_range;
    this->data[offset] = value;
}

void MemIoRam::install(
    uint16_t start_range,
    uint16_t end_range,
    uint8_t data[])
{
    this->start_range = start_range;
    this->end_range = end_range;

    size_t len = end_range - start_range;

    // printf("ram install: %04x:%04x 0x(%04x) %ld\n", start_range, end_range, len, len);

    this->data = data;
}
