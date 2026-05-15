CC = gcc
CFLAGS = -Wall -O2 -I.

all: vfs_test

vfs_test: main.c skred_vfs.c miniz.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f vfs_test assets_production.zip
