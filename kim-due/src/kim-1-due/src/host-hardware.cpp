#include <stdint.h>

#include "host-hardware.h"
#include "cpu.h"

uint8_t curkey = 0;
uint8_t useKeyboardLed = 0x01; // set to 0 to use Serial port, to 1 to use onboard keyboard/LED display.
uint8_t eepromProtect = 1; // default is to write-protect EEPROM

char threeHex[3][2]; // LED display

// ---------- called from cpu.c ----------------------

uint8_t getAkey()
{
    return (curkey);
}

void clearkey()
{
    curkey = 0;
}

// getKIMkey() translates ASCII keypresses to codes the KIM ROM expects.
// note that, inefficiently, the KIM Uno board's key codes are first translated to ASCII, then routed through
// here just like key presses from the ASCII serial port are. That's inefficient but left like this
// for hysterical raisins.

uint8_t getKIMkey()
{
    //Serial.print('#');  Serial.print(curkey);  Serial.print('#');

    if (curkey == 0)
        return (0xFF); //0xFF: illegal keycode

    if ((curkey >= '0') & (curkey <= '9'))
        return (curkey - '0');
    if ((curkey >= 'A') & (curkey <= 'F'))
        return (curkey - 'A' + 10);
    if ((curkey >= 'a') & (curkey <= 'f'))
        return (curkey - 'a' + 10);

    if (curkey == VKEY_AD)   // ctrlA
        return (0x10);       // AD address mode
    if (curkey == VKEY_DA)   // ctrlD
        return (0x11);       // DA data mode
    if (curkey == VKEY_PLUS) // +
        return (0x12);       // step
    if (curkey == VKEY_GO)   // ctrlG
        return (0x13);       // GO
    if (curkey == VKEY_PC)   // ctrlP
        return (0x14);       // PC mode
    // curkey==ctrlR for hard reset (/RST) (KIM's RS key) is handled in main loop!
    // curkey==ctrlT for ST key (/NMI) is handled in main loop!
    return (curkey); // any other key, should not be hit but ignored by KIM
}

// translate keyboard commands to KIM-I keycodes or emulator actions
void interpretkeys()
{
    // round 1: keys that always have the same meaning
    switch (curkey)
    {
    case VKEY_RS: // CtrlR = RS key = hardware reset (RST)
        reset6502();
        clearkey();
        break;

    case VKEY_ST: // CtrlT = ST key = throw an NMI to stop execution of user program
        nmi6502();
        clearkey();
        break;

    case VKEY_SST_OFF: // SST off
        SSTmode = 0;
        clearkey();
        break;

    case VKEY_SST_ON: // SST on
        SSTmode = 1;
        clearkey();
        break;

    case VKEY_TOGGLE_SERIAL_MODE: // TAB pressed, toggle between serial port and onboard keyboard/display
        if (useKeyboardLed == 0)
        {
            useKeyboardLed = 1;
        }
        else
        {
            useKeyboardLed = 0;
        }
        reset6502();
        clearkey();
        break;

    case VKEY_TOGGLE_EPROM_WRITE: // Toggle write protect on eeprom
        if (eepromProtect == 0)
        {
            eepromProtect = 1;
        }
        else
        {
            eepromProtect = 0;
        }
        clearkey();
        break;
    }
}

uint8_t xkeyPressed() // just see if there's any keypress waiting
{
    return (curkey == 0 ? 0 : 1);
}

