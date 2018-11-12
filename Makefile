demo.com: demo.c videoram.c videoram.h characters.c characters.h
	zcc +cpm -lm -vn -O3 -SO3 -o demo.com demo.c videoram.c characters.c

dpbinfo.com: dpbinfo.c
	zcc +cpm -lm -vn -O3 -SO3 -o dpbinfo.com dpbinfo.c

clean:
	rm demo.com demo.reloc zcc_opt.def
