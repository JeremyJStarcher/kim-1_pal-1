#ifdef TARGETWEB

#include "cpu.h"
#include "led_driver.h"
#include "host-hardware.h"
#include <emscripten/emscripten.h>

extern "C"
{
    void EMSCRIPTEN_KEEPALIVE websetup()
    {
        reset6502();
        initKIM(); // Enters 1c00 in KIM vectors 17FA and 17FE. Might consider doing 17FC as well????????
        loadTestProgram();

        init_display();
    }

    void injectkey(uint8_t key)
    {
        curkey = key;
        interpretkeys();
    }

    void webloop(uint16_t num_instructions)
    {
        exec6502(num_instructions);
    }
}

#endif