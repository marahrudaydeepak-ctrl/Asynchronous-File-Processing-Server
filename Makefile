CC=gcc
CFLAGS=-Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE
LIBS=-luring -lpthread

all: server client

server: server.c queue.c server.h
	$(CC) $(CFLAGS) -o server server.c queue.c $(LIBS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client
	rm -f storage/*.txt

run: server
	./server 9090
