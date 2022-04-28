#ifndef CPU_INCLUDE_H
#define CPU_INCLUDE_H

#ifndef TARGETWEB
#include "Arduino.h"
#else
#include <emscripten/emscripten.h>
#endif

#include <stdint.h>

#include "boardhardware.h"

uint16_t getpc();
uint8_t getop();
void exec6502(int32_t tickcount);
void reset6502();
void nmi6502();
void initKIM(void);
void loadTestProgram(void);
extern void driveLEDs();
void nmi6502(void);

extern "C"
{
    uint16_t debuggetaddress(void);
}

extern uint8_t RAM[ONBOARD_RAM]; // main 1KB RAM		 0x000-0x3FF
extern uint16_t pc;              // 6502 pc
extern uint8_t SSTmode;

#endif