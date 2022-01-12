CC=gcc
CFLAGS=-I. -fshort-wchar
DEPS=src/WinCePEHeader.h WinCECab000Header.h
OUT_DIR=dist

build: src/wcecabinfo.o
	$(shell mkdir -p $(OUT_DIR))
	$(CC) -o $(OUT_DIR)/wcecabinfo src/wcecabinfo.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f src/*.o dist/wcecabinfo