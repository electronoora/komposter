CCOPTS=-FGLUT -m32 -std=gnu99 -I/opt/local/include -I$(FREETYPE)

# OS X
LDOPTS=-m32 -L/opt/local/lib
CCOPTS=-FGLUT -m32 -std=gnu99 -I/opt/local/include -I$(FREETYPE)

# Linux
#LDOPTS=-L/opt/local/lib -lm
#CCOPTS=-std=gnu99 -m32 -I/opt/local/include

DEBUGOPT=-g
#DEBUGOPT=-mtune=core2 -O3

OBJS=converter.o fileops.o

all: converter

.c.o:
	gcc -c $(CCOPTS) $(DEBUGOPT) $<

converter: $(OBJS)
	gcc $(LDOPTS) -o converter $(OBJS)
	
clean:
	rm -f converter *.o *~


	