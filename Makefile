ifeq ($(DYNAMIC), yes)
    LIB     = libeepp.so
    LIBNAME = $(LIBPATH)/$(LIB).$(VERSION)
    INSTALL = && $(LN) $(LNFLAGS) $(DESTLIBDIR)/$(LIB).$(VERSION) $(DESTLIBDIR)/$(LIB)
else
    LIB     = libeepp-s.a
    LIBNAME = $(LIBPATH)/$(LIB)
    INSTALL = 
endif

ifeq ($(DEBUGBUILD), yes)
    DEBUGFLAGS = -g -DDEBUG -DEE_DEBUG -DEE_MEMORY_MANAGER
    RELEASETYPE = debug
else
    DEBUGFLAGS = -fno-strict-aliasing -O3 -s -DNDEBUG
    RELEASETYPE = release
endif

ifeq ($(DYNAMIC), yes)
    BUILDFLAGS = -fPIC
    LINKFLAGS  = -shared
else
    BUILDFLAGS = 
    LINKFLAGS  = 
endif

ifeq ($(LLVM_BUILD), yes)
export CC         	= llvm-gcc
export CPP        	= llvm-g++
else
export CC         	= gcc
export CPP        	= g++
endif

export CFLAGS     	= -Wall $(DEBUGFLAGS) $(BUILDFLAGS)
export CFLAGSEXT  	= $(DEBUGFLAGS) $(BUILDFLAGS)
export LDFLAGS    	= $(LINKFLAGS)
export LIBPATH    	= ./
export VERSION    	= 0.7
export CP         	= cp
export LN         	= ln
export LNFLAGS    	= -s -f
export AR         	= ar
export ARFLAGS    	= rcs
export DESTDIR    	= /usr
export DESTLIBDIR 	= $(DESTDIR)/lib
export DESTINCDIR 	= $(DESTDIR)/include

STRLOWERCASE 		= $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))
OS 					= $(strip $(call STRLOWERCASE, $(shell uname) ) )

ifeq ($(OS), linux)
LIBS 		= -lfreetype -lSDL -lsndfile -lopenal -lGL -lGLU
LIBSIV 		= -lX11 -lXcursor
OTHERINC	= -I/usr/include/freetype2
else
ifeq ($(OS), darwin)
LIBS 		= -lfreetype -lSDL -lSDLmain -lsndfile -framework OpenGL -framework GLUT -framework OpenAL -framework Cocoa -framework CoreFoundation
LIBSIV 		= 
OTHERINC	= -I/usr/include/freetype2
endif
endif

EXE     			= eetest-$(RELEASETYPE)
EXEIV				= eeiv-$(RELEASETYPE)

SRCGLEW 			= $(wildcard ./src/helper/glew/*.c)
SRCSOIL 			= $(wildcard ./src/helper/SOIL/*.c)
SRCSTBVORBIS 		= $(wildcard ./src/helper/stb_vorbis/*.c)
SRCZLIB				= $(wildcard ./src/helper/zlib/*.c)
SRCLIBZIP			= $(wildcard ./src/helper/libzip/*.c)

SRCHAIKUTTF 		= $(wildcard ./src/helper/haikuttf/*.cpp)
SRCBASE				= $(wildcard ./src/base/*.cpp)
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

SRCHELPERS			= $(SRCGLEW) $(SRCSOIL) $(SRCSTBVORBIS) $(SRCZLIB) $(SRCLIBZIP)
SRCMODULES			= $(SRCHAIKUTTF) $(SRCBASE) $(SRCAUDIO) $(SRCGAMING) $(SRCGRAPHICS) $(SRCMATH) $(SRCSYSTEM) $(SRCUI) $(SRCUTILS) $(SRCWINDOW)
SRCALL				= $(SRCMODULES) $(SRCHELPERS) $(SRCTEST) $(SRCEEIV)
SRCHPPALL			= $(SRCALL:.cpp=.hpp)
SRCHALL				= $(SRCALL:.c=.h)

OBJGLEW 			= $(SRCGLEW:.c=.o)
OBJSOIL 			= $(SRCSOIL:.c=.o)
OBJSTBVORBIS 		= $(SRCSTBVORBIS:.c=.o) 
OBJZLIB 			= $(SRCZLIB:.c=.o) 
OBJLIBZIP 			= $(SRCLIBZIP:.c=.o) 

OBJHAIKUTTF 		= $(SRCHAIKUTTF:.cpp=.o)
OBJBASE 			= $(SRCBASE:.cpp=.o)
OBJAUDIO 			= $(SRCAUDIO:.cpp=.o)
OBJGAMING 			= $(SRCGAMING:.cpp=.o)
OBJGRAPHICS 		= $(SRCGRAPHICS:.cpp=.o)
OBJMATH 			= $(SRCMATH:.cpp=.o)
OBJSYSTEM 			= $(SRCSYSTEM:.cpp=.o)
OBJUI 				= $(SRCUI:.cpp=.o)
OBJUTILS			= $(SRCUTILS:.cpp=.o)
OBJWINDOW			= $(SRCWINDOW:.cpp=.o)

OBJHELPERS			= $(OBJGLEW) $(OBJSOIL) $(OBJSTBVORBIS) $(OBJZLIB) $(OBJLIBZIP)
OBJMODULES			= $(OBJHAIKUTTF) $(OBJBASE) $(OBJUTILS) $(OBJMATH) $(OBJSYSTEM) $(OBJAUDIO) $(OBJWINDOW) $(OBJGRAPHICS) $(OBJGAMING) $(OBJUI)

OBJTEST     		= $(SRCTEST:.cpp=.o)
OBJEEIV     		= $(SRCEEIV:.cpp=.o)

OBJDIR				= obj/$(OS)/$(RELEASETYPE)/

FOBJHELPERS			= $(patsubst ./%, $(OBJDIR)%, $(OBJGLEW) $(OBJSOIL) $(OBJSTBVORBIS) $(OBJZLIB) $(OBJLIBZIP) )
FOBJMODULES			= $(patsubst ./%, $(OBJDIR)%, $(OBJHAIKUTTF) $(OBJBASE) $(OBJUTILS) $(OBJMATH) $(OBJSYSTEM) $(OBJAUDIO) $(OBJWINDOW) $(OBJGRAPHICS) $(OBJGAMING) $(OBJUI) )

FOBJTEST     		= $(patsubst ./%, $(OBJDIR)%, $(SRCTEST:.cpp=.o) )
FOBJEEIV     		= $(patsubst ./%, $(OBJDIR)%, $(SRCEEIV:.cpp=.o) )

FOBJEEPP			= $(FOBJMODULES) $(FOBJTEST) $(FOBJEEIV)
FOBJALL 			= $(FOBJHELPERS) $(FOBJEEPP)

DEPSEEPP			= $(FOBJEEPP:.o=.d)
DEPSALL				= $(FOBJALL:.o=.d)

all: lib

dirs:
	@mkdir -p $(OBJDIR)/src
	@mkdir -p $(OBJDIR)/src/helper/glew
	@mkdir -p $(OBJDIR)/src/helper/SOIL
	@mkdir -p $(OBJDIR)/src/helper/stb_vorbis
	@mkdir -p $(OBJDIR)/src/helper/zlib
	@mkdir -p $(OBJDIR)/src/helper/libzip
	@mkdir -p $(OBJDIR)/src/helper/haikuttf
	@mkdir -p $(OBJDIR)/src/base
	@mkdir -p $(OBJDIR)/src/audio
	@mkdir -p $(OBJDIR)/src/gaming
	@mkdir -p $(OBJDIR)/src/graphics
	@mkdir -p $(OBJDIR)/src/math
	@mkdir -p $(OBJDIR)/src/system
	@mkdir -p $(OBJDIR)/src/ui
	@mkdir -p $(OBJDIR)/src/utils
	@mkdir -p $(OBJDIR)/src/window
	@mkdir -p $(OBJDIR)/src/test
	@mkdir -p $(OBJDIR)/src/eeiv

lib: dirs $(LIB)

$(FOBJMODULES):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJHELPERS):
	$(CC) -o $@ -c $(patsubst $(OBJDIR)%.o,%.c,$@) $(CFLAGSEXT) -DSTBI_FAILURE_USERMSG
	@$(CC) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.c,$@)  -DSTBI_FAILURE_USERMSG > $(patsubst %.o,%.d,$@)

$(FOBJTEST):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJEEIV):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(EXEIV): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEEIV)
	$(CPP) -o ./$(EXEIV) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEEIV) $(LDFLAGS) $(LIBS) $(LIBSIV)

$(EXE): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST)
	$(CPP) -o ./$(EXE) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST) $(LDFLAGS) $(LIBS)

libeepp-s.a: $(FOBJHELPERS) $(FOBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES)

libeepp.so: $(FOBJHELPERS) $(FOBJMODULES)
	$(CPP) $(LDFLAGS) -Wl,-soname,$(LIB).$(VERSION) -o $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES) $(LIBS)

os:
	@echo $(OS)

test: dirs $(EXE)

eeiv: dirs $(EXEIV)

docs:
	doxygen ./Doxyfile

clean:
	@rm -rf $(FOBJALL) $(DEPSALL)

cleantemp:
	@rm -rf $(FOBJEEPP) $(DEPSEEPP)

cleanall: clean
	@rm -rf $(LIBNAME)
	@rm -rf ./$(EXE)
	@rm -rf ./$(EXEIV)
	@rm -rf ./log.log

install:
	@($(CP) $(LIBNAME) $(DESTLIBDIR) $(INSTALL))

-include $(DEPSALL)
