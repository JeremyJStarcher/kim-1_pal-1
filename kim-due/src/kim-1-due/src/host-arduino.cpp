#ifdef TARGETWEB
#include "fake-progmen.h"
#else
#include <Arduino.h>
#endif

#include <stdint.h>

#include "led_driver.h"
#include "cpu.h"
#include "host-hardware.h"
#include "boardhardware.h"

#ifdef USE_EPROM
#include <EEPROM.h>
#endif

#ifdef USE_EPROM
uint8_t eepromread(uint16_t eepromaddress)
{
    return EEPROM.read(eepromaddress);
}

void eepromwrite(uint16_t eepromaddress, uint8_t bytevalue)
{
    if (eepromProtect == 0)
        EEPROM.write(eepromaddress, bytevalue);
    else
    {
        Serial.println(F("ERROR: EEPROM STATE IS WRITE-PROTECTED. HIT '>' TO TOGGLE WRITE PROTECT"));
        Serial.println(freeRam());
    }
}
#endif

// check for out of RAM
int freeRam()
{
#ifdef CALC_RAM_SIZE
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
#else
    return -1;
#endif
}

// =================================================================================================
// KIM Uno Board functions are bolted on from here
// =================================================================================================
#ifndef TARGETWEB

void setupUno()
{
    int i;
    // --------- initialse for scanning keyboard matrix -----------------
    // set columns to input with pullups
    for (i = 0; i < 8; i++)
    {
        pinMode(aCols[i], INPUT);     // set pin to input
        digitalWrite(aCols[i], HIGH); // turn on pullup resistors
    }
    // set rows to output, and set them High to be in Neutral position
    for (i = 0; i < 3; i++)
    {
        pinMode(aRows[i], OUTPUT);    // set pin to output
        digitalWrite(aRows[i], HIGH); // set to high
    }
    // --------- clear display buffer ------------------------------------
    for (i = 0; i < 3; i++)
    {
        threeHex[i][0] = threeHex[i][1] = 0;
    }
    //    Serial.println(F("KIM-UNO initialized."));
}

uint8_t parseChar(uint8_t n) //  parse keycode to return its ASCII code
{
    uint8_t c = 0;

    // KIM-I keys
    switch (n - 1)
    { //KIM Uno keyscan codes to ASCII codes used by emulator
    case 7:
        c = VKEY_0;
        break; //        note: these are n-1 numbers!
    case 6:
        c = VKEY_1;
        break; //
    case 5:
        c = VKEY_2;
        break; //
    case 4:
        c = VKEY_3;
        break; //
    case 3:
        c = VKEY_4;
        break; //
    case 2:
        c = VKEY_5;
        break; //
    case 1:
        c = VKEY_6;
        break; //
    case 0:
        c = VKEY_ST;
        break; // ST
    case 15:
        c = VKEY_7;
        break; //
    case 14:
        c = VKEY_8;
        break; //
    case 13:
        c = VKEY_9;
        break; //
    case 12:
        c = VKEY_A;
        break; //
    case 11:
        c = VKEY_B;
        break; //
    case 10:
        c = VKEY_C;
        break; //
    case 9:
        c = VKEY_D;
        break; //
    case 8:
        c = VKEY_RS;
        break; // RS
    case 23:
        c = VKEY_E;
        break; //
    case 22:
        c = VKEY_F;
        break; //
    case 21:
        c = VKEY_AD;
        break; // AD
    case 20:
        c = VKEY_DA;
        break; // DA
    case 19:
        c = VKEY_PLUS;
        break; // +
    case 18:
        c = VKEY_GO;
        break; // GO
    case 17:
        c = VKEY_PC;
        break; // PC
    case 16:
        c = (SSTmode == 0 ? VKEY_SST_ON : VKEY_SST_OFF);
        break; // 	SST toggle
    }

    return c;
}

void scanKeys()
{
    int led, row, col, noKeysScanned;
    static int keyCode = -1, prevKey = 0;
    static unsigned long timeFirstPressed = 0;

    // 0. disable driving the 7segment LEDs -----------------
    for (led = 0; led < 8; led++)
    {
        pinMode(ledSelect[led], INPUT); // set led pins to input
        // not really necessary, just to stop them
        // from driving either high or low.
        digitalWrite(ledSelect[led], HIGH); // Use builtin pullup resistors
    }

    // 1. initialise: set columns to input with pullups
    for (col = 0; col < 8; col++)
    {
        pinMode(aCols[col], INPUT);     // set pin to input
        digitalWrite(aCols[col], HIGH); // turn on pullup resistors
    }
    // 2. perform scanning
    noKeysScanned = 0;

    // #define DEBUG_KEYBOARD

    for (row = 0; row < 3; row++)
    {
        digitalWrite(aRows[row], LOW); // activate this row
        for (col = 0; col < 8; col++)
        {
            if (digitalRead(aCols[col]) == LOW)
            { // key is pressed
                keyCode = col + row * 8 + 1;
                if (keyCode != prevKey)
                {
#ifdef DEBUG_KEYBOARD
                    {
                        Serial.println();
                        Serial.print(" col: ");
                        Serial.print(col, DEC);
                        Serial.print(" row: ");
                        Serial.print(row, DEC);
                        Serial.print(" prevKey: ");
                        Serial.print(prevKey, DEC);
                        Serial.print(" KeyCode: ");
                        Serial.println(keyCode, DEC);
                    }
#endif

                    prevKey = keyCode;
                    curkey = parseChar(keyCode);

#ifdef DEBUG_KEYBOARD
                    Serial.print(" curkey: ");
                    Serial.print(curkey, DEC);
#endif

                    timeFirstPressed = millis(); //
                }
                else
                { // if pressed for >1sec, it's a ModeShift key
                    if ((millis() - timeFirstPressed) > 1000)
                    {                                // more than 1000 ms
                        if (keyCode == 9)            // it was RS button
                            curkey = '>';            // toggle eeprom write protect
                        timeFirstPressed = millis(); // because otherwise you toggle right back!
                    }
                }
            }
            else
                noKeysScanned++; // another row in which no keys were pressed
        }
        digitalWrite(aRows[row], HIGH); // de-activate this row
    }

    if (noKeysScanned == 24) // no keys detected in any row, 3 rows * 8 columns = 24. used to be 28.
        prevKey = 0;         // allows you to enter same key twice
} // end of function
#endif
