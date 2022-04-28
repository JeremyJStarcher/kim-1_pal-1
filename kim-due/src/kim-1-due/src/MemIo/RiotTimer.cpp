/* Many of the concepts and timer values were taken from the "z26" project,
   an Atari 2600 emulator.  The 6532RIOT chip timers work the same way as
   the 6530 */

/*
      Write  to 
 Kim Address Relative Address    Sets Divide Ratio To     Interrupt
-------------------------        -c-------------------    ----------
       1704      0004                   1                 Disabled
       1705      0005                   8                 Disabled
       1706      0006                   64                Disabled
       1707      0007                   1024              Disabled
       170C      000C                   1                 Enabled
       170D      000D                   8                 Enabled
       170E      000E                   64                Enabled
       170F      000F                   1024              Enabled

b. Determining the timer status

         After timing has begun, reading address location 1707 will provide
      the timer status.  If the counter has passed the count of zero, bit 7
      will be set to 1, otherwise, bit 7 (and all other bits in location 1707)
      will be zero.  This allows a program to "watch" location 1707 and
      determine when the timer has timed out.

c. Reading the count in the timer

         If the timer has not counted past zero, reading location 1706 will
      provide the current timer count and disable the interrupt option;
      reading location 170E will provide the current timer count and enable
      the interrupt option.  Thus the interrupt option can be changed while
      the timer is counting down.
          If the timer has counted past zero, reading either memory location
      1706 or 170E will restore the divide ratio to its previously programmed
      value, disable the interrupt option and leave the timer with its current
      count (not the count originally written to the timer). Because the timer
      never stops counting, the timer will continue to decrement, pass zero,
      set the divide rate to 1, and continue to count down at the clock
      frequency, unless new information is written to the timer.

d. Using the interrupt option

          In order to use the interrupt option described above, line PB7
      (application connector, pin 15) should be connected to either the
      IRQ (Expansion Connector, pin 4) or NMI (Expansion Connector, pin 6)
      pin depending on the desired interrupt function.  PB7 should be
      programmed as in input line (it's normal state after a RESET).

           NOTE:  If the programmer desires to use PB7 as a normal I/O line,
                  the programmer is responsible for disabling the timer
                  interrupt option (by writing or reading address 1706)
                  so that it does not interfere with normal operation
                  of PB7.  Also, PB7 was designed to be wire-ORed with
                  other possible interrupt sources; if this is not desired,
                  a 5.1K resistor should be used as a pull-up from PB7 to
                  +5v.  (The pull-up should NOT be used if PB7 is connected
                  to NMI or IRQ.)

 */

#include "RiotTimer.h"

#define START_TIME 0x7FFF

MemRiotTimer::MemRiotTimer()
{
    this->timer = START_TIME;
    this->timerMode = DIV1024;
}

void MemRiotTimer::install(
    uint16_t start_range,
    uint16_t end_range,
    uint16_t basd_address)
{
    this->start_range = start_range;
    this->end_range = end_range;
    this->baseAddress = basd_address;
}

void MemRiotTimer::write(uint16_t address, uint8_t value)
{
    switch (address - this->baseAddress)
    {
    case 0x0004:
    case 0x000C:
        this->timer = value << DIV1;
        this->timerMode = DIV1;
        break;
    case 0x0005:
    case 0x000D:
        this->timer = value << DIV8;
        this->timerMode = DIV8;
        break;
    case 0x0006:
    case 0x000E:
        this->timer = value << DIV64;
        this->timerMode = DIV64;
        break;
    case 0x0007:
    case 0x000F:
        this->timer = value << DIV1024;
        this->timerMode = DIV1024;
        break;
    }
}

uint8_t MemRiotTimer::read(uint16_t address)
{
    switch (address - this->baseAddress)
    {
    case 0x0004:
    case 0x0006:
        if (this->timer & 0x40000)
        {
            return this->timer & 0xFF;
        }
        else
        {
            return (this->timer >> this->timerMode) & 0xFF;
        }
    case 0x0005:
    case 0x0007:
        if (this->timer < 0)
        {
            return 0xFF;
        }
        else
        {
            return 0x00;
        }
    }

    return 0xEF;
}
