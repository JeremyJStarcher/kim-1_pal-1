
romdest = "../../src/kim-1-due/src/roms"
#all:	kim.ptp
all:	uchess7.c

uchess7.c: uchess7.bin Makefile
	../tools/rom2c uchess7.bin ${romdest}/uchess7.h uchess7

uchess7.ptp: uchess7.bin Makefile
	srec_cat uchess7.bin -binary -offset 0x1800 -o uchess7.ptp -MOS_Technologies

uchess7.bin: uchess7.o
	ld65 -t none -vm -m uchess7.map -o uchess7.bin uchess7.o

uchess7.o:	uchess7.s
	ca65 -g -l uchess7.lst --feature labels_without_colons uchess7.s
	ca65 -g -l uchess7-kimuno.lst --feature labels_without_colons uchess7-kimuno.s

clean:
	$(RM) *.o *.lst *.map *.bin *.ptp *.c

distclean: clean
