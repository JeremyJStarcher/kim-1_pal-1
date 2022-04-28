#ifndef HOST_HARDWARE_H
#define HOST_HARDWARE_H

#ifndef TARGETWEB
#include <Arduino.h>
#endif

#include <stdint.h>

void interpretkeys(void);
uint8_t xkeyPressed(void);
void scanKeys(void);
void clearkey(void);
uint8_t getKIMkey();           // for emulation of KIM keypad

extern uint8_t useKeyboardLed;
extern uint8_t curkey;
extern char threeHex[3][2];
extern uint8_t eepromProtect;

// Virtual keyboard codes.
// These map to the ASCII value of the given key, as would be
// entered over the virtual terminal.
// The keypad is scanned and converted into these values as well.
// These differ from the "KIMKey" values.
//
// There are values here that would reflect keys wired up to
// the 6502 interrupts on a real KIM
#define VKEY_0 '0'
#define VKEY_1 '1'
#define VKEY_2 '2'
#define VKEY_3 '3'
#define VKEY_4 '4'
#define VKEY_5 '5'
#define VKEY_6 '6'
#define VKEY_7 '7'
#define VKEY_8 '8'
#define VKEY_9 '9'
#define VKEY_A 'A'
#define VKEY_B 'B'
#define VKEY_C 'C'
#define VKEY_D 'D'
#define VKEY_RS 18
#define VKEY_E 'E'
#define VKEY_F 'F'
#define VKEY_AD 1
#define VKEY_DA 4
#define VKEY_PLUS '+'
#define VKEY_GO 7
#define VKEY_PC 16
#define VKEY_ST 20
#define VKEY_SST_ON ']'
#define VKEY_SST_OFF '['

#define VKEY_TOGGLE_SERIAL_MODE 9
#define VKEY_TOGGLE_EPROM_WRITE '>'
#endif
