romdest=./rom

PROGRAM=guess-word
WORDLIST=wordlist
DEST=/tmp/guessword

all: $(PROGRAM).ptp ${WORDLIST}.ptp
	rm -rf ${DEST}
	mkdir ${DEST}
	cp *.ptp ${DEST}
	cp *.hex ${DEST}

wordlist.ptp: js/make-word-list.js
	cd js; node make-word-list; cd ..
	cp js/$(WORDLIST).s .
	ca65 -g -l $(WORDLIST).lst $(WORDLIST).s
	ld65 -t none -vm -m $(WORDLIST).map -o $(WORDLIST).bin $(WORDLIST).o
	srec_cat $(WORDLIST).bin -binary -offset 0x2000 -o $(WORDLIST).ptp -MOS_Technologies
	ptp2hex	${WORDLIST}.ptp > /dev/null

${PROGRAM}.c: ${PROGRAM}.bin Makefile
	ls -alh ${romdest}

$(PROGRAM).ptp: $(PROGRAM).bin Makefile
	srec_cat $(PROGRAM).bin -binary -offset 0x0200 -o $(PROGRAM).ptp -MOS_Technologies

$(PROGRAM).bin: $(PROGRAM).o
	ld65 -t none -vm -m $(PROGRAM).map -o $(PROGRAM).bin $(PROGRAM).o

$(PROGRAM).o:	$(PROGRAM).s
	ca65 -g -l $(PROGRAM).lst $(PROGRAM).s

clean:
	$(RM) *.o *.lst *.map *.bin  js/wordlist.s wordlist.s

distclean: clean
