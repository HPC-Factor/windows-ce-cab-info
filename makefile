CC=gcc
CFLAGS=-I. -fshort-wchar
DEPS=src/WinCePEHeader.h src/WinCECab000Header.h src/cjson/cJSON.h
OUT_DIR=dist

build: src/wcecabinfo.o src/cjson/cJSON.o
	$(shell mkdir -p $(OUT_DIR))
	$(CC) -o $(OUT_DIR)/wcecabinfo src/wcecabinfo.o src/cjson/cJSON.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f src/*.o dist/wcecabinfo