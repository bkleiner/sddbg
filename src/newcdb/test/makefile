###############################
# makefile		              #
# newcdb test programs        #
# (C) Ricky White March 2007  #
###############################


CFLAGS = -mmcs51 --model-small --debug --code-loc 0x0000 --iram-size 0xFF

%.rel : %.a51
	asx8051 -losg $<

%.rel : %.c
	sdcc -c $(CFLAGS) $<


all: simple.ihx			\
	twofunc.ihx			\
	twofile.ihx			\
	types_singlefile.ihx\
	types_twofile.ihx

simple.ihx : simple.rel
	sdcc $(CFLAGS) -o simple.ihx simple.rel

twofunc.ihx : twofunc.rel
	sdcc $(CFLAGS) -o twofunc.ihx twofunc.rel

twofile.ihx : twofileA.rel twofileB.rel
	sdcc $(CFLAGS) -o twofile.ihx twofileA.rel twofileB.rel

types_singlefile.ihx : types_singlefile.rel
	sdcc $(CFLAGS) -o types_singlefile.ihx types_singlefile.rel


types_twofile.ihx : types_twofileA.rel types_twofileB.rel
	sdcc $(CFLAGS) -o types_twofileA.ihx types_twofileA.rel types_twofileB.rel

clean:
	rm -f *.asm *.rel *.rst *.cdb *.ihx *.adb *.lnk *.lst *.map *.sym *.mem
	rm -f simple twofunc twofile types_singlefile




