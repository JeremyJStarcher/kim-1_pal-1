#ifndef BOARDHARDWARE_H
#define BOARDHARDWARE_H

#if defined(__arm__)
#define ONBOARD_RAM 0x04FF
#endif

#if defined(TARGETWEB)
#define ONBOARD_RAM 0x04FF
#endif


#if defined(__avr_atmega328p__)
#endif

#if defined(__AVR__)
#define USE_EPROM
#define CALC_RAM_SIZE
#define ONBOARD_RAM 1024
#define EMULATE_KEYBOARD
#endif

#ifdef TARGETWEB
#define BOARD_LED_MAX7219 false
#define BOARD_WIRED_LED false
#else
#define BOARD_LED_MAX7219 false
#define BOARD_WIRED_LED false
#define BOARD_LED2 true
#endif

#if BOARD_LED_MAX7219
#define LED_CS 15
#endif

extern uint8_t aCols[8];
extern uint8_t aRows[3];

extern uint8_t ledSelect[8];
extern uint8_t ledSelect7[8];

#endif