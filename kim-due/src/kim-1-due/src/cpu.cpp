/* KIM-I emulator for Arduino.
   Most of this is the 6502 emulator from Mike Chambers,
   http://forum.arduino.cc/index.php?topic=193216.0
   KIM-I hacked in by Oscar
   http://obsolescenceguaranteed.blogspot.ch/
   Tidied up by Mark VandeWettering to make it compile with platformio.
*/
#define USE_TIMING

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef TARGETWEB
#include "fake-progmen.h"
#include <emscripten/emscripten.h>
#else
#include <avr/pgmspace.h>
#endif

#include "MemIo/MemIo.h"
#include "MemIo/MemIoRam.h"
#include "MemIo/MemIoRom.h"
#include "MemIo/Riot002.h"
#include "MemIo/RiotTimer.h"

#include "roms/cassette.h"
#include "roms/monitor.h"
#include "roms/astroid.h"
#include "roms/uchess7.h"
#include "cpu.h"
#include "boardhardware.h"
#include "led_driver.h"
#include "serial_display.h"
#include "host-hardware.h"

// Option 1:
//   Emulate all of the keyboard handling by hijacking the keyboard ROM
//   routines and replacing them with native code and return the right
//   values back.  This is the absolute fastest approach.
// Option 2:
//   Mimic the RIOT chip and play with the ports. The ROM will see it
//   like real hardware.
//   MUCH slower but slightly more compatible.

MemIo *memio = new MemIo();
MemIoRom *rom1 = new MemIoRom();
MemIoRom *rom2 = new MemIoRom();
MemIoRom *romuchess7 = new MemIoRom();
MemIoRiot002 *riotIo002 = new MemIoRiot002();
MemIoRam *ramMain = new MemIoRam();
MemIoRam *ramRiot002 = new MemIoRam();
MemIoRam *ramRiot003 = new MemIoRam();
MemRiotTimer *timer003 = new MemRiotTimer();

static uint16_t last_read_address;

void write6502(uint16_t address, uint8_t value);
uint8_t read6502(uint16_t address);

void getBin(int num, char *str)
{
    *(str + 5) = '\0';
    int mask = 0x10 << 1;
    while (mask >>= 1)
    {
        *str++ = !!(mask & num) + '0';
    }
}

extern char threeHex[3][2]; // buffer for 3 hex digits

extern uint8_t getAkey(void); // for serial port get normal ASCII keys

uint8_t iii;         // counter for various purposes, declared here to avoid in-function delay in 6502 functions.
uint8_t nmiFlag = 0; // added by OV to aid single-stepping SST mode on KIM-I
uint8_t SSTmode = 0; // SST switch in KIM-I: 1 = on.

#define FLAG_CARRY 0x01
#define FLAG_ZERO 0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL 0x08
#define FLAG_BREAK 0x10
#define FLAG_CONSTANT 0x20
#define FLAG_OVERFLOW 0x40
#define FLAG_SIGN 0x80
#define BASE_STACK 0x100

#define saveaccum(n) a = (uint8_t)((n)&0x00FF)

//flag modifier macros
#define setcarry() cpustatus |= FLAG_CARRY
#define clearcarry() cpustatus &= (~FLAG_CARRY)
#define setzero() cpustatus |= FLAG_ZERO
#define clearzero() cpustatus &= (~FLAG_ZERO)
#define setinterrupt() cpustatus |= FLAG_INTERRUPT
#define clearinterrupt() cpustatus &= (~FLAG_INTERRUPT)
#define setdecimal() cpustatus |= FLAG_DECIMAL
#define cleardecimal() cpustatus &= (~FLAG_DECIMAL)
#define setoverflow() cpustatus |= FLAG_OVERFLOW
#define clearoverflow() cpustatus &= (~FLAG_OVERFLOW)
#define setsign() cpustatus |= FLAG_SIGN
#define clearsign() cpustatus &= (~FLAG_SIGN)

//flag calculation macros
#define zerocalc(n)      \
    {                    \
        if ((n)&0x00FF)  \
            clearzero(); \
        else             \
            setzero();   \
    }
#define signcalc(n)      \
    {                    \
        if ((n)&0x0080)  \
            setsign();   \
        else             \
            clearsign(); \
    }
#define carrycalc(n)      \
    {                     \
        if ((n)&0xFF00)   \
            setcarry();   \
        else              \
            clearcarry(); \
    }
#define overflowcalc(n, m, o)                             \
    {                                                     \
        if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080) \
            setoverflow();                                \
        else                                              \
            clearoverflow();                              \
    }

//6502 CPU registers
uint16_t pc;
uint8_t sp, a, x, y, cpustatus;

// BCD fix OV 20140915 - helper variables for adc and sbc
uint16_t lxx, hxx;
// end of BCD fix part 1

//helper variables
uint32_t instructions = 0; //keep track of total instructions executed
int32_t clockticks6502 = 0, clockgoal6502 = 0;
uint16_t oldpc, ea, reladdr, value, result;
uint8_t opcode, oldcpustatus, useaccum;

// --- OVERVIEW OF KIM-1 MEMORY MAP -------------------------------------------------
uint8_t RAM[ONBOARD_RAM]; // main 1KB RAM	     0x0000-0x04FF
// empty            					    0x0900-0x13FF
// I/O and timer of 6530-003, free for user  0x1700-0x173F, not used in KIM ROM
// I/O and timer of 6530-002, used by KIM    0x1740-0x177F, used by LED/Keyboard
uint8_t RAM003[64]; // RAM from 6530-003  0x1780-0x17BF, free for user applications
uint8_t RAM002[64]; // RAM from 6530-002  0x17C0-0x17FF, free for user except 0x17E7-0x17FF
// rom003 is                                 0x1800-0x1BFF
// rom002 is                                 0x1C00-0x1FFF

// note that above 8K map is not replicated 8 times to fill 64K,
// but INSTEAD, emulator mirrors last 6 bytes of ROM 002 to FFFB-FFFF:
//               FFFA, FFFB - NMI Vector
//               FFFC, FFFD - RST Vector
//               FFFE, FFFF - IRQ Vector
// Application roms (mchess) are above standard 8K of KIM-1

// --- ROM CODE SECTION ------------------------------------------------------------
// ROM1: KIM-1 ROM002 (monitor main code)                                 $17C0
// ROM2: KIM-1 ROM003 (tape and RS-232 code)                              $1780
// mchess: the updated microchess version from www.benlo.com/microchess
//                                                 recompiled to start at $C000
// calc: the 6502 floating point library from www.crbond.com/calc65.htm
//                                                 recompiled to start at $D000
// disassembler: the famous Baum & Wozniak disassember
//    6502.org/documents/publications/6502notes/6502_user_notes_14.pdf at $2000
// storage arrays that are copied to RAM are also here:
// relocate (first book of kim)                                        to $0110
// branch (first book of kim)                                          to $01A5
// movit (first book of kim)                                           to $1780
// ---------------------------------------------------------------------------------

/* MOVIT utility, copied into RAM 1780-17E3. Length: decimal 102 */
/* for use, see http://users.telenet.be/kim1-6502/6502/fbok.html#p114 */
const uint8_t movit[100] PROGMEM = {
    0xD8, 0xA0, 0xFF, 0x38, 0xA5, 0xD2, 0xE5, 0xD0,
    0x85, 0xD8, 0xA5, 0xD3, 0xE5, 0xD1, 0x85, 0xD9,
    0x18, 0xA5, 0xD8, 0x65, 0xD4, 0x85, 0xD6, 0xA5,
    0xD9, 0x65, 0xD5, 0x85, 0xD7, 0xE6, 0xD8, 0xE6,
    0xD9, 0x38, 0xA5, 0xD4, 0xE5, 0xD0, 0xA5, 0xD5,
    0xE5, 0xD1, 0xA2, 0x00, 0x90, 0x02, 0xA2, 0x02,
    0xA1, 0xD0, 0x81, 0xD4, 0x90, 0x14, 0xC6, 0xD2,
    0x98, 0x45, 0xD2, 0xD0, 0x02, 0xC6, 0xD3, 0xC6,
    0xD6, 0x98, 0x45, 0xD6, 0xD0, 0x02, 0xC6, 0xD7,
    0xB0, 0x0C, 0xE6, 0xD0, 0xD0, 0x02, 0xE6, 0xD1,
    0xE6, 0xD4, 0xD0, 0x02, 0xE6, 0xD5, 0xC6, 0xD8,
    0xD0, 0x02, 0xC6, 0xD9, 0xD0, 0xCC, 0x00};

/* RELOCATE utility, copied into RAM 0110-01A4. Length: decimal 149 */
/* for use, see http://users.telenet.be/kim1-6502/6502/fbok.html#p114 */
const uint8_t relocate[149] PROGMEM = {
    0xD8, 0xA0, 0x00, 0xB1, 0xEA, 0xA8, 0xA2, 0x07,
    0x98, 0x3D, 0x8E, 0x01, 0x5D, 0x95, 0x01, 0xF0,
    0x03, 0xCA, 0xD0, 0xF4, 0xBC, 0x9D, 0x01, 0x30,
    0x0D, 0xF0, 0x22, 0xE6, 0xEA, 0xD0, 0x02, 0xE6,
    0xEB, 0x88, 0xD0, 0xF7, 0xF0, 0xDA, 0xC8, 0x30,
    0xD9, 0xC8, 0xB1, 0xEA, 0xAA, 0xC8, 0xB1, 0xEA,
    0x20, 0x79, 0x01, 0x91, 0xEA, 0x88, 0x8A, 0x91,
    0xEA, 0xA0, 0x03, 0x10, 0xDE, 0xC8, 0xA6, 0xEA,
    0xA5, 0xEB, 0x20, 0x79, 0x01, 0x86, 0xE0, 0xA2,
    0xFF, 0xB1, 0xEA, 0x18, 0x69, 0x02, 0x30, 0x01,
    0xE8, 0x86, 0xE3, 0x18, 0x65, 0xEA, 0xAA, 0xA5,
    0xE3, 0x65, 0xEB, 0x20, 0x79, 0x01, 0xCA, 0xCA,
    0x8A, 0x38, 0xE5, 0xE0, 0x91, 0xEA, 0xC8, 0x10,
    0xB2, 0xC5, 0xE7, 0xB0, 0x11, 0xC5, 0xED, 0xD0,
    0x02, 0xE4, 0xEC, 0x90, 0x09, 0x48, 0x8A, 0x18,
    0x65, 0xE8, 0xAA, 0x68, 0x65, 0xE9, 0x60, 0x0C,
    0X1F, 0x0D, 0x87, 0X1F, 0xFF, 0x03, 0x0C, 0x19,
    0x08, 0x00, 0x10, 0x20, 0x03, 0x02, 0xFF, 0xFF,
    0x01, 0x01, 0x00, 0xFF, 0xFE};

/* BRANCH calculation utility, to be copied into RAM anywhere you want (relocatable). Length: decimal 42 */
/* for use, see http://users.telenet.be/kim1-6502/6502/fbok.htmln#p114 */
const uint8_t branch[42] PROGMEM = {
    0xD8, 0x18, 0xA5, 0xFA, 0xE5, 0xFB, 0x85, 0xF9, 0xC6, 0xF9, 0x20, 0x1F, 0x1F, 0x20, 0x6A, 0x1F,
    0xC5, 0xF3, 0xF0, 0xEC, 0x85, 0xF3, 0xC9, 0x10, 0xB0, 0xE6, 0x0A, 0x0A, 0x0A, 0x0A, 0xA2, 0x04,
    0x0A, 0x26, 0xFA, 0x26, 0xFB, 0xCA, 0xD0, 0xF8, 0xF0, 0xD6};

/* C:\temp27\KIM Uno\sw\tools\WozBaum disasm\WozBaum disasm\dis2.bin (02/09/2014 23:58:36)
   StartOffset: 00000000, EndOffset: 000001F8, Length: 000001F9 */
const uint8_t disasm[505] PROGMEM = {
    0x20, 0x0F, 0x20, 0x20, 0x9E, 0x1E, 0x20, 0x9E, 0x1E, 0x20, 0x9E, 0x1E,
    0x4C, 0x64, 0x1C, 0xA9, 0x0D, 0x85, 0x02, 0x20, 0x21, 0x20, 0x20, 0xFC,
    0x20, 0x85, 0x00, 0x84, 0x01, 0xC6, 0x02, 0xD0, 0xF2, 0x20, 0xE2, 0x20,
    0xA1, 0x00, 0xA8, 0x4A, 0x90, 0x0B, 0x4A, 0xB0, 0x17, 0xC9, 0x22, 0xF0,
    0x13, 0x29, 0x07, 0x09, 0x80, 0x4A, 0xAA, 0xBD, 0x1B, 0x21, 0xB0, 0x04,
    0x4A, 0x4A, 0x4A, 0x4A, 0x29, 0x0F, 0xD0, 0x04, 0xA0, 0x80, 0xA9, 0x00,
    0xAA, 0xBD, 0x5F, 0x21, 0x85, 0x03, 0x29, 0x03, 0x85, 0x04, 0x98, 0x29,
    0x8F, 0xAA, 0x98, 0xA0, 0x03, 0xE0, 0x8A, 0xF0, 0x0B, 0x4A, 0x90, 0x08,
    0x4A, 0x4A, 0x09, 0x20, 0x88, 0xD0, 0xFA, 0xC8, 0x88, 0xD0, 0xF2, 0x48,
    0xB1, 0x00, 0x20, 0x13, 0x21, 0xA2, 0x01, 0x20, 0xF3, 0x20, 0xC4, 0x04,
    0xC8, 0x90, 0xF1, 0xA2, 0x03, 0xC0, 0x04, 0x90, 0xF2, 0x68, 0xA8, 0xB9,
    0x79, 0x21, 0x85, 0x05, 0xB9, 0xB9, 0x21, 0x85, 0x06, 0xA9, 0x00, 0xA0,
    0x05, 0x06, 0x06, 0x26, 0x05, 0x2A, 0x88, 0xD0, 0xF8, 0x69, 0x3F, 0x20,
    0x0B, 0x21, 0xCA, 0xD0, 0xEC, 0x20, 0xF1, 0x20, 0xA2, 0x06, 0xE0, 0x03,
    0xD0, 0x12, 0xA4, 0x04, 0xF0, 0x0E, 0xA5, 0x03, 0xC9, 0xE8, 0xB1, 0x00,
    0xB0, 0x1C, 0x20, 0x13, 0x21, 0x88, 0xD0, 0xF2, 0x06, 0x03, 0x90, 0x0E,
    0xBD, 0x6C, 0x21, 0x20, 0x0B, 0x21, 0xBD, 0x72, 0x21, 0xF0, 0x03, 0x20,
    0x0B, 0x21, 0xCA, 0xD0, 0xD5, 0x60, 0x20, 0xFF, 0x20, 0xAA, 0xE8, 0xD0,
    0x01, 0xC8, 0x98, 0x20, 0x13, 0x21, 0x8A, 0x4C, 0x13, 0x21, 0x20, 0x2F,
    0x1E, 0xA5, 0x01, 0xA6, 0x00, 0x20, 0xDB, 0x20, 0xA9, 0x2D, 0x20, 0x0B,
    0x21, 0xA2, 0x03, 0xA9, 0x20, 0x20, 0x0B, 0x21, 0xCA, 0xD0, 0xF8, 0x60,
    0xA5, 0x04, 0x38, 0xA4, 0x01, 0xAA, 0x10, 0x01, 0x88, 0x65, 0x00, 0x90,
    0x01, 0xC8, 0x60, 0x84, 0x07, 0x20, 0xA0, 0x1E, 0xA4, 0x07, 0x60, 0x84,
    0x07, 0x20, 0x3B, 0x1E, 0xA4, 0x07, 0x60, 0x40, 0x02, 0x45, 0x03, 0xD0,
    0x08, 0x40, 0x09, 0x30, 0x22, 0x45, 0x33, 0xD0, 0x08, 0x40, 0x09, 0x40,
    0x02, 0x45, 0x33, 0xD0, 0x08, 0x40, 0x09, 0x40, 0x02, 0x45, 0xB3, 0xD0,
    0x08, 0x40, 0x09, 0x00, 0x22, 0x44, 0x33, 0xD0, 0x8C, 0x44, 0x00, 0x11,
    0x22, 0x44, 0x33, 0xD0, 0x8C, 0x44, 0x9A, 0x10, 0x22, 0x44, 0x33, 0xD0,
    0x08, 0x40, 0x09, 0x10, 0x22, 0x44, 0x33, 0xD0, 0x08, 0x40, 0x09, 0x62,
    0x13, 0x78, 0xA9, 0x00, 0x21, 0x01, 0x02, 0x00, 0x80, 0x59, 0x4D, 0x11,
    0x12, 0x06, 0x4A, 0x05, 0x1D, 0x2C, 0x29, 0x2C, 0x23, 0x28, 0x41, 0x59,
    0x00, 0x58, 0x00, 0x00, 0x00, 0x1C, 0x8A, 0x1C, 0x23, 0x5D, 0x8B, 0x1B,
    0xA1, 0x9D, 0x8A, 0x1D, 0x23, 0x9D, 0x8B, 0x1D, 0xA1, 0x00, 0x29, 0x19,
    0xAE, 0x69, 0xA8, 0x19, 0x23, 0x24, 0x53, 0x1B, 0x23, 0x24, 0x53, 0x19,
    0xA1, 0x00, 0x1A, 0x5B, 0x5B, 0xA5, 0x69, 0x24, 0x24, 0xAE, 0xAE, 0xA8,
    0xAD, 0x29, 0x00, 0x7C, 0x00, 0x15, 0x9C, 0x6D, 0x9C, 0xA5, 0x69, 0x29,
    0x53, 0x84, 0x13, 0x34, 0x11, 0xA5, 0x69, 0x23, 0xA0, 0xD8, 0x62, 0x5A,
    0x48, 0x26, 0x62, 0x94, 0x88, 0x54, 0x44, 0xC8, 0x54, 0x68, 0x44, 0xE8,
    0x94, 0x00, 0xB4, 0x08, 0x84, 0x74, 0xB4, 0x28, 0x6E, 0x74, 0xF4, 0xCC,
    0x4A, 0x72, 0xF2, 0xA4, 0x8A, 0x00, 0xAA, 0xA2, 0xA2, 0x74, 0x74, 0x74,
    0x72, 0x44, 0x68, 0xB2, 0x32, 0xB2, 0x00, 0x22, 0x00, 0x1A, 0x1A, 0x26,
    0x26, 0x72, 0x72, 0x88, 0xC8, 0xC4, 0xCA, 0x26, 0x48, 0x44, 0x44, 0xA2,
    0xC8};

uint8_t read6502(uint16_t address)
{
    last_read_address = address;

    uint8_t tempval = 0;

    if (ramMain->inRange(address))
    {
        return ramMain->read(address);
    }

#ifdef USE_EPROM
    if (address < 0x0800)
    {
        return (eepromread(address - 0x0400)); // 0x0400-0x0800 is EEPROM for Arduino,
    }                                          // 0x0800-0x1700 is empty space, should not be read
#endif

    if (timer003->inRange(address))
    {
        return timer003->read(address);
    }

    if (riotIo002->inRange(address))
    {
        return riotIo002->read(address);
    }

    if (ramRiot003->inRange(address))
    {
        return ramRiot003->read(address);
    }

    if (ramRiot002->inRange(address))
    {
        return ramRiot002->read(address);
    }

    if (address < 0x2000 && address >= 0x1C00)
    { // 0x1C00-0x2000 is ROM 002. It needs some intercepting from emulator...
        if (address == 0x1EA0)
        {                  // intercept OUTCH (send char to serial)
            serout(a);     // print A to serial
            pc = 0x1ED3;   // skip subroutine
            return (0xEA); // and return from subroutine with a fake NOP instruction
        }

        if (address == 0x1E65)
        {                  //intercept GETCH (get char from serial). used to be 0x1E5A, but intercept *within* routine just before get1 test
            a = getAkey(); // get A from main loop's curkey

            if (a == 0)
            {
                pc = 0x1E60; // cycle through GET1 loop for character start, let the 6502 runs through this loop in a fake way
                return (0xEA);
            }

            clearkey();
            x = read6502(0x00FD); // x is saved in TMPX by getch routine, we need to get it back in x;
            pc = 0x1E87;          // skip subroutine
            return (0xEA);        // and return from subroutine with a fake NOP instruction
        }

        if (address == 0x1C2A)
        {                                // intercept DETCPS
            RAM002[0x17F3 - 0x17C0] = 1; // just store some random bps delay on TTY in CNTH30
            RAM002[0x17F2 - 0x17C0] = 1; // just store some random bps delay on TTY in CNTL30
            pc = 0x1C4F;                 // skip subroutine
            return (0xEA);               // and return from subroutine with a fake NOP instruction
        }

        if (address == 0x1F1F)
        { // intercept SCANDS (display F9,FA,FB)
            // light LEDs ---------------------------------------------------------
            if (
                threeHex[0][0] != (read6502(0x00FB) & 0xF0) >> 4 ||
                threeHex[0][1] != (read6502(0x00FB) & 0xF) ||
                threeHex[1][0] != (read6502(0x00FA) & 0xF0) >> 4 ||
                threeHex[1][1] != (read6502(0x00FA) & 0xF) ||
                threeHex[2][0] != (read6502(0x00F9) & 0xF0) >> 4 ||
                threeHex[2][1] != (read6502(0x00F9) & 0xF))
            {

                threeHex[0][0] = (read6502(0x00FB) & 0xF0) >> 4;
                threeHex[0][1] = read6502(0x00FB) & 0xF;
                threeHex[1][0] = (read6502(0x00FA) & 0xF0) >> 4;
                threeHex[1][1] = read6502(0x00FA) & 0xF;
                threeHex[2][0] = (read6502(0x00F9) & 0xF0) >> 4;
                threeHex[2][1] = read6502(0x00F9) & 0xF;
            }
            serial_scands();
            // driveLEDs();

            //pc = 0x1F45;   // skip subroutine part that deals with LEDs
            //return (0xEA); // and return a fake NOP instruction for this first read in the subroutine, it'll now go to AK
        }

#ifdef EMULATE_KEYBOARD
        if (address == 0x1EFE)
        {                  // intercept AK (check for any key pressed)
            a = getAkey(); // 0 means no key pressed - the important bit - but if a key is pressed is curkey the right value to send back?
            //a= getKIMkey();
            if (a == 0)
                a = 0xFF; // that's how AK wants to see 'no key'
            pc = 0x1F14;  // skip subroutine

            return (0xEA); // and return a fake NOP instruction for this first read in the subroutine, it'll now RTS at its end
        }
#endif

#ifdef EMULATE_KEYBOARD
        if (address == 0x1F6A)
        { // intercept GETKEY (get key from keyboard)
            //		serout('-');serout('G');serout('K');serout('-');
            a = getKIMkey(); // curkey = the key code in the emulator's keyboard buffer
            clearkey();
            pc = 0x1F90;   // skip subroutine part that deals with LEDs
            return (0xEA); // and return a fake NOP instruction for this first read in the subroutine, it'll now RTS at its end
        }
#endif
    }

    if (rom1->inRange(address))
    {
        return rom1->read(address);
    }

    if (rom2->inRange(address))
    {
        return rom2->read(address);
    }

    if (address < 0x21F9 && address >= 0x2000)
    { // 0x2000-0x21F8 is disasm
        return (pgm_read_byte_near(disasm + address - 0x2000));
    }

    if (romuchess7->inRange(address))
    {
        return romuchess7->read(address);
    }

    if (address >= 0xFFFA)
    {
        // 6502 reset and interrupt vectors. Reroute to top of ROM002.
        return (pgm_read_byte_near(monitor + address - 0xFC00));
    }

    return (0xE1);
}

void write6502(uint16_t address, uint8_t value)
{
    if (ramMain->inRange(address))
    {
        ramMain->write(address, value);
        return;
    }

#ifdef USE_EPROM
    if (address < 0x0800)
    {
        eepromwrite(address - 0x0400, value); // 0x0500-0x0900 is EEPROM for Arduino,
        return;
    }
#endif

    if (timer003->inRange(address))
    {
        timer003->write(address, value);
    }

    if (riotIo002->inRange(address))
    {
        riotIo002->write(address, value);
    }

    if (ramRiot003->inRange(address))
    {
        ramRiot003->write(address, value);
    }

    if (ramRiot002->inRange(address))
    {
        ramRiot002->write(address, value);
    }

    // Character out function for microchess only: write to display at $F001
    if (address == 0xCFF1)
    { // Character out for microchess only
        serout(value);
        return;
    }
}

//a few general functions used by various other functions
void push16(uint16_t pushval)
{
    write6502(BASE_STACK + sp, (pushval >> 8) & 0xFF);
    write6502(BASE_STACK + ((sp - 1) & 0xFF), pushval & 0xFF);
    sp -= 2;
}

void push8(uint8_t pushval)
{
    write6502(BASE_STACK + sp--, pushval);
}

uint16_t pull16()
{
    uint16_t temp16;
    temp16 = read6502(BASE_STACK + ((sp + 1) & 0xFF)) | ((uint16_t)read6502(BASE_STACK + ((sp + 2) & 0xFF)) << 8);
    sp += 2;
    return (temp16);
}

uint8_t pull8()
{
    return (read6502(BASE_STACK + ++sp));
}

void reset6502()
{
    pc = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
    a = 0;
    x = 0;
    y = 0;
    sp = 0xFD;
    cpustatus |= FLAG_CONSTANT;
}

// this is what user has to enter manually when powering KIM on. Why not do it here.
void initKIM()
{
    rom1->install(0x1800, 0x1BFF, cassette);
    rom2->install(0x1C00, 0x1FFF, monitor);
    romuchess7->install(0xC000, 0xC000 + (sizeof(uchess7) / sizeof(uchess7[0])), uchess7);

    timer003->install(0x1704, 0x170F, 0x1700);

    ramMain->install(0x0000, ONBOARD_RAM, RAM);
    ramRiot003->install(0x1780, 0x17BF, RAM003);
    ramRiot002->install(0x17C0, 0x17FF, RAM003);

    uint16_t i;

    write6502(0x17FA, 0x00);
    write6502(0x17FB, 0x1C);
    write6502(0x17FE, 0x00);
    write6502(0x17FF, 0x1C);

    // the code below copies movit (a copy routine) to 0x1780 in RAM. It can be
    // overwritten by users - it's an extra note that the HTML version of the
    // book contains OR scan codes, so don't take the bytes from there!

    for (i = 0; i < 64; i++)
    { //64 of 102 program bytes
        //NOCOPY   RAM003[i] = pgm_read_byte_near(movit + i);
    }

    // movit spans into the second 64 byte memory segment...

    for (i = 0; i < (95 - 64); i++)
    {
        //NOCOPY  RAM002[i] = pgm_read_byte_near(movit + i + 64);
    }

    // the code below copies relocate to 0x0110 in RAM. It can be overwritten
    // by users or by the stack pointer - it's an extra

    for (i = 0; i < 149; i++)
    {
        //NOCOPY  RAM[i + 0x0110] = pgm_read_byte_near(relocate + i);
    }

    // the code below copies branch to 0x01A5 (not 0x17C0 anymore) in RAM. It
    // can be overwritten by users - it's an extra note: the program can easily
    // be damaged by the stack, because it ends at 1CF. Still, the monitor
    // brings the stack down to no worse than 0x1FF-8.

    for (i = 0; i < 42; i++)
    {
        //RAM002[i] = pgm_read_byte_near(branch + i);
        //NOCOPY  RAM[i + 0x01A5] = pgm_read_byte_near(branch + i);
    }
}

void loadTestProgram() // Call this from main() if you want a program preloaded. It's the first program from First Book of KIM...
{
    uint16_t i;

    /*
    // the first program from First Book of KIM...
    uint8_t fbkDemo[9] = {
        0xA5, 0x10, 0xA6, 0x11, 0x85, 0x11, 0x86, 0x10, 0x00};
    write6502(0x0010, 0x10);
    write6502(0x0011, 0x11);
    // */

    //uint8_t fbkDemo[13] = {
    //    0xa9, 0xff, 0x8d, 0x40, 0x17, 0xa9, 0x09, 0x8d, 0x42, 0x17, 0x4c, 0x0a, 0x02};

#define fbkDemo astroid
    size_t l = sizeof fbkDemo / sizeof fbkDemo[0];

    for (i = 0; i < l; i++)
    {
        write6502(i + 0x0200, fbkDemo[i]);
    }
}

//addressing mode functions, calculates effective addresses
void imp() //implied
{
}

void acc() //accumulator
{
    useaccum = 1;
}

void imm() //immediate
{
    ea = pc++;
}

void zp() //zero-page
{
    ea = (uint16_t)read6502((uint16_t)pc++);
}

void zpx() //zero-page,X
{
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)x) & 0xFF; //zero-page wraparound
}

void zpy() //zero-page,Y
{
    ea = ((uint16_t)read6502((uint16_t)pc++) + (uint16_t)y) & 0xFF; //zero-page wraparound
}

void rel() //relative for branch ops (8-bit immediate value, sign-extended)
{
    reladdr = (uint16_t)read6502(pc++);
    if (reladdr & 0x80)
        reladdr |= 0xFF00;
}

void abso() //absolute
{
    ea = (uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8);
    pc += 2;
}

void absx() //absolute,X
{
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8));
    ea += (uint16_t)x;
    pc += 2;
}

void absy() //absolute,Y
{
    ea = ((uint16_t)read6502(pc) | ((uint16_t)read6502(pc + 1) << 8));
    ea += (uint16_t)y;
    pc += 2;
}

void ind() //indirect
{
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc) | (uint16_t)((uint16_t)read6502(pc + 1) << 8);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    pc += 2;
}

void indx() // (indirect,X)
{
    uint16_t eahelp;
    eahelp = (uint16_t)(((uint16_t)read6502(pc++) + (uint16_t)x) & 0xFF); //zero-page wraparound for table pointer
    ea = (uint16_t)read6502(eahelp & 0x00FF) | ((uint16_t)read6502((eahelp + 1) & 0x00FF) << 8);
}

void indy() // (indirect),Y
{
    uint16_t eahelp, eahelp2;
    eahelp = (uint16_t)read6502(pc++);
    eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
    ea = (uint16_t)read6502(eahelp) | ((uint16_t)read6502(eahelp2) << 8);
    ea += (uint16_t)y;
}

static uint16_t getvalue()
{
    if (useaccum)
        return ((uint16_t)a);
    else
        return ((uint16_t)read6502(ea));
}

/*static uint16_t getvalue16() {
    return((uint16_t)read6502(ea) | ((uint16_t)read6502(ea+1) << 8));
}*/

void putvalue(uint16_t saveval)
{
    if (useaccum)
        a = (uint8_t)(saveval & 0x00FF);
    else
        write6502(ea, (saveval & 0x00FF));
}

//instruction handler functions
void adc()
{
    value = getvalue();

    // BCD fix OV 20140915 - adc
    if ((cpustatus & FLAG_DECIMAL) == 0)
    {
        result = (uint16_t)a + value + (uint16_t)(cpustatus & FLAG_CARRY);

        carrycalc(result);
        zerocalc(result);
        overflowcalc(result, a, value);
        signcalc(result);
    }
    else
    { // #ifndef NES_CPU
        // Decimal mode
        lxx = (a & 0x0f) + (value & 0x0f) + (uint16_t)(cpustatus & FLAG_CARRY);
        if ((lxx & 0xFF) > 0x09)
            lxx += 0x06;
        hxx = (a >> 4) + (value >> 4) + (lxx > 15 ? 1 : 0);
        if ((hxx & 0xff) > 9)
            hxx += 6;
        result = (lxx & 0x0f);
        result += (hxx << 4);
        result &= 0xff;
        // deal with carry flag:
        if (hxx > 15)
            setcarry();
        else
            clearcarry();
        zerocalc(result);
        clearsign();     // negative flag never set for decimal mode.
        clearoverflow(); // overflow never set for decimal mode.
                         // end of BCD fix PART 2

        clockticks6502++;
    }
    //    #endif // of NES_CPU

    saveaccum(result);
}

void op_and()
{
    value = getvalue();
    result = (uint16_t)a & value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

void asl()
{
    value = getvalue();
    result = value << 1;

    carrycalc(result);
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

void bcc()
{
    if ((cpustatus & FLAG_CARRY) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void bcs()
{
    if ((cpustatus & FLAG_CARRY) == FLAG_CARRY)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void beq()
{
    if ((cpustatus & FLAG_ZERO) == FLAG_ZERO)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void op_bit()
{
    value = getvalue();
    result = (uint16_t)a & value;

    zerocalc(result);
    cpustatus = (cpustatus & 0x3F) | (uint8_t)(value & 0xC0);
}

void bmi()
{
    if ((cpustatus & FLAG_SIGN) == FLAG_SIGN)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void bne()
{
    if ((cpustatus & FLAG_ZERO) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void bpl()
{
    if ((cpustatus & FLAG_SIGN) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void brk()
{
    pc++;
    push16(pc);                    //push next instruction address onto stack
    push8(cpustatus | FLAG_BREAK); //push CPU cpustatus to stack
    setinterrupt();                //set interrupt flag
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
}

void bvc()
{
    if ((cpustatus & FLAG_OVERFLOW) == 0)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void bvs()
{
    if ((cpustatus & FLAG_OVERFLOW) == FLAG_OVERFLOW)
    {
        oldpc = pc;
        pc += reladdr;
        if ((oldpc & 0xFF00) != (pc & 0xFF00))
            clockticks6502 += 2; //check if jump crossed a page boundary
        else
            clockticks6502++;
    }
}

void clc()
{
    clearcarry();
}

void cld()
{
    cleardecimal();
}

void cpu_cli()
{
    clearinterrupt();
}

void clv()
{
    clearoverflow();
}

void cmp()
{
    value = getvalue();
    result = (uint16_t)a - value;

    if (a >= (uint8_t)(value & 0x00FF))
        setcarry();
    else
        clearcarry();
    if (a == (uint8_t)(value & 0x00FF))
        setzero();
    else
        clearzero();
    signcalc(result);
}

void cpx()
{
    value = getvalue();
    result = (uint16_t)x - value;

    if (x >= (uint8_t)(value & 0x00FF))
        setcarry();
    else
        clearcarry();
    if (x == (uint8_t)(value & 0x00FF))
        setzero();
    else
        clearzero();
    signcalc(result);
}

void cpy()
{
    value = getvalue();
    result = (uint16_t)y - value;

    if (y >= (uint8_t)(value & 0x00FF))
        setcarry();
    else
        clearcarry();
    if (y == (uint8_t)(value & 0x00FF))
        setzero();
    else
        clearzero();
    signcalc(result);
}

void dec()
{
    value = getvalue();
    result = value - 1;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

void dex()
{
    x--;

    zerocalc(x);
    signcalc(x);
}

void dey()
{
    y--;

    zerocalc(y);
    signcalc(y);
}

void eor()
{
    value = getvalue();
    result = (uint16_t)a ^ value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

void inc()
{
    value = getvalue();
    result = value + 1;

    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

void inx()
{
    x++;

    zerocalc(x);
    signcalc(x);
}

void iny()
{
    y++;

    zerocalc(y);
    signcalc(y);
}

void jmp()
{
    pc = ea;
}

void jsr()
{
    push16(pc - 1);
    pc = ea;
}

void lda()
{
    value = getvalue();
    a = (uint8_t)(value & 0x00FF);

    zerocalc(a);
    signcalc(a);
}

void ldx()
{
    value = getvalue();
    x = (uint8_t)(value & 0x00FF);

    zerocalc(x);
    signcalc(x);
}

void ldy()
{
    value = getvalue();
    y = (uint8_t)(value & 0x00FF);

    zerocalc(y);
    signcalc(y);
}

void lsr()
{
    value = getvalue();
    result = value >> 1;

    if (value & 1)
        setcarry();
    else
        clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

void nop()
{
}

void ora()
{
    value = getvalue();
    result = (uint16_t)a | value;

    zerocalc(result);
    signcalc(result);

    saveaccum(result);
}

void pha()
{
    push8(a);
}

void php()
{
    push8(cpustatus | FLAG_BREAK);
}

void pla()
{
    a = pull8();

    zerocalc(a);
    signcalc(a);
}

void plp()
{
    cpustatus = pull8() | FLAG_CONSTANT;
}

void rol()
{
    value = getvalue();
    result = (value << 1) | (cpustatus & FLAG_CARRY);

    carrycalc(result);
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

void ror()
{
    value = getvalue();
    result = (value >> 1) | ((cpustatus & FLAG_CARRY) << 7);

    if (value & 1)
        setcarry();
    else
        clearcarry();
    zerocalc(result);
    signcalc(result);

    putvalue(result);
}

void rti()
{
    cpustatus = pull8();
    value = pull16();
    pc = value;
}

void rts()
{
    value = pull16();
    pc = value + 1;
}

void sbc()
{
    // BCD fix OV 20140915 - adc
    if ((cpustatus & FLAG_DECIMAL) == 0)
    {
        value = getvalue() ^ 0x00FF;
        result = (uint16_t)a + value + (uint16_t)(cpustatus & FLAG_CARRY);

        carrycalc(result);
        zerocalc(result);
        overflowcalc(result, a, value);
        signcalc(result);
    }
    else
    { //   #ifndef NES_CPU
        // decimal mode
        value = getvalue();
        lxx = (a & 0x0f) - (value & 0x0f) - (uint16_t)((cpustatus & FLAG_CARRY) ? 0 : 1);
        if ((lxx & 0x10) != 0)
            lxx -= 6;
        hxx = (a >> 4) - (value >> 4) - ((lxx & 0x10) != 0 ? 1 : 0);
        if ((hxx & 0x10) != 0)
            hxx -= 6;
        result = (lxx & 0x0f);
        result += (hxx << 4);
        result = (lxx & 0x0f) | (hxx << 4);
        // deal with carry
        if ((hxx & 0xff) < 15)
            setcarry(); // right? I think so. Intended is   setCarryFlag((hxx & 0xff) < 15);
        else
            clearcarry();
        zerocalc(result); // zero dec is zero hex, no problem?
        clearsign();      // negative flag never set for decimal mode. That's a simplification, see http://www.6502.org/tutorials/decimal_mode.html
        clearoverflow();  // overflow never set for decimal mode.
        result = result & 0xff;
        // end of BCD fix PART 3 (final part)

        clockticks6502++;
    }
    //    #endif // of NES_CPU

    saveaccum(result);
}

void sec()
{
    setcarry();
}

void sed()
{
    setdecimal();
}

void cpu_sei()
{
    setinterrupt();
}

void sta()
{
    putvalue(a);
}

void stx()
{
    putvalue(x);
}

void sty()
{
    putvalue(y);
}

void tax()
{
    x = a;

    zerocalc(x);
    signcalc(x);
}

void tay()
{
    y = a;

    zerocalc(y);
    signcalc(y);
}

void tsx()
{
    x = sp;

    zerocalc(x);
    signcalc(x);
}

void txa()
{
    a = x;

    zerocalc(a);
    signcalc(a);
}

void txs()
{
    sp = x;
}

void tya()
{
    a = y;

    zerocalc(a);
    signcalc(a);
}

//undocumented instructions
#ifdef UNDOCUMENTED
void lax()
{
    lda();
    ldx();
}

void sax()
{
    sta();
    stx();
    putvalue(a & x);
}

void dcp()
{
    dec();
    cmp();
}

void isb()
{
    inc();
    sbc();
}

void slo()
{
    asl();
    ora();
}

void rla()
{
    rol();
    op_and();
}

void sre()
{
    lsr();
    eor();
}

void rra()
{
    ror();
    adc();
}
#else
#define lax nop
#define sax nop
#define dcp nop
#define isb nop
#define slo nop
#define rla nop
#define sre nop
#define rra nop
#endif

void nmi6502()
{
    push16(pc);
    push8(cpustatus);
    cpustatus |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFA) | ((uint16_t)read6502(0xFFFB) << 8);
    pc = 0x1C1C;
}

void irq6502()
{
    push16(pc);
    push8(cpustatus);
    cpustatus |= FLAG_INTERRUPT;
    pc = (uint16_t)read6502(0xFFFE) | ((uint16_t)read6502(0xFFFF) << 8);
    //	pc = 0x1C1F;
}

#ifdef USE_TIMING
prog_char ticktable[256] PROGMEM = {
    /*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
    /* 0 */ 7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, /* 0 */
    /* 1 */ 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 1 */
    /* 2 */ 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, /* 2 */
    /* 3 */ 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 3 */
    /* 4 */ 6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, /* 4 */
    /* 5 */ 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 5 */
    /* 6 */ 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, /* 6 */
    /* 7 */ 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 7 */
    /* 8 */ 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, /* 8 */
    /* 9 */ 2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, /* 9 */
    /* A */ 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, /* A */
    /* B */ 2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, /* B */
    /* C */ 2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, /* C */
    /* D */ 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* D */
    /* E */ 2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, /* E */
    /* F */ 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7  /* F */
};
#endif

void exec6502(int32_t tickcount)
{
    uint16_t saveticks = clockticks6502;
    // jjz timer003->timer -= tickcount;

#ifdef USE_TIMING
    clockgoal6502 += tickcount;

    while (clockgoal6502 > 0)
    {
#else
    while (tickcount--)
    {
#endif

        // part 1 of single stepping using NMI
        if ((SSTmode == 1) & (pc < 0x1C00)) // no mni if running ROM code (K7), that would also single-step the monitor code!
            nmiFlag = 1;                    // handled after this instruction has completed.
                                            // -------------

        opcode = read6502(pc++);
        cpustatus |= FLAG_CONSTANT;

        useaccum = 0;

        switch (opcode)
        {
        case 0x0:
            imp();
            brk();
            break;
        case 0x1:
            indx();
            ora();
            break;
        case 0x5:
            zp();
            ora();
            break;
        case 0x6:
            zp();
            asl();
            break;
        case 0x8:
            imp();
            php();
            break;
        case 0x9:
            imm();
            ora();
            break;
        case 0xA:
            acc();
            asl();
            break;
        case 0xD:
            abso();
            ora();
            break;
        case 0xE:
            abso();
            asl();
            break;
        case 0x10:
            rel();
            bpl();
            break;
        case 0x11:
            indy();
            ora();
            break;
        case 0x15:
            zpx();
            ora();
            break;
        case 0x16:
            zpx();
            asl();
            break;
        case 0x18:
            imp();
            clc();
            break;
        case 0x19:
            absy();
            ora();
            break;
        case 0x1D:
            absx();
            ora();
            break;
        case 0x1E:
            absx();
            asl();
            break;
        case 0x20:
            abso();
            jsr();
            break;
        case 0x21:
            indx();
            op_and();
            break;
        case 0x24:
            zp();
            op_bit();
            break;
        case 0x25:
            zp();
            op_and();
            break;
        case 0x26:
            zp();
            rol();
            break;
        case 0x28:
            imp();
            plp();
            break;
        case 0x29:
            imm();
            op_and();
            break;
        case 0x2A:
            acc();
            rol();
            break;
        case 0x2C:
            abso();
            op_bit();
            break;
        case 0x2D:
            abso();
            op_and();
            break;
        case 0x2E:
            abso();
            rol();
            break;
        case 0x30:
            rel();
            bmi();
            break;
        case 0x31:
            indy();
            op_and();
            break;
        case 0x35:
            zpx();
            op_and();
            break;
        case 0x36:
            zpx();
            rol();
            break;
        case 0x38:
            imp();
            sec();
            break;
        case 0x39:
            absy();
            op_and();
            break;
        case 0x3D:
            absx();
            op_and();
            break;
        case 0x3E:
            absx();
            rol();
            break;
        case 0x40:
            imp();
            rti();
            break;
        case 0x41:
            indx();
            eor();
            break;
        case 0x45:
            zp();
            eor();
            break;
        case 0x46:
            zp();
            lsr();
            break;
        case 0x48:
            imp();
            pha();
            break;
        case 0x49:
            imm();
            eor();
            break;
        case 0x4A:
            acc();
            lsr();
            break;
        case 0x4C:
            abso();
            jmp();
            break;
        case 0x4D:
            abso();
            eor();
            break;
        case 0x4E:
            abso();
            lsr();
            break;
        case 0x50:
            rel();
            bvc();
            break;
        case 0x51:
            indy();
            eor();
            break;
        case 0x55:
            zpx();
            eor();
            break;
        case 0x56:
            zpx();
            lsr();
            break;
        case 0x58:
            imp();
            cpu_cli();
            break;
        case 0x59:
            absy();
            eor();
            break;
        case 0x5D:
            absx();
            eor();
            break;
        case 0x5E:
            absx();
            lsr();
            break;
        case 0x60:
            imp();
            rts();
            break;
        case 0x61:
            indx();
            adc();
            break;
        case 0x65:
            zp();
            adc();
            break;
        case 0x66:
            zp();
            ror();
            break;
        case 0x68:
            imp();
            pla();
            break;
        case 0x69:
            imm();
            adc();
            break;
        case 0x6A:
            acc();
            ror();
            break;
        case 0x6C:
            ind();
            jmp();
            break;
        case 0x6D:
            abso();
            adc();
            break;
        case 0x6E:
            abso();
            ror();
            break;
        case 0x70:
            rel();
            bvs();
            break;
        case 0x71:
            indy();
            adc();
            break;
        case 0x75:
            zpx();
            adc();
            break;
        case 0x76:
            zpx();
            ror();
            break;
        case 0x78:
            imp();
            cpu_sei();
            break;
        case 0x79:
            absy();
            adc();
            break;
        case 0x7D:
            absx();
            adc();
            break;
        case 0x7E:
            absx();
            ror();
            break;
        case 0x81:
            indx();
            sta();
            break;
        case 0x84:
            zp();
            sty();
            break;
        case 0x85:
            zp();
            sta();
            break;
        case 0x86:
            zp();
            stx();
            break;
        case 0x88:
            imp();
            dey();
            break;
        case 0x8A:
            imp();
            txa();
            break;
        case 0x8C:
            abso();
            sty();
            break;
        case 0x8D:
            abso();
            sta();
            break;
        case 0x8E:
            abso();
            stx();
            break;
        case 0x90:
            rel();
            bcc();
            break;
        case 0x91:
            indy();
            sta();
            break;
        case 0x94:
            zpx();
            sty();
            break;
        case 0x95:
            zpx();
            sta();
            break;
        case 0x96:
            zpy();
            stx();
            break;
        case 0x98:
            imp();
            tya();
            break;
        case 0x99:
            absy();
            sta();
            break;
        case 0x9A:
            imp();
            txs();
            break;
        case 0x9D:
            absx();
            sta();
            break;
        case 0xA0:
            imm();
            ldy();
            break;
        case 0xA1:
            indx();
            lda();
            break;
        case 0xA2:
            imm();
            ldx();
            break;
        case 0xA4:
            zp();
            ldy();
            break;
        case 0xA5:
            zp();
            lda();
            break;
        case 0xA6:
            zp();
            ldx();
            break;
        case 0xA8:
            imp();
            tay();
            break;
        case 0xA9:
            imm();
            lda();
            break;
        case 0xAA:
            imp();
            tax();
            break;
        case 0xAC:
            abso();
            ldy();
            break;
        case 0xAD:
            abso();
            lda();
            break;
        case 0xAE:
            abso();
            ldx();
            break;
        case 0xB0:
            rel();
            bcs();
            break;
        case 0xB1:
            indy();
            lda();
            break;
        case 0xB4:
            zpx();
            ldy();
            break;
        case 0xB5:
            zpx();
            lda();
            break;
        case 0xB6:
            zpy();
            ldx();
            break;
        case 0xB8:
            imp();
            clv();
            break;
        case 0xB9:
            absy();
            lda();
            break;
        case 0xBA:
            imp();
            tsx();
            break;
        case 0xBC:
            absx();
            ldy();
            break;
        case 0xBD:
            absx();
            lda();
            break;
        case 0xBE:
            absy();
            ldx();
            break;
        case 0xC0:
            imm();
            cpy();
            break;
        case 0xC1:
            indx();
            cmp();
            break;
        case 0xC4:
            zp();
            cpy();
            break;
        case 0xC5:
            zp();
            cmp();
            break;
        case 0xC6:
            zp();
            dec();
            break;
        case 0xC8:
            imp();
            iny();
            break;
        case 0xC9:
            imm();
            cmp();
            break;
        case 0xCA:
            imp();
            dex();
            break;
        case 0xCC:
            abso();
            cpy();
            break;
        case 0xCD:
            abso();
            cmp();
            break;
        case 0xCE:
            abso();
            dec();
            break;
        case 0xD0:
            rel();
            bne();
            break;
        case 0xD1:
            indy();
            cmp();
            break;
        case 0xD5:
            zpx();
            cmp();
            break;
        case 0xD6:
            zpx();
            dec();
            break;
        case 0xD8:
            imp();
            cld();
            break;
        case 0xD9:
            absy();
            cmp();
            break;
        case 0xDD:
            absx();
            cmp();
            break;
        case 0xDE:
            absx();
            dec();
            break;
        case 0xE0:
            imm();
            cpx();
            break;
        case 0xE1:
            indx();
            sbc();
            break;
        case 0xE4:
            zp();
            cpx();
            break;
        case 0xE5:
            zp();
            sbc();
            break;
        case 0xE6:
            zp();
            inc();
            break;
        case 0xE8:
            imp();
            inx();
            break;
        case 0xE9:
            imm();
            sbc();
            break;
        case 0xEB:
            imm();
            sbc();
            break;
        case 0xEC:
            abso();
            cpx();
            break;
        case 0xED:
            abso();
            sbc();
            break;
        case 0xEE:
            abso();
            inc();
            break;
        case 0xF0:
            rel();
            beq();
            break;
        case 0xF1:
            indy();
            sbc();
            break;
        case 0xF5:
            zpx();
            sbc();
            break;
        case 0xF6:
            zpx();
            inc();
            break;
        case 0xF8:
            imp();
            sed();
            break;
        case 0xF9:
            absy();
            sbc();
            break;
        case 0xFD:
            absx();
            sbc();
            break;
        case 0xFE:
            absx();
            inc();
            break;
        }
    
    timer003->timer -= clockticks6502 - saveticks;

#ifdef USE_TIMING
        clockgoal6502 -= (int32_t)pgm_read_byte_near(ticktable + opcode);
#else
        timer003->timer -= 1;
#endif
        instructions++;

        // part 2 of NMI single-step handling for KIM-I
        if (nmiFlag == 1)
        {              //SST switch on and not in K7 area (ie, ROM002), so single step
            nmi6502(); // set up for NMI
            nmiFlag = 0;
        }
        // ----------------------------------
    }
}

uint16_t getpc()
{
    return (pc);
}

uint8_t getop()
{
    return (opcode);
}

extern "C"
{
    uint16_t debuggetaddress()
    {
        return last_read_address;
    }
}
