#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>

void init_display(void);
void clear_display(void);
void driveLED(uint8_t led, uint8_t code, uint8_t raw_led);

#endif