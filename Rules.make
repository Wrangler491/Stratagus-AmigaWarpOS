##       _________ __                 __                               
##      /   _____//  |_____________ _/  |______     ____  __ __  ______
##      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
##      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
##     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
##             \/                  \/          \//_____/            \/ 
##  ______________________                           ______________________
##			  T H E   W A R   B E G I N S
##	   Stratagus - A free fantasy real time strategy game engine
##

# Compile commands
CC=gcc
CCLD=$(CC)
RM=rm -f
MAKE=make

# Prefix for 'make install'
PREFIX=/usr/local

# Use LUA support
CCL		= -DUSE_LUA
CCLLIB		= -lm

# Video support
VIDEO_CFLAGS = -DUSE_SDL -Igg:os-includeppc/SDL -I/gg/os-includeppc/SDL -Igg:os-includeppc 

# Compression support
COMP_CFLAGS		= -DUSE_ZLIB -DUSE_BZ2LIB
COMP_LIBS		= -lz -lbz2

TOOLLIBS=$(XLDFLAGS) -lpng12 -lz -lm   -L/usr/local/lib \
	

STRATAGUS_LIBS= -lpng12 -lz -lm -lmgl \
	 -L/lib -lSDL -lSDL_mixer -lSDL_net -lpng12 -ljpeg -lz -lSDL_Image  $(CCLLIB) $(COMP_LIBS) \
	 -lvorbisfile -lvorbis -logg  -lmikmod  \
	 -llua50 -llualib50 -lm  -lz -lm -L/usr/local/lib -L$(TOPDIR)/amiga_includes 

DISTLIST=$(TOPDIR)/distlist
TAGS=$(TOPDIR)/src/tags

# Linux
EXE=
OUTFILE=$(TOPDIR)/stratagus$(EXE)
ARCH=linux
OE=o
OBJDIR=obj

XIFLAGS= 	-I$(TOPDIR)/amiga_includes -I/gg/ppc-amigaos/newlib/include
IFLAGS=	-I$(TOPDIR)/src/include $(XIFLAGS) -I/gg/os-includeppc -I$(TOPDIR)/src/movie/vp31/include

CFLAGS=  -w -specs=warpup -O2 -pipe -fsigned-char -fomit-frame-pointer -fexpensive-optimizations -ffast-math $(IFLAGS) \
    -DUSE_HP_FOR_XP -DMAP_REGIONS -DUSE_CDAUDIO -DUSE_SDLCD -DUSE_SDLA -DWARPOS -DWITH_NETWORK\
     -O2 -pipe -fsigned-char -fomit-frame-pointer -fexpensive-optimizations -ffast-math -DUSE_SDL -Igg:os-includeppc/SDL -I/gg/os-includeppc/SDL -DUSE_BZ2LIB \
    -DUSE_OGG  \
    -DUSE_MIKMOD  $(CCL) \
    $(COMP_CFLAGS)  \
    -I/usr/local/include

CTAGSFLAGS=-i defptvS -a -f 

# Locks versions with a symbolic name
LOCKVER=	rcs -q -n$(NAME)

# Source code documentation
DOXYGEN=	doxygen
DOCIFY=		docify
DOCPP=		doc++

%.doc: %.c
	@$(TOPDIR)/tools/aledoc $< | $(DOCIFY) > $*-c.doc 2>/dev/null
%.doc: %.h
	@$(TOPDIR)/tools/aledoc $< | $(DOCIFY) > $*-h.doc 2>/dev/null
