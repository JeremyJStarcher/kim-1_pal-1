SUBDIRS = "tools" "ROMs" "TheFirstBookOfKIM"  Misc TinyBasic fltpt65_uno microchess
# 	Microchess

all:
	@for i in $(SUBDIRS); do \
	echo "make all in $$i..."; \
	(cd "$$i"; $(MAKE) $(MFLAGS) all); done

clean:
	@for i in $(SUBDIRS); do \
	echo "Clearing in $$i..."; \
	(cd "$$i"; $(MAKE) $(MFLAGS) clean); done
