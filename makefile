CC=gcc
CFLAGS=-I. -fshort-wchar
DEPS=src/WinCePEHeader.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

build: src/wcecabinfo.o
	$(CC) -o dist/wcecabinfo src/wcecabinfo.o

clean:
	rm -f src/*.o