CC=gcc
CFLAGS = -g -O2 -I.
LDLIBS = -lpthread

PROGS = myshell

all: myshell.o  $(PROGS)

$(PROGS): myshell.o

clean:
	rm -rf $(PROGS) *~


