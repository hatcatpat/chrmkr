chrviewer=uxnemu ~/source/nasu/nasu.rom
imgviewer=feh

all: chrmkr

chrmkr: chrmkr.c
	gcc -g -o chrmkr *.c -Wall -ansi -lpng

run: all
	./chrmkr

test: all
	./chrmkr ako.chr test.tga
	$(imgviewer) test.tga
	./chrmkr ako.chr test.png
	$(imgviewer) test.png
	./chrmkr ako.chr test.bmp
	$(imgviewer) test.bmp
	./chrmkr test.tga test.chr
	$(chrviewer) test.chr
	./chrmkr test.png test.chr
	$(chrviewer) test.chr
	./chrmkr test.bmp test.chr
	$(chrviewer) test.chr

clean:
	rm chrmkr test.*
