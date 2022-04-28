#ifndef MEM_IO_H
#define MEM_IO_H

#include <stdint.h>
#include <stdlib.h>
#include "MemIoBase.h"

#define MEM_IO_LIST_SIZE 10
class MemIo
{
private:
   MemIoBase ranges[MEM_IO_LIST_SIZE];

public:
    MemIo(void);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};

#endif
