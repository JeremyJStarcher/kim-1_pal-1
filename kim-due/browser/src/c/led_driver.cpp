#include <stdint.h>

#include <emscripten/emscripten.h>
#include <emscripten.h>

void init_display()
{
    convert_led_pattern();

    uint8_t logo[8] = {
        0b01110111, // A
        0b00000101, // r
        0b00111101, // d
        0b00011100, // u
        0b00010000, // i
        0b00010101, // n
        0b00011101, // o
        0b00000000  // <blank>
    };

    for (int i = 0; i < sizeof(logo) / sizeof(logo[0]); i++)
    {
        EM_ASM({
            setLed($0, $1);
        },
               i, logo[i]);
    }

    //  clear_display();
}

void driveLED(uint8_t led, uint8_t n, uint8_t raw_led)
{
    // Cheap anti-flicker
    if (n == 0)
    {
        // return;
    }

    // Pick off just the bits we need
    led &= 0b00000111;

    if (led > 5)
    {
        return;
    }

    uint8_t p2 = xlate_led_pattern[n];

    EM_ASM({
        setLed($0, $1);
    },
           led, p2);
}

void driveLEDs()
{
    int ledNo;
    int byt, i;
    int out;

    for (byt = 0; byt < 3; byt++)
    {
        for (i = 0; i < 2; i++)
        {
            ledNo = byt * 2 + i;
            char bcd = threeHex[byt][i];
            out = dig[(int)bcd];
            EM_ASM({
                setLed($0, $1);
            },
                   ledNo, out);
        }
    }
}

void clear_display() {
}
