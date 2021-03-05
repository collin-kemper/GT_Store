CC=gcc
CFLAGS=-O3 -Wall -Wextra -std=c89 -pedantic -Wno-long-long

all: blake2b.o

blake2b.o: blake2b.c
	$(CC) -c blake2b.c -o obj/$@ $(CFLAGS)

clean:
		rm -rf obj/*
