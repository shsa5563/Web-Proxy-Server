#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS = -lpthread -lrt -lssl -lcrypto 
OBJS = 
HEADERS= .
all:   proxy 

proxy:
	$(CC) -o proxy util.c proxy.c -I$(HEADERS) $(CFLAGS) $(LIBS)
clean:
	rm -f   proxy

