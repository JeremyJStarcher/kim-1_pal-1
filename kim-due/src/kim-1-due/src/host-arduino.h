void setupUno(void);
int freeRam(void);

#ifdef USE_EPROM
extern uint8_t eepromread(uint16_t eepromaddress);
extern void eepromwrite(uint16_t eepromaddress, uint8_t bytevalue);
#endif

