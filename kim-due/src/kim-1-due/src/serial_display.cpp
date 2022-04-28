#include "host-hardware.h"
#include "cpu.h"

uint8_t kim_hardware_i; // static for speed

void serout(uint8_t val)
{
    Serial.write(val);
}

void serouthex(uint8_t val)
{
    Serial.print(val, HEX);
}

void serial_scands(void)
{
}

void serial_statusline(void)
{
    if (!useKeyboardLed) {
        return;
    }

    // Save cursor position
    serout(27);
    serout('[');
    serout('s');

    // Move to the home position
    serout(27);
    serout('[');
    serout('H');

    // Erase entire line
    serout(27);
    serout('[');
    serout('2');
    serout('K');

    serout('>');
    for (kim_hardware_i = 0; kim_hardware_i < 3; kim_hardware_i++)
    {
        serouthex(threeHex[kim_hardware_i][0]);
        serouthex(threeHex[kim_hardware_i][1]);
        if (kim_hardware_i == 1)
        {
            serout(' ');
        }
    }
    serout('<');

    Serial.print(F("   SST mode: "));
    Serial.print(SSTmode ? F("ON ") : F("OFF"));

    Serial.print(F("   LED mode: "));
    Serial.print(useKeyboardLed ? F("ON ") : F("OFF"));

    // Move back to prior position
    serout(27);
    serout('[');
    serout('u');
    serout(13);
}
