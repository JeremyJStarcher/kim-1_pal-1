#include <stdio.h>
#include "MemIoBase.h"

MemIoBase::MemIoBase()
{
}

void MemIoBase::write(uint16_t address, uint8_t value)
{
}

uint8_t MemIoBase::read(uint16_t address)
{
    return 0xEE;
}

bool MemIoBase::inRange(uint16_t x)
{
    bool ret = (x >= this->start_range && x <= this->end_range);

    // printf("%04x:%04x %04x %d\n", this->start_range, this->end_range, x, ret);

    return ret;
}
