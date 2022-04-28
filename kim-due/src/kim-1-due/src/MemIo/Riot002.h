#ifndef MEM_IO_RIOT002_H
#define MEM_IO_RIOT002_H

#include <stdint.h>
#include "MemIoBase.h"

class MemIoRiot002 : public MemIoBase
{
private:
    uint8_t ioPAD;      // Port A data register
    uint8_t ioPADD;     // Port A data direction register
    uint8_t ioPBD;      // port B data register
    uint8_t ioPBDD;     // Port B data direction register
    void processIoChange(void);

public:
    MemIoRiot002(void);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};

#endif
