#
# Makefile for komposter v2.0
#
# auto-detects the platform and sets specific build options accordingly
#

VERSION=2.0b1

##
## Linux
##
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
  CC=gcc
  INSTALL_PATH_BIN=/usr/local/bin
  INSTALL_PATH_LIB=/usr/local/lib/komposter
  FREETYPE=ftinclude
  MCCOPTS=-DRESOURCEPATH='"$(INSTALL_PATH_LIB)/resources/"'
  LDOPTS=-m32 -L/opt/local/lib -lGL -lglut -lopenal
  LDPOST=resources/linux/libfreetype32.a
  DIST=tar.gz
  CCOPTS=-FGLUT -m32 -std=gnu99 -I/opt/local/include -I$(FREETYPE)
endif

##
## MacOS X
##
ifeq ($(UNAME), Darwin)
  # use 'gcc' on Snow Leopard, this works only on Lion
  CC=gcc-4.2
  INSTALL_PATH_BIN=/Applications
  INSTALL_PATH_LIB=/Applications
  FREETYPE=ftinclude
  MCCOPTS=-I /Developer/SDKs/MacOSX10.6.sdk/Developer/Headers/ -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.6
  LDOPTS=-m32 -Lresources -framework OpenGL -framework GLUT -lfreetype -framework OpenAL -framework CoreFoundation -lz -mmacosx-version-min=10.6
  LDPOST=
  DIST=dmg
  CCOPTS=-FGLUT -m32 -std=gnu99 -I/opt/local/include -I$(FREETYPE)  
endif

##
## Common to all platforms
##
OPTIMOPT=-mtune=core2 -O3
#DEBUGOPT=-g



OBJS=main.o widgets.o bezier.o synthesizer.o font.o dialog.o console.o about.o pattern.o filedialog.o patch.o sequencer.o audio.o modules.o buffermm.o fileops.o dotfile.o

.DEFAULT: komposter

.PHONY: clean converter player dist-dmg dist-tar.gz

.c.o:
	$(CC) -c $(CCOPTS) $(MCCOPTS) $(DEBUGOPT) $(OPTIMOPT) $<

all: komposter

converter:
	make -C converter all
	
player:
	make -C player all

komposter: $(OBJS)
	$(CC) $(LDOPTS) -o komposter $(OBJS) $(LDPOST)

Komposter.app: komposter
	strip komposter
	rm -rf Komposter.app
	mkdir Komposter.app
	mkdir Komposter.app/Contents
	mkdir Komposter.app/Contents/MacOS
	mkdir Komposter.app/Contents/Resources
	mkdir Komposter.app/Contents/Frameworks
	cp komposter Komposter.app/Contents/MacOS/Komposter
	install_name_tool -change /usr/local/lib/libfreetype.6.dylib @executable_path/./libfreetype.6.dylib Komposter.app/Contents/MacOS/Komposter
	install_name_tool -change /System/Library/Frameworks/GLUT.framework/Versions/A/GLUT @executable_path/../Frameworks/GLUT.framework/GLUT Komposter.app/Contents/MacOS/Komposter
	cp resources/libfreetype.6.dylib Komposter.app/Contents/MacOS/
	cp -R resources/GLUT.framework Komposter.app/Contents/Frameworks/
	ln -s Komposter.app/Contents/Macos/libfreetype.6.dylib Komposter.app/Contents/Macos/libfreetype.dylib
	cp resources/komposter.plist Komposter.app/Contents/Info.plist
	cp resources/komposter.PkgInfo Komposter.app/Contents/PkgInfo
	cp resources/komposter.AppSettings Komposter.app/Contents/Resources/AppSettings.plist
	cp resources/komposter.icns Komposter.app/Contents/Resources/appIcon.icns
	cp resources/078MKSD_MC.TTF	Komposter.app/Contents/Resources/
	cp resources/acknowtt.ttf Komposter.app/Contents/Resources/
	cp resources/m42.TTF Komposter.app/Contents/Resources/

dist: dist-$(DIST)

dist-dmg: Komposter.app converter
	mkdir Komposter-$(VERSION)
	mkdir Komposter-$(VERSION)/docs
	mkdir Komposter-$(VERSION)/examples
	mkdir Komposter-$(VERSION)/converter
	mkdir Komposter-$(VERSION)/player
	cp -R Komposter.app Komposter-$(VERSION)/
	cp doc/info.txt Komposter-$(VERSION)/
	cp examples/*.k* Komposter-$(VERSION)/examples/
	cp doc/komposter.txt Komposter-$(VERSION)/docs/
	cp doc/LICENSE Komposter-$(VERSION)/docs/
	cp doc/fileformat.txt Komposter-$(VERSION)/docs/
	cp doc/releasenotes.txt Komposter-$(VERSION)/docs/
	cp converter/*.[ch] converter/Makefile  converter/converter Komposter-$(VERSION)/converter/
	cp player/Makefile player/*.asm player/*.inc Komposter-$(VERSION)/player/
	hdiutil create ./Komposter-$(VERSION).dmg -srcfolder ./Komposter-$(VERSION)/ -ov
	rm -rf Komposter-$(VERSION)

dist-tar.gz: komposter
	strip komposter
	tar zcvf komposter-$(VERSION).tar.gz  --exclude-vcs komposter install.sh resources/*.ttf resources/*.TTF examples doc/info.txt doc/komposter.txt doc/fileformat.txt doc/releasenotes.txt doc/LICENSE converter player

ifeq ($(UNAME), Linux)
install: komposter
	mkdir -p $(INSTALL_PATH_BIN)
	mkdir -p $(INSTALL_PATH_LIB)
	cp komposter $(INSTALL_PATH_BIN)/
	cp -R resources examples doc converter player $(INSTALL_PATH_LIB)/
endif

ifeq ($(UNAME), Darwin)
install: Komposter.app
	cp -R Komposter.app $(INSTALL_PATH_BIN)/
endif


clean:
	rm -f *~ *.o *.raw komposter Komposter-*.dmg komposter-*.tar.gz
	rm -rf Komposter.app dist
	make -C converter clean
	make -C player clean
