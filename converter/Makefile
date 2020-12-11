CCOPTS=-m32 -std=gnu99

# OS X
LDOPTS=-m32
# -L/opt/local/lib
CCOPTS=-FGLUT -m32 -std=gnu99 

# Linux
#LDOPTS=-lm
#CCOPTS=-std=gnu99 -m32

DEBUGOPT=-g
#DEBUGOPT=-mtune=core2 -O3

OBJS=converter.o fileops.o moduleinfo.o

all: converter

.c.o:
	gcc -c $(CCOPTS) $(DEBUGOPT) $<

converter: $(OBJS)
	gcc $(LDOPTS) -o converter $(OBJS)
	
clean:
	rm -f converter *.o *~


	