CC= gcc
CFLAGS= -g -Wall
LIBS = 

all:   cclient server

cclient: cclient.c shared.c gethostbyname6.c
	$(CC) $(CFLAGS) -o cclient cclient.c shared.c gethostbyname6.c $(LIBS)

server: server.c shared.c gethostbyname6.c
	$(CC) $(CFLAGS) -o server server.c shared.c gethostbyname6.c $(LIBS)

.c.o:
	gcc -c $(CFLAGS) $< -o $@ $(LIBS)

cleano:
	rm -f *.o

clean:
	rm -f server cclient *.o
