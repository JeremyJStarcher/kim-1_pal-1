#ifndef MEM_RIOT_TIMER_H
#define MEM_RIOT_TIMER_H

#include <stdint.h>
#include "MemIoBase.h"

#define DIV1 0
#define DIV8 3
#define DIV64 6
#define DIV1024 10

class MemRiotTimer : public MemIoBase
{
private:
    uint8_t timerMode;
    uint16_t baseAddress;

public:
    // This is public for speed
    int32_t timer;

    MemRiotTimer(void);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    void install(
        uint16_t start_range,
        uint16_t end_range,
        uint16_t base_address);
};

#endif
