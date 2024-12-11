CC = gcc
CFLAGS = -Wall -O2
LIBS = -lrabbitmq -lpthread -luuid

all: manager node

manager: manager.c
	$(CC) $(CFLAGS) -o manager manager.c $(LIBS)

node: node.c
	$(CC) $(CFLAGS) -o node node.c $(LIBS)

clean:
	rm -f manager node
