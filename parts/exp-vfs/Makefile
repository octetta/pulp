CC = gcc
CFLAGS = -Wall -O2 -I.

# Using version 2.1.0 where miniz was strictly and genuinely self-contained
MINIZ_URL_C = https://raw.githubusercontent.com/richgel999/miniz/2.1.0/miniz.c
MINIZ_URL_H = https://raw.githubusercontent.com/richgel999/miniz/2.1.0/miniz.h

all: get_miniz vfs_test

vfs_test: main.c skred_vfs.c miniz.c
	$(CC) $(CFLAGS) -o $@ $^

get_miniz:
	@if [ ! -f miniz.c ]; then curl -sLO $(MINIZ_URL_C); fi
	@if [ ! -f miniz.h ]; then curl -sLO $(MINIZ_URL_H); fi

clean:
	rm -f vfs_test miniz.c miniz.h miniz_export.h assets_production.zip
