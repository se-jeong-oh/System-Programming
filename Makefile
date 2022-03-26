CC=gcc
CFLAGS = -g -O2 -I.
LDLIBS = -lpthread

PROGS = task

all: csapp.o  $(PROGS)

$(PROGS): csapp.o

clean:
	rm -rf $(PROGS) *~


