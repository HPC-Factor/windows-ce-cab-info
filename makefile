CC?=gcc
CFLAGS=-I.
DEPS=src/WinCePEHeader.h src/WinCECab000Header.h src/cjson/cJSON.h
OUT_DIR=dist

# DESTDIR is environment variable, but if it is not set, then set default value
ifeq ($(DESTDIR),)
    DESTDIR := /usr/local
endif

ifeq ($(CC),x86_64-w64-mingw32-gcc)
	WINICONV=-L/usr/x86_64-w64-mingw32/bin -liconv
endif

wcecabinfo: src/wcecabinfo.o src/cjson/cJSON.o
	$(shell mkdir -p $(OUT_DIR))
	$(CC) -o $(OUT_DIR)/wcecabinfo src/wcecabinfo.o src/cjson/cJSON.o $(WINICONV) -static

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

install: clean wcecabinfo
	install -m 655 dist/wcecabinfo $(DESTDIR)/bin/

clean:
	rm -f src/*.o src/cjson/*.o dist/wcecabinfo dist/wcecabinfo.exe