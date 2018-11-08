demo.com: demo.c videoram.c videoram.h characters.h
	zcc +cpm -lm -O3 -SO3 -o demo.com demo.c videoram.c

clean:
	rm demo.com demo.reloc zcc_opt.def
