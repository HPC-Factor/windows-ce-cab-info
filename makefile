CC=gcc
CFLAGS=-I. -fshort-wchar
DEPS=src/WinCePEHeader.h src/WinCECab000Header.h src/cjson/cJSON.h
OUT_DIR=dist

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

wcecabinfo: src/wcecabinfo.o src/cjson/cJSON.o
	$(shell mkdir -p $(OUT_DIR))
	$(CC) -o $(OUT_DIR)/wcecabinfo src/wcecabinfo.o src/cjson/cJSON.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

install: clean wcecabinfo
	install -m 655 dist/wcecabinfo $(DESTDIR)/bin/

clean:
	rm -f src/*.o src/cjson/*.o dist/wcecabinfo