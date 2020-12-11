
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
GCC_PARAMS=-DMACOSX -Os -m32 -fomit-frame-pointer -ffast-math -Wall -fpack-struct -I/opt/local/include
NASM_PARAMS=-f elf32 -w+orphan-labels
AL_LD_PARAMS=-e _start -lc -shared --oformat elf32-i386
endif
ifeq ($(UNAME), Darwin)
GCC_PARAMS=-m32 -DMACOSX -Os -fomit-frame-pointer -ffast-math -Wall -fpack-struct -I/opt/local/include
NASM_PARAMS=-f macho -DMACOSX -w+orphan-labels
AL_LD_PARAMS=-m32 -e _start -framework OpenAL -lc
endif

OBJS=main.o player.o
#DEBUG=-DDEBUG=1 -g
DEBUG=-O2

.c.o:
	gcc $(GCC_PARAMS) $(DEBUG) $(FEATURES) -c $<

%.o: %.asm
	nasm $(NASM_PARAMS) $(DEBUG) $(FEATURES) $<

player: $(OBJS)
	gcc $(AL_LD_PARAMS) -o player main.o player.o
	strip player

pcompr:
	../../laturi/laturi.32 -f OpenAL -v -o pcompr -i main.o -i player.o

all: player

clean:
	rm -f example *.o *~ audio.raw player



	