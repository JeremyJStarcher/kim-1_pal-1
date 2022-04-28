#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <emscripten/emscripten.h>

// The code inside the main will be executed once the WASM module loads.
int main(int argc, char ** argv) {
    printf("WebAssembly module loaded\n");
    return 0;
}

