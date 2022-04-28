all:	fs6 fs7

fs6:	fs6.cba 
	wine ../../bin/cba65/CBA65.EXE fs6.cba
	../tools/h65toc FS6.h65 /tmp/fs6.c

fs7:	fs7.cba 
	wine ../../bin/cba65/CBA65.EXE fs7.cba
	../tools/h65toc FS7.h65 ../../src/kim-1-due/src/roms/calcrom.h calcRom 0x30 

clean:
	$(RM) *.h6x *.hex *.lst *.h65 *.xrf *.log

distclean: clean

