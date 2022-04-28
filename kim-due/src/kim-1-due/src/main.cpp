#include <Arduino.h>

#include "cpu.h"
#include "host-hardware.h"
#include "host-arduino.h"
#include "led_driver.h"
#include "serial_display.h"

void setup()
{
    Serial.begin(19200);
    Serial.println();

    setupUno();

    reset6502();
    initKIM(); // Enters 1c00 in KIM vectors 17FA and 17FE. Might consider doing 17FC as well????????
    loadTestProgram();

    Serial.print(F("V2019-05-12 FREEMEM=")); // just a little check, to avoid running out of RAM!
    Serial.println(freeRam());

    init_display();
}

void loop()
{
    const uint16_t redraw_delay = 250;
    static uint32_t end_time = millis() + redraw_delay;
    exec6502(100);

    if (millis() > end_time)
    {
        serial_statusline();
        end_time = millis() + redraw_delay;
    }

    if (Serial.available())
    {
        curkey = Serial.read() & 0x7F;
        interpretkeys();
    }

    scanKeys();
    if (xkeyPressed() != 0)
    {
        interpretkeys();
    }
}
