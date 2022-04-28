#include <stdio.h>
#include <stdlib.h>
#include "Riot002.h"

#include "../host-hardware.h"
#include "../boardhardware.h"
#include "../led_driver.h"
#include "../serial_display.h"

#define ioSAD 0x1740   // 6530 A Data
#define aIoPADD 0x1741 // 6530 A Data Direction
#define ioSBD 0x1742   // 6530 B Data
#define aIoPBDD 0x1743 // 6530 B Data Direction

// #define EMULATE_KEYBOARD

//assumes little endian
#define printBits(v) printBits2(sizeof(v), (&v))

void printBits2(size_t const size, void const *const ptr)
{
    unsigned char *b = (unsigned char *)ptr;
    unsigned char byte;
    int i, j;

    for (i = size - 1; i >= 0; i--)
    {
        for (j = 7; j >= 0; j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
}

void MemIoRiot002::processIoChange()
{
    uint8_t led;
    uint8_t code;
    /* The LED port values start at 9 and work up
    * by increments of two:
    *  Port B Data Register       LED #
    * ---------------------       -----
    *  09                           1
    *  0b                           2
    *  0d                           3
    *  0f                           4
    *  11                           5
    *  13                           6
    */

    led = ((ioPBD - 9) >> 1) & 0b111;

    if (ioPADD == 0x7F && false)
    {
        printf("ioPBD: ");
        printBits(ioPBD);
        printf(" led: ");
        printBits(led);
        puts("");
    }

    // There is a software demo that fails if we include
    // the ioPADD.
    // trouble is, I'm not sure if the demo is actually working
    // or not, since it was written for a KIM replica, and not a
    // real KIM.
    // However, leaving it out doesn't seem to cause any harm.

    //                 * = 0200
    // 0200   A9 FF      LDA #$FF
    // 0202   8D 40 17   STA $1740
    // 0205   A9 09      LDA #$09
    // 0207   8D 42 17   STA $1742
    // 020A   4C 0A 02   JMP $020A
    // 020D              .END

    code = ioPAD & ioPADD;
    driveLED(led, code, ioPBD);
}

MemIoRiot002::MemIoRiot002()
{
    /* I/O Space of the RIOT002 */
    this->start_range = 0x1740;
    this->end_range = 0x177F;

    this->ioPAD = 0;
    this->ioPADD = 0xFF;
    this->ioPBD = 0;
    this->ioPBDD = 0;
}

/***************************************************
 * Real KIM-1 Keyboard scan codes.  This are the scans
 * that we are emulating.
 *
 * Note! This differs from the Kim-UNO keyboard which
 * has an entirely different scan pattern.
 *
    ioPAD          (opPDB .....XX.)
     bit         00    01    10    11        ioPAD
  assignment    row0  row1  row2  row3       value
  -----------   ----- ----  ----  ----    ---------
      bit6        0    7     E            1011 1111
      bit5        1    8     F            1101 1111
      bit4        2    9     AD           1110 1111
      bit3        3    A     DA           1111 0111
      bit2        4    B     +            1111 1011
      bit1        5    C     GO           1111 1101
      bit0        6    D     PC   SST     1111 1110
 */

uint8_t MemIoRiot002::read(uint16_t address)
{
#ifdef EMULATE_KEYBOARD
    if (address == 0x1740)
    {
        // returns 1 for Keyboard/LED or 0 for Serial terminal
        return (useKeyboardLed);
    }

    serout('%');
    serout('6'); // trap code 6 - read in I/O 002
    return (0);
#else

    // The ROM needs the button held down for a certain length of
    // time as debounce technique.  Fake holding the button down.
    const uint8_t HOLDBUTTON_DELAY = 0x08; // Found by trial and error
    static uint8_t ctr = 0;
    static bool holding_key_down = false;
    static uint8_t last_key_value;

    uint8_t key_value;
    uint8_t ret = 0;

    if (holding_key_down)
    {
        key_value = last_key_value;

        ctr--;
        if (ctr == 0)
        {
            holding_key_down = false;
            clearkey();
        }
    }
    else
    {
        key_value = getKIMkey();
        last_key_value = key_value;
        if (key_value != 0xFF)
        {
            holding_key_down = true;
            ctr = HOLDBUTTON_DELAY;
        }
    }

    switch (address)
    {
    case ioSAD:
        switch (ioPBD & 0x07)
        {
        case 1:
            ret = ~(0x40 >> key_value);
            break;
        case 3:
            ret = ~(0x40 >> (key_value - 7));
            break;
        case 5:
            ret = ~(0x40 >> (key_value - 14));
            break;
        case 7:
            ret = useKeyboardLed ? 0xFF : 0xFE;
            break;
        default:
            ret = 0xFF;
            break;
        }

        ret &= ~ioPADD;
        break;

    case aIoPADD:
        ret = ioPADD;
        break;

    case ioSBD:
        ret = ioPBD;
        ret &= ~ioPBDD;
        break;

    case aIoPBDD:
        ret = ioPBDD;
        break;

    default:
        ret = 0xff;
        break;
    }

    return ret;
#endif
}

void MemIoRiot002::write(uint16_t address, uint8_t value)
{
    switch (address)
    {
    case ioSAD:

        /*
            * ioPDB set to         drives digit
            * ------------         ------------
            * xxx1001x               0 (leftmost)
            * xxx1010x               1
            * xxx1011x               2
            * xxx1100x               3
            * xxx1101x               4
            * xxx1110x               5 (rightmost)
            * */

        ioPAD = value;
        processIoChange();

        break;
    case aIoPADD:
        ioPADD = value;
        processIoChange();
        break;

    case ioSBD:
        ioPBD = value & ioPBDD;
        processIoChange();
        break;

    case aIoPBDD:
        ioPBDD = value;
        processIoChange();
        break;

    default:
        break;
    }
}
