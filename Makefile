STRLOWERCASE 		= $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))
OS 					= $(strip $(call STRLOWERCASE, $(shell uname) ) )

export LIBPATH    	= ./
export VERSION    	= 0.8
export CP         	= cp
export LN         	= ln
export LNFLAGS    	= -s -f
export AR         	= ar
export ARFLAGS    	= rcs
export DESTDIR    	= /usr
export DESTLIBDIR 	= $(DESTDIR)/lib
export DESTINCDIR 	= $(DESTDIR)/include

ifeq ($(DYNAMIC), yes)
	LIB     = libeepp.so
	LIBNAME = $(LIBPATH)/$(LIB).$(VERSION)
	INSTALL = && $(LN) $(LNFLAGS) $(DESTLIBDIR)/$(LIB).$(VERSION) $(DESTLIBDIR)/$(LIB)
else
	LIB     = libeepp-s.a
	LIBNAME = $(LIBPATH)/$(LIB)
	INSTALL = 
endif

ifeq ($(LLVM_BUILD), yes)
export CC         	= clang
export CPP        	= clang++
else
export CC         	= gcc
export CPP        	= g++
endif

ifeq ($(DEBUGBUILD), yes)
    DEBUGFLAGS = -g -DDEBUG -DEE_DEBUG -DEE_MEMORY_MANAGER
    RELEASETYPE = debug
else
	ifeq ($(LLVM_BUILD), yes)
		DEBUGFLAGS = -fno-strict-aliasing -O3 -DNDEBUG -ffast-math
	else
    	DEBUGFLAGS = -fno-strict-aliasing -O3 -s -DNDEBUG -ffast-math
    endif
    
    RELEASETYPE = release
endif

ifeq ($(DYNAMIC), yes)
    BUILDFLAGS = -fPIC
    LINKFLAGS  = -shared
else
    BUILDFLAGS = 
    LINKFLAGS  = 
endif

ifeq ($(BACKENDS_ALL),yes)
	BACKEND_SDL		= yes
	BACKEND_ALLEGRO	= yes
endif

ifeq ($(BACKEND_SDL),)
	ifeq ($(BACKEND_ALLEGRO),)
		BACKEND_SDL	= yes
	endif
endif

ifeq ($(BACKEND_SDL),yes)

	# First check for SDL2
	SDLVERSION2			= $(shell type -P sdl2-config &>/dev/null && sdl2-config --version || echo "")
	
	ifeq ($(SDLVERSION2),)
		# Then for SDL 1.2 or SDL 1.3
		SDLVERSION				= $(shell type -P sdl-config &>/dev/null && sdl-config --version || echo "")

		ifeq ($(SDLVERSION),)
			# Default 2.0.0
			SDL_VERSION		= 2.0.0
		else
			SDL_VERSION			= $(SDLVERSION)
		endif
	else
		SDL_VERSION		= $(SDLVERSION2)
	endif
	
	# Then for SDL 1.2 or SDL 1.3
	#SDLVERSION				= $(shell type -P sdl-config &>/dev/null && sdl-config --version || echo "")

	#ifeq ($(SDLVERSION),)
		#SDLVERSION2			= $(shell type -P sdl2-config &>/dev/null && sdl2-config --version || echo "")
		
		#ifeq ($(SDLVERSION2),)
			# Default 2.0.0
			#SDL_VERSION		= 2.0.0
		#else
			#SDL_VERSION		= $(SDLVERSION2)
		#endif
	#else
		#SDL_VERSION			= $(SDLVERSION)
	#endif
	
	# If version is 1.2.x
	ifneq (,$(findstring 1.2,$(SDL_VERSION)))
		ifeq ($(OS), darwin)
			SDL_ADD_LINK	= -framework Cocoa -lSDLmain
		else
			SDL_ADD_LINK	=
		endif
	
		SDL_BACKEND_LINK	= -lSDL $(SDL_ADD_LINK)

		SDL_BACKEND_SRC		= $(wildcard ./src/window/backend/SDL/*.cpp)

		EE_SDL_VERSION		= -DEE_SDL_VERSION_1_2
	else
		ifeq ($(SHARED_BACKEND),)
			#Check if static library exists
			SDL_STATIC_FOUND	= $(shell ls libs/$(OS)/libSDL.a >/dev/null 2>&1 && echo "YES" || echo "NO")

			SDL2_STATIC_FOUND	= $(shell ls libs/$(OS)/libSDL2.a >/dev/null 2>&1 && echo "YES" || echo "NO")

			ifeq ($(SDL_STATIC_FOUND),NO)
				ifeq ($(SDL2_STATIC_FOUND),NO)
					SHARED_BACKEND = yes
				endif
			endif
		endif

		# Compile as shared?
		ifeq ($(SHARED_BACKEND),yes)
			ifneq (,$(findstring 1.3,$(SDL_VERSION)))

				ifeq ($(OS), darwin)
					SDL_ADD_LINK	= -framework Cocoa -lSDLmain2
				else
					SDL_ADD_LINK	=
				endif
		
				SDL_BACKEND_LINK	= -lSDL $(SDL_ADD_LINK)

				EE_SDL_VERSION		= -DEE_SDL_VERSION_1_3
			else
				SDL_BACKEND_LINK	= -lSDL2

				EE_SDL_VERSION		= -DEE_SDL_VERSION_2
			endif
		else
			# Compile as static then... ( only SDL 1.3 or SDL 2 allowed )
			
			# If version is 1.3.x
			ifneq (,$(findstring 1.3,$(SDL_VERSION)))
				SDL_BACKEND_LINK	= libs/$(OS)/libSDL.a

				EE_SDL_VERSION		= -DEE_SDL_VERSION_1_3
			else
				# If version is 2.x.x
				SDL_BACKEND_LINK	= libs/$(OS)/libSDL2.a

				EE_SDL_VERSION		= -DEE_SDL_VERSION_2
			endif

		endif
		
		SDL_BACKEND_SRC		= $(wildcard ./src/window/backend/SDL2/*.cpp)
	endif
	
	SDL_DEFINE			= -DEE_BACKEND_SDL_ACTIVE $(EE_SDL_VERSION)
else
	SDL_BACKEND_LINK	= 
	SDL_BACKEND_SRC		= 
	SDL_DEFINE			= 
endif

ifeq ($(BACKEND_ALLEGRO), yes)
	ifeq ($(OS), darwin)
		ALLEGRO_BACKEND_LINK	= -lallegro -lallegro_main
	else
		ALLEGRO_BACKEND_LINK	= -lallegro
	endif

	ALLEGRO_BACKEND_SRC		= $(wildcard ./src/window/backend/allegro5/*.cpp)
	ALLEGRO_DEFINE			= -DEE_BACKEND_ALLEGRO_ACTIVE
else
	ALLEGRO_BACKEND_LINK	= 
	ALLEGRO_BACKEND_SRC		= 
	ALLEGRO_DEFINE			= 
endif

BACKENDFLAGS = $(SDL_DEFINE) $(ALLEGRO_DEFINE)

ifeq ($(NO_LIBSNDFILE),yes)
	LIBSNDFILE	=
	SNDFILEFLAG = -DEE_NO_SNDFILE
else
	LIBSNDFILE	= -lsndfile
	SNDFILEFLAG = 
endif

ifeq ($(GLES2), yes)
	FINALFLAGS = $(DEBUGFLAGS) $(SNDFILEFLAG) -DEE_GLES2 -DSOIL_GLES2
else
	ifeq ($(GLES1), yes)
		FINALFLAGS = $(DEBUGFLAGS) $(SNDFILEFLAG) -DEE_GLES1 -DSOIL_GLES1
	else
		FINALFLAGS = $(DEBUGFLAGS) $(SNDFILEFLAG)
	endif
endif

export CFLAGS     	= -Wall -Wno-unknown-pragmas $(FINALFLAGS) $(BUILDFLAGS) $(BACKENDFLAGS)
export CFLAGSEXT  	= $(FINALFLAGS) $(BUILDFLAGS)
export LDFLAGS    	= $(LINKFLAGS)

ifeq ($(OS), linux)

LIBS 		= -lrt -lpthread -lX11 -lfreetype -lopenal -lGL -lXcursor $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK)
OTHERINC	= -I/usr/include/freetype2
PLATFORMSRC	= $(wildcard ./src/window/platform/x11/*.cpp)

else

ifeq ($(OS), darwin)
LIBS 		= -lfreetype -framework OpenGL -framework OpenAL -framework CoreFoundation -framework AGL $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK)
OTHERINC	= -I/usr/include/freetype2 -I/usr/local/include/freetype2
PLATFORMSRC = $(wildcard ./src/window/platform/osx/*.cpp)

else

ifeq ($(OS), haiku)

LIBS 		= -lfreetype -lopenal -lGL $(SDL_BACKEND_LINK)
OTHERINC	= -I/usr/include/freetype2
PLATFORMSRC	= 

else

ifeq ($(OS), freebsd)

LIBS 		= -lrt -lpthread -lX11 -lfreetype -lopenal -lGL -lXcursor $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK)
OTHERINC	= -I/usr/include/freetype2
PLATFORMSRC	= $(wildcard ./src/window/platform/x11/*.cpp)

endif

endif

endif

endif

HELPERSINC			= -I./src/helper/chipmunk -I./src/helper/zlib

EXE     			= eetest-$(RELEASETYPE)
EXEIV				= eeiv-$(RELEASETYPE)
EXEFLUID			= eefluid-$(RELEASETYPE)
EXEBNB				= bnb-$(RELEASETYPE)
EXEEMPTYWINDOW		= eeew-$(RELEASETYPE)
EXEPARTICLES		= eeparticles-$(RELEASETYPE)

ifeq ($(OS), haiku)
SRCGLEW 			= 
else
SRCGLEW 			= $(wildcard ./src/helper/glew/*.c)
endif

SRCSOIL 			= $(wildcard ./src/helper/SOIL/*.c)
SRCSTBVORBIS 		= $(wildcard ./src/helper/stb_vorbis/*.c)
SRCZLIB				= $(wildcard ./src/helper/zlib/*.c)
SRCLIBZIP			= $(wildcard ./src/helper/libzip/*.c)
SRCCHIPMUNK			= $(wildcard ./src/helper/chipmunk/*.c) $(wildcard ./src/helper/chipmunk/constraints/*.c)

SRCHAIKUTTF 		= $(wildcard ./src/helper/haikuttf/*.cpp)
SRCBASE				= $(wildcard ./src/base/*.cpp)
SRCAUDIO			= $(wildcard ./src/audio/*.cpp)
SRCGAMING			= $(wildcard ./src/gaming/*.cpp) $(wildcard ./src/gaming/mapeditor/*.cpp)
SRCGRAPHICS			= $(wildcard ./src/graphics/*.cpp) $(wildcard ./src/graphics/renderer/*.cpp)
SRCMATH				= $(wildcard ./src/math/*.cpp)
SRCSYSTEM			= $(wildcard ./src/system/*.cpp)
SRCUI				= $(wildcard ./src/ui/*.cpp) $(wildcard ./src/ui/tools/*.cpp)
SRCUTILS     		= $(wildcard ./src/utils/*.cpp)
SRCWINDOW     		= $(wildcard ./src/window/*.cpp) $(wildcard ./src/window/backend/null/*.cpp) $(wildcard ./src/window/platform/null/*.cpp) $(SDL_BACKEND_SRC) $(ALLEGRO_BACKEND_SRC) $(PLATFORMSRC)
SRCPHYSICS			= $(wildcard ./src/physics/*.cpp) $(wildcard ./src/physics/constraints/*.cpp)

SRCTEST     		= $(wildcard ./src/test/*.cpp)
SRCEEIV     		= $(wildcard ./src/eeiv/*.cpp)
SRCFLUID     		= $(wildcard ./src/fluid/*.cpp)
SRCPARTICLES    	= $(wildcard ./src/particles/*.cpp) $(wildcard ./src/particles/objects/*.cpp) $(wildcard ./src/particles/gameobjects/*.cpp)
SRCBNB     			= $(wildcard ./src/bnb/*.cpp)
SRCEMPTYWINDOW  	= $(wildcard ./src/test/empty_window/*.cpp)

SRCHELPERS			= $(SRCGLEW) $(SRCSOIL) $(SRCSTBVORBIS) $(SRCZLIB) $(SRCLIBZIP) $(SRCCHIPMUNK)
SRCMODULES			= $(SRCHAIKUTTF) $(SRCBASE) $(SRCAUDIO) $(SRCGAMING) $(SRCGRAPHICS) $(SRCMATH) $(SRCSYSTEM) $(SRCUI) $(SRCUTILS) $(SRCWINDOW) $(SRCPHYSICS)

OBJGLEW 			= $(SRCGLEW:.c=.o)
OBJSOIL 			= $(SRCSOIL:.c=.o)
OBJSTBVORBIS 		= $(SRCSTBVORBIS:.c=.o) 
OBJZLIB 			= $(SRCZLIB:.c=.o) 
OBJLIBZIP 			= $(SRCLIBZIP:.c=.o) 
OBJCHIPMUNK			= $(SRCCHIPMUNK:.c=.o)

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
OBJPHYSICS			= $(SRCPHYSICS:.cpp=.o)

OBJHELPERS			= $(OBJGLEW) $(OBJSOIL) $(OBJSTBVORBIS) $(OBJZLIB) $(OBJLIBZIP) $(OBJCHIPMUNK)
OBJMODULES			= $(OBJHAIKUTTF) $(OBJBASE) $(OBJUTILS) $(OBJMATH) $(OBJSYSTEM) $(OBJAUDIO) $(OBJWINDOW) $(OBJGRAPHICS) $(OBJGAMING) $(OBJUI) $(OBJPHYSICS)

OBJTEST     		= $(SRCTEST:.cpp=.o)
OBJEEIV     		= $(SRCEEIV:.cpp=.o)
OBJFLUID     		= $(SRCFLUID:.cpp=.o)
OBJBNB     			= $(SRCBNB:.cpp=.o)
OBJEMPTYWINDOW		= $(SRCEMPTYWINDOW:.cpp=.o)
OBJPARTICLES     	= $(SRCPARTICLES:.cpp=.o)

OBJDIR				= obj/$(OS)/$(RELEASETYPE)/

FOBJHELPERS			= $(patsubst ./%, $(OBJDIR)%, $(OBJGLEW) $(OBJSOIL) $(OBJSTBVORBIS) $(OBJZLIB) $(OBJLIBZIP) $(OBJCHIPMUNK) )
FOBJMODULES			= $(patsubst ./%, $(OBJDIR)%, $(OBJHAIKUTTF) $(OBJBASE) $(OBJUTILS) $(OBJMATH) $(OBJSYSTEM) $(OBJAUDIO) $(OBJWINDOW) $(OBJGRAPHICS) $(OBJGAMING) $(OBJUI) $(OBJPHYSICS) )

FOBJTEST     		= $(patsubst ./%, $(OBJDIR)%, $(SRCTEST:.cpp=.o) )
FOBJEEIV     		= $(patsubst ./%, $(OBJDIR)%, $(SRCEEIV:.cpp=.o) )
FOBJFLUID     		= $(patsubst ./%, $(OBJDIR)%, $(SRCFLUID:.cpp=.o) )
FOBJBNB     		= $(patsubst ./%, $(OBJDIR)%, $(SRCBNB:.cpp=.o) )
FOBJEMTPYWINDOW     = $(patsubst ./%, $(OBJDIR)%, $(SRCEMPTYWINDOW:.cpp=.o) )
FOBJPARTICLES     	= $(patsubst ./%, $(OBJDIR)%, $(SRCPARTICLES:.cpp=.o) )

FOBJEEPP			= $(FOBJMODULES) $(FOBJTEST) $(FOBJEEIV) $(FOBJFLUID) $(FOBJBNB) $(FOBJEMTPYWINDOW) $(FOBJPARTICLES)
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
	@mkdir -p $(OBJDIR)/src/helper/chipmunk
	@mkdir -p $(OBJDIR)/src/helper/chipmunk/constraints
	@mkdir -p $(OBJDIR)/src/helper/haikuttf
	@mkdir -p $(OBJDIR)/src/base
	@mkdir -p $(OBJDIR)/src/audio
	@mkdir -p $(OBJDIR)/src/gaming
	@mkdir -p $(OBJDIR)/src/gaming/mapeditor
	@mkdir -p $(OBJDIR)/src/graphics
	@mkdir -p $(OBJDIR)/src/graphics/renderer
	@mkdir -p $(OBJDIR)/src/math
	@mkdir -p $(OBJDIR)/src/system
	@mkdir -p $(OBJDIR)/src/ui
	@mkdir -p $(OBJDIR)/src/ui/tools
	@mkdir -p $(OBJDIR)/src/utils
	@mkdir -p $(OBJDIR)/src/window
	@mkdir -p $(OBJDIR)/src/window/backend/SDL
	@mkdir -p $(OBJDIR)/src/window/backend/SDL2
	@mkdir -p $(OBJDIR)/src/window/backend/null
	@mkdir -p $(OBJDIR)/src/window/backend/allegro5
	@mkdir -p $(OBJDIR)/src/window/platform/x11
	@mkdir -p $(OBJDIR)/src/window/platform/win
	@mkdir -p $(OBJDIR)/src/window/platform/osx
	@mkdir -p $(OBJDIR)/src/window/platform/null
	@mkdir -p $(OBJDIR)/src/physics
	@mkdir -p $(OBJDIR)/src/physics/constraints
	@mkdir -p $(OBJDIR)/src/test
	@mkdir -p $(OBJDIR)/src/test/empty_window
	@mkdir -p $(OBJDIR)/src/eeiv
	@mkdir -p $(OBJDIR)/src/fluid
	@mkdir -p $(OBJDIR)/src/bnb
	@mkdir -p $(OBJDIR)/src/particles
	@mkdir -p $(OBJDIR)/src/particles/objects
	@mkdir -p $(OBJDIR)/src/particles/gameobjects

lib: dirs $(LIB)

$(FOBJMODULES):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJHELPERS):
	$(CC) -o $@ -c $(patsubst $(OBJDIR)%.o,%.c,$@) $(CFLAGSEXT) -DSTBI_FAILURE_USERMSG -std=gnu99 $(HELPERSINC)
	@$(CC) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.c,$@) -DSTBI_FAILURE_USERMSG > $(patsubst %.o,%.d,$@) $(HELPERSINC)

$(FOBJTEST):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJEEIV):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJFLUID):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)


$(FOBJPARTICLES):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)


$(FOBJBNB):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJEMTPYWINDOW):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(EXE): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST)
	$(CPP) -o ./$(EXE) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST) $(LDFLAGS) $(LIBS)

$(EXEIV): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEEIV)
	$(CPP) -o ./$(EXEIV) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEEIV) $(LDFLAGS) $(LIBS)

$(EXEFLUID): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJFLUID)
	$(CPP) -o ./$(EXEFLUID) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJFLUID) $(LDFLAGS) $(LIBS)

$(EXEBNB): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJBNB)
	$(CPP) -o ./$(EXEBNB) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJBNB) $(LDFLAGS) $(LIBS)

$(EXEEMPTYWINDOW): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEMTPYWINDOW)
	$(CPP) -o ./$(EXEEMPTYWINDOW) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEMTPYWINDOW) $(LDFLAGS) $(LIBS)

$(EXEPARTICLES): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJPARTICLES)
	$(CPP) -o ./$(EXEPARTICLES) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJPARTICLES) $(LDFLAGS) $(LIBS)

libeepp-s.a: $(FOBJHELPERS) $(FOBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES)

libeepp.so: $(FOBJHELPERS) $(FOBJMODULES)
	$(CPP) $(LDFLAGS) -Wl,-soname,$(LIB).$(VERSION) -o $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES) $(LIBS)

os:
	@echo $(OS)

test: dirs $(EXE)

eeiv: dirs $(EXEIV)

fluid: dirs $(EXEFLUID)

bnb: dirs $(EXEBNB)

ew: dirs $(EXEEMPTYWINDOW)

particles: dirs $(EXEPARTICLES)

docs:
	doxygen ./Doxyfile

clean:
	@rm -rf $(FOBJALL) $(DEPSALL)

cleantemp:
	@rm -rf $(FOBJEEPP) $(DEPSEEPP)

cleanall: clean
	@rm -rf $(LIBNAME)
	@rm -rf ./$(EXE)
	@rm -rf ./$(EXEFLUID)
	@rm -rf ./$(EXEPARTICLES)
	@rm -rf ./$(EXEIV)
	@rm -rf ./$(EXEBNB)
	@rm -rf ./log.log

install:
	@($(CP) $(LIBNAME) $(DESTLIBDIR) $(INSTALL))

-include $(DEPSALL)
