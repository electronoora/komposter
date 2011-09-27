AUTOMAKE_OPTIONS = dist-bzip2

EXTRA_DIST = doc/ \
						 examples/ \
						 ftinclude/ \
						 player/ \
						 resources/

AM_CPPFLAGS = -DRESOURCEPATH=\"$(prefix)/share/komposter\"

komposter_SOURCES = about.c \
										audio.c \
										bezier.c \
										buffermm.c \
										console.c \
										dialog.c \
										dotfile.c \
										filedialog.c \
										fileops.c \
										font.c \
										main.c \
										modules.c \
										patch.c \
										pattern.c \
										sequencer.c \
										synthesizer.c \
										widgets.c

bin_PROGRAMS = komposter
