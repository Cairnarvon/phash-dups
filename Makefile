DEFS = -D_GNU_SOURCE -D_POSIX_SOURCE -DDB_DIR='"$(HOME)/.phashdb"'
MW = `MagickWand-config --cflags --ldflags --libs`
WARNS = -Wall -Wextra -pedantic -std=c99
CFLAGS := $(CFLAGS) -lm $(WARNS) $(DEFS) $(MW)


.PHONY: all
all: phash-index phashd phash-dups


# Library

phash.so: src/phash.c
	$(CC) -o $@ -shared $(CFLAGS) -fpic $^
	strip $@

.PHONY: install-lib
install-lib: phash.so
	install phash.so /usr/lib/phash.so
	install src/phash.h /usr/include/phash.h
	@gzip man/phash_dct.3 man/hamming_uint64_t.3
	install man/*.3 man/*.3.gz /usr/share/man/man3/
	@gunzip man/*.gz


# phash-dups

phash.o: src/phash.c
	$(CC) -c $(CFLAGS) $^

phash-index: src/phash-index.c phash.o
	$(CC) -o $@ $(CFLAGS) $^

phashd: src/phashd.c phash.o
	$(CC) -o $@ $(CFLAGS) $^

phash-dups: src/phash-dups.c phash.o
	$(CC) -o $@ $(CFLAGS) $^

.PHONY: install
install: phash-index phashd phash-dups
ifeq "$(USER)" 'root'
	install $^ scripts/* /usr/bin/
	@gzip man/*.1
	install man/*.1.gz /usr/share/man/man1/
	@gunzip man/*.gz
else
	install $^ scripts/* ~/bin/
endif


.PHONY: clean
clean:
	rm -f phash.so phash.o phash-index phashd phash-dups
