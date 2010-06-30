ifeq ($(DEBUGBUILD), yes)
    DEBUGFLAGS = -g -DDEBUG
else
    DEBUGFLAGS = -O2 -DNDEBUG
endif

ifeq ($(STATIC), no)
    BUILDFLAGS = -fPIC
    LINKFLAGS  = -shared
else
    BUILDFLAGS = 
    LINKFLAGS  = 
endif

export CC         	= gcc
export CPP        	= g++
export CFLAGS     	= -Wall $(DEBUGFLAGS) $(BUILDFLAGS)
export CFLAGSEXT  	= $(DEBUGFLAGS) $(BUILDFLAGS)
export LDFLAGS    	= $(LINKFLAGS)
export LIBPATH    	= ./
export VERSION    	= 0.6svn
export CP         	= cp
export LN         	= ln
export LNFLAGS    	= -s -f
export AR         	= ar
export ARFLAGS    	= rcs
export DESTDIR    	= /usr
export DESTLIBDIR 	= $(DESTDIR)/lib
export DESTINCDIR 	= $(DESTDIR)/include

EXE     			= eetest
EXEIV				= eeiv

SRCGLEW 			= $(wildcard ./src/helper/glew/*.c)
SRCSDLTTF 			= $(wildcard ./src/helper/SDL_ttf/*.c)
SRCSOIL 			= $(wildcard ./src/helper/SOIL/*.c)
SRCFE 				= $(wildcard ./src/helper/fastevents/*.c)
SRCSTBVORBIS 		= $(wildcard ./src/helper/stb_vorbis/*.c)

SRCAUDIO			= $(wildcard ./src/audio/*.cpp)
SRCGAMING			= $(wildcard ./src/gaming/*.cpp)
SRCGRAPHICS			= $(wildcard ./src/graphics/*.cpp)
SRCMATH				= $(wildcard ./src/math/*.cpp)
SRCSYSTEM			= $(wildcard ./src/system/*.cpp)
SRCUI				= $(wildcard ./src/ui/*.cpp)
SRCUTILS     		= $(wildcard ./src/utils/*.cpp)
SRCWINDOW     		= $(wildcard ./src/window/*.cpp)

SRCTEST     		= $(wildcard ./src/test/*.cpp)
SRCEEIV     		= $(wildcard ./src/eeiv/*.cpp)

OBJGLEW 			= $(SRCGLEW:.c=.o)
OBJFE 				= $(SRCFE:.c=.o)
OBJSDLTTF 			= $(SRCSDLTTF:.c=.o)
OBJSOIL 			= $(SRCSOIL:.c=.o)
OBJSTBVORBIS 		= $(SRCSTBVORBIS:.c=.o) 

OBJAUDIO 			= $(SRCAUDIO:.cpp=.o)
OBJGAMING 			= $(SRCGAMING:.cpp=.o)
OBJGRAPHICS 		= $(SRCGRAPHICS:.cpp=.o)
OBJMATH 			= $(SRCMATH:.cpp=.o)
OBJSYSTEM 			= $(SRCSYSTEM:.cpp=.o)
OBJUI 				= $(SRCUI:.cpp=.o)
OBJUTILS			= $(SRCUTILS:.cpp=.o)
OBJWINDOW			= $(SRCWINDOW:.cpp=.o)

OBJHELPERS			= $(OBJGLEW) $(OBJFE) $(OBJSDLTTF) $(OBJSOIL) $(OBJSTBVORBIS)
OBJMODULES			= $(OBJUTILS) $(OBJMATH) $(OBJSYSTEM) $(OBJAUDIO) $(OBJWINDOW) $(OBJGRAPHICS) $(OBJGAMING) $(OBJUI)

OBJTEST     		= $(SRCTEST:.cpp=.o)
OBJEEIV     		= $(SRCEEIV:.cpp=.o)

ifeq ($(STATIC), yes)
    LIB     = libeepp-s.a
    LIBNAME = $(LIBPATH)/$(LIB)
    INSTALL = 
else
    LIB     = libeepp.so
    LIBNAME = $(LIBPATH)/$(LIB).$(VERSION)
    INSTALL = && $(LN) $(LNFLAGS) $(DESTLIBDIR)/$(LIB).$(VERSION) $(DESTLIBDIR)/$(LIB)
endif

all: $(LIB)

libeepp-s.a: $(OBJHELPERS) $(OBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(OBJHELPERS) $(OBJMODULES)

libeepp.so: $(OBJHELPERS) $(OBJMODULES)
	$(CPP) $(LDFLAGS) -Wl,-soname,$(LIB).$(VERSION) -o $(LIBNAME) $(OBJHELPERS) $(OBJMODULES) -lfreetype -lSDL -lsndfile -lopenal -lGL -lGLU

$(OBJMODULES): %.o: %.cpp
	$(CPP) -o $@ -c $< $(CFLAGS) -I/usr/include/freetype2

$(OBJHELPERS): %.o: %.c
	$(CC) -o $@ -c $< $(CFLAGSEXT) -DSTBI_FAILURE_USERMSG -I/usr/include/freetype2

test: $(EXE)

$(EXE): $(OBJHELPERS) $(OBJMODULES) $(OBJTEST)
	$(CPP) -o ./$(EXE) $(OBJHELPERS) $(OBJMODULES) $(OBJTEST) $(LDFLAGS) -lfreetype -lSDL -lsndfile -lopenal -lGL -lGLU

$(OBJTEST): %.o: %.cpp
	$(CPP) -o $@ -c $< $(CFLAGS) -I/usr/include/freetype2

eeiv: $(EXEIV)

$(EXEIV): $(OBJHELPERS) $(OBJMODULES) $(OBJEEIV)
	$(CPP) -o ./$(EXEIV) $(OBJHELPERS) $(OBJMODULES) $(OBJEEIV) $(LDFLAGS) -lfreetype -lSDL -lsndfile -lopenal -lGL -lGLU -lX11 -lXcursor

$(OBJEEIV): %.o: %.cpp
	$(CPP) -o $@ -c $< $(CFLAGS) -I/usr/include/freetype2

docs:
	doxygen ./Doxyfile

clean:
	@rm -rf $(OBJHELPERS) $(OBJMODULES) $(OBJTEST) $(OBJEEIV)
	
cleanall: clean
	@rm -rf $(LIBNAME)
	@rm -rf ./$(EXE)
	@rm -rf ./$(EXEIV)
	@rm -rf ./log.log

install:
	@($(CP) $(LIBNAME) $(DESTLIBDIR) $(INSTALL))
