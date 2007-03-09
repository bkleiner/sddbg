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
	types_twofile.ihx	\
	nested_struct.ihx	\
	nested_types.ihx


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


nested_struct.ihx : nested_struct.rel
	sdcc $(CFLAGS) -o nested_struct.ihx nested_struct.rel

nested_types.ihx : nested_types.rel
	sdcc $(CFLAGS) -o nested_types.ihx nested_types.rel


clean:
	rm -f *.asm *.rel *.rst *.cdb *.ihx *.adb *.lnk *.lst *.map *.sym *.mem
	rm -f simple twofunc twofile types_twofileA types_twofileB types_singlefile types_twofile nested_struct nested_types




