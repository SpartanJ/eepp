STRLOWERCASE 		= $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

#cross-compiling support
ifeq ($(MINGW32),yes)
	BUILD_OS		= mingw32
else
	ifeq ($(IOS),yes)
		BUILD_OS	= ios
	else
		BUILD_OS 	= $(strip $(call STRLOWERCASE, $(shell uname) ) )
	endif
endif

export VERSION    	= 0.8
export CP         	= cp
export LN         	= ln
export LNFLAGS    	= -s -f
export ARFLAGS    	= rcs
export DESTDIR    	= /usr
export DESTLIBDIR 	= $(DESTDIR)/lib
export DESTINCDIR 	= $(DESTDIR)/include
export MKDIR		= mkdir -p
export RM			= rm -rf

FRAMEWORKFLAGS		=
STATIC_LIBS			=

ifeq ($(BUILD_OS), mingw32)

export AR         	= i686-w64-mingw32-ar
export CC         	= i686-w64-mingw32-gcc
export CPP        	= i686-w64-mingw32-g++
OSLIBEXTENSION		= dll

else

ifeq ($(BUILD_OS), ios)
	ifeq ($(IOSVERSION),)
		ifneq (,$(findstring 4.3,$(XCODE)))
			IOSVERSION	= 5.1
		else
			IOSVERSION	= 5.0
		endif
	endif

	ifeq ($(STATIC_FT2),)
		STATIC_FT2=yes
	endif
	
	#if TOOLCHAINPATH is empty
	ifeq ($(TOOLCHAINPATH),)
		ifeq ($(SIMULATOR),yes)
			ARCH = i386
			PARCHFLAGS = -m32 -march=i386
			PLATNAME = Simulator
		else
			ARCH = armv7
			PARCHFLAGS = -march=armv7 -marm -mcpu=cortex-a8 
			PLATNAME = OS
		endif

		#if xcode is 4.3
		ifneq (,$(findstring 4.3,$(XCODE)))
			TOOLCHAINPATH	= /Applications/Xcode.app/Contents/Developer/Platforms/iPhone$(PLATNAME).platform/Developer/usr/bin/
			SYSROOTPATH		= /Applications/Xcode.app/Contents/Developer/Platforms/iPhone$(PLATNAME).platform/Developer/SDKs/iPhone$(PLATNAME)$(IOSVERSION).sdk
			FRAMEWORKPATH	= /Applications/Xcode.app/Contents/Developer/Platforms/iPhone$(PLATNAME).platform/Developer/SDKs/iPhone$(PLATNAME)$(IOSVERSION).sdk/System/Library/Frameworks
		else
			TOOLCHAINPATH	= /Developer/Platforms/iPhone$(PLATNAME).platform/Developer/usr/bin/
			SYSROOTPATH		= /Developer/Platforms/iPhone$(PLATNAME).platform/Developer/SDKs/iPhone$(PLATNAME)$(IOSVERSION).sdk
			FRAMEWORKPATH	= /Developer/Platforms/iPhone$(PLATNAME).platform/Developer/SDKs/iPhone$(PLATNAME)$(IOSVERSION).sdk/System/Library/Frameworks
		endif

		export C_INCLUDE_PATH		= $(SYSROOTPATH)/usr/include
		export CPLUS_INCLUDE_PATH	= $(SYSROOTPATH)/usr/include
		export LIBRARY_PATH			= $(FRAMEWORKPATH)/usr/lib

		PLATFORMFLAGS = -arch ${ARCH} -miphoneos-version-min=$(IOSVERSION) -isysroot $(SYSROOTPATH) -I${C_INCLUDE_PATH}
		FRAMEWORKFLAGS += -arch ${ARCH} $(PARCHFLAGS) -F$(FRAMEWORKPATH) -L$(SYSROOTPATH)/usr/lib -isysroot $(SYSROOTPATH)
	endif
endif

export AR         	= $(TOOLCHAINPATH)ar

ifeq ($(LLVM_BUILD), yes)
	export CC         	= $(TOOLCHAINPATH)clang
	export CPP        	= $(TOOLCHAINPATH)clang++

else
	export CC         	= $(TOOLCHAINPATH)gcc
	export CPP        	= $(TOOLCHAINPATH)g++
endif

ifneq (,$(findstring cygwin,$(BUILD_OS)))
	OSLIBEXTENSION		= dll
else
	OSLIBEXTENSION		= so
endif

SDLCONFIGPATH		= 

endif

ifeq ($(ARCH),)
	ARCHEXT		=
else
	ARCHEXT		=-$(ARCH)
endif

LIBPATH		= ./libs/$(BUILD_OS)/$(RELEASETYPE)/
DYLIB		= libeepp$(ARCHEXT).$(OSLIBEXTENSION)

ifeq ($(DYNAMIC), yes)
	LIB     = $(DYLIB)
	LIBNAME = $(LIBPATH)$(LIB)
	INSTALL = && $(LN) $(LNFLAGS) $(DESTLIBDIR)/$(LIB).$(VERSION) $(DESTLIBDIR)/$(LIB)
else
	LIB		= libeepp$(ARCHEXT).a
	LIBNAME = $(LIBPATH)$(LIB)
	INSTALL = 
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
    BUILDFLAGS = -fPIC -DEE_EXPORTS
    LINKFLAGS  = -shared -DEE_EXPORTS
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
	ifeq ($(BUILD_OS), ios)
		TRY_SDL2 = yes
	else
		ifeq ($(BUILD_OS), darwin)
			TRY_SDL2 = yes
		else
			TRY_SDL2 = no
		endif
	endif
	
	ifeq ($(TRY_SDL2), yes)
		# First check for SDL2
		SDLVERSION2			= $(shell type -P $(SDLCONFIGPATH)sdl2-config &>/dev/null && $(SDLCONFIGPATH)sdl2-config --version || echo "")
		
		ifeq ($(SDLVERSION2),)
			# Then for SDL 1.2 or SDL 1.3
			SDLVERSION				= $(shell type -P $(SDLCONFIGPATH)sdl-config &>/dev/null && $(SDLCONFIGPATH)sdl-config --version || echo "")

			ifeq ($(SDLVERSION),)
				# Default 2.0.0
				SDL_VERSION		= 2.0.0
			else
				SDL_VERSION			= $(SDLVERSION)
			endif
		else
			SDL_VERSION		= $(SDLVERSION2)
		endif
	else
		# First check for SDL 1.2 or SDL 1.3
		SDLVERSION				= $(shell type -P $(SDLCONFIGPATH)sdl-config &>/dev/null && $(SDLCONFIGPATH)sdl-config --version || echo "")

		ifeq ($(SDLVERSION),)
			# Then for SDL 2
			SDLVERSION2			= $(shell type -P $(SDLCONFIGPATH)sdl2-config &>/dev/null && $(SDLCONFIGPATH)sdl2-config --version || echo "")
			
			ifeq ($(SDLVERSION2),)
				# Default 1.2
				SDL_VERSION		= 1.2
			else
				SDL_VERSION		= $(SDLVERSION2)
			endif
		else
			SDL_VERSION			= $(SDLVERSION)
		endif
	endif

	# If version is 1.2.x
	ifneq (,$(findstring 1.2,$(SDL_VERSION)))
		ifeq ($(BUILD_OS), darwin)
			SDL_ADD_LINK	= -framework Cocoa -lSDLmain
		else
			SDL_ADD_LINK	=
		endif
	
		SDL_BACKEND_LINK	= -lSDL $(SDL_ADD_LINK)

		SDL_BACKEND_SRC		= $(wildcard ./src/eepp/window/backend/SDL/*.cpp)

		EE_SDL_VERSION		= -DEE_SDL_VERSION_1_2
	else
		ifeq ($(SHARED_BACKEND),)
			#Check if static library exists
			SDL_STATIC_FOUND	= $(shell ls libs/$(BUILD_OS)/libSDL.a >/dev/null 2>&1 && echo "YES" || echo "NO")

			SDL2_STATIC_FOUND	= $(shell ls libs/$(BUILD_OS)/libSDL2.a >/dev/null 2>&1 && echo "YES" || echo "NO")

			ifeq ($(SDL_STATIC_FOUND),NO)
				ifeq ($(SDL2_STATIC_FOUND),NO)
					SHARED_BACKEND = yes
				endif
			endif
		endif

		# Compile as shared?
		ifeq ($(SHARED_BACKEND),yes)
			ifneq (,$(findstring 1.3,$(SDL_VERSION)))

				ifeq ($(BUILD_OS), darwin)
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
				SDL_BACKEND_LINK	= libs/$(BUILD_OS)/libSDL.a

				EE_SDL_VERSION		= -DEE_SDL_VERSION_1_3
			else
				# If version is 2.x.x
				SDL_BACKEND_LINK	= libs/$(BUILD_OS)/libSDL2$(ARCHEXT).a
				EE_SDL_VERSION		= -DEE_SDL_VERSION_2
			endif

			STATIC_LIBS			+= $(SDL_BACKEND_LINK)
		endif
		
		SDL_BACKEND_SRC		= $(wildcard ./src/eepp/window/backend/SDL2/*.cpp)
	endif
	
	SDL_DEFINE			= -DEE_BACKEND_SDL_ACTIVE $(EE_SDL_VERSION)
else
	SDL_BACKEND_LINK	= 
	SDL_BACKEND_SRC		= 
	SDL_DEFINE			= 
endif

ifeq ($(BACKEND_ALLEGRO), yes)
	ifeq ($(STATIC_ALLEGRO),)
		ifeq ($(BUILD_OS), darwin)
			ALLEGRO_BACKEND_LINK	= -lallegro -lallegro_main
		else
			ALLEGRO_BACKEND_LINK	= -lallegro
		endif
	else
		ALLEGRO_BACKEND_LINK	= libs/$(BUILD_OS)/liballegro$(ARCHEXT).a libs/$(BUILD_OS)/liballegro_main$(ARCHEXT).a
	endif

	ALLEGRO_BACKEND_SRC		= $(wildcard ./src/eepp/window/backend/allegro5/*.cpp)
	ALLEGRO_DEFINE			= -DEE_BACKEND_ALLEGRO_ACTIVE
else
	ALLEGRO_BACKEND_LINK	= 
	ALLEGRO_BACKEND_SRC		= 
	ALLEGRO_DEFINE			= 
endif

BACKENDFLAGS = $(SDL_DEFINE) $(ALLEGRO_DEFINE)

ifeq ($(LIBSNDFILE_ENABLE),yes)
	ifeq ($(MINGW32),yes)
		LIBSNDFILE	= -llibsndfile-1
	else
		#if it is cygwin
		ifneq (,$(findstring cygwin,$(BUILD_OS)))
			LIBSNDFILE	= -llibsndfile-1
		else
			LIBSNDFILE	= -lsndfile
		endif
	endif
	
	SNDFILEFLAG = -DEE_LIBSNDFILE_ENABLED
else
	LIBSNDFILE	=
	SNDFILEFLAG =
endif

ifeq ($(STATIC_FT2),yes)
	LIBFREETYPE2	= 
	INCFREETYPE2	= -I./src/eepp/helper/freetype2/include
else
	LIBFREETYPE2	= -lfreetype
	
	ifneq (,$(findstring cygwin,$(BUILD_OS)))
		INCFREETYPE2	= -I./src/eepp/helper/freetype2/include
	else
		INCFREETYPE2	= -I$(DESTINCDIR)/freetype2
	endif
endif

ifeq ($(BUILD_OS), ios)

	ifeq ($(BACKEND_SDL),yes)
		BACKENDINCLUDE = -I./src/eepp/helper/SDL2/include
	else
		BACKENDINCLUDE = -I./src/eepp/helper/allegro5/include
	endif

	PLATFORMFLAGS += $(BACKENDINCLUDE)

	ifneq ($(GLES2), yes)
		ifneq ($(GLES1), yes)
			GLES1=yes
		endif
	endif
endif

FINALFLAGS = $(DEBUGFLAGS) $(SNDFILEFLAG)

ifeq ($(GLES2), yes)
	ifneq ($(GLES1), yes)
		FINALFLAGS += -DEE_GLES2 -DSOIL_GLES2
		
		GL_VERSION = GLES2
	else
		FINALFLAGS += -DEE_GLES1 -DSOIL_GLES1 -DEE_GLES2 -DSOIL_GLES2
		
		GL_VERSION = GLES
	endif
else
	ifeq ($(GLES1), yes)
		FINALFLAGS += -DEE_GLES1 -DSOIL_GLES1
		
		GL_VERSION = GLES1
	else
		GL_VERSION = GL
	endif
endif

BASEINC		= -I./include/ -I./src/
OTHERINC	= $(BASEINC)
BININC		= -I./include/

##################### OS BUILD OPTIONS #####################
ifeq ($(BUILD_OS), linux)

LIBS 		= -lrt -lpthread -lX11 -lopenal -lGL -lXcursor $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK) $(LIBFREETYPE2)
OTHERINC	+= $(INCFREETYPE2)
PLATFORMSRC	= $(wildcard ./src/eepp/window/platform/x11/*.cpp) $(wildcard ./src/eepp/system/platform/posix/*.cpp)

else

ifeq ($(BUILD_OS), darwin)

LIBS 		= -framework OpenGL -framework OpenAL -framework CoreFoundation -framework AGL $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK) $(LIBFREETYPE2)
OTHERINC	+= $(INCFREETYPE2) -I/usr/local/include/freetype2
PLATFORMSRC = $(wildcard ./src/eepp/window/platform/osx/*.cpp) $(wildcard ./src/eepp/system/platform/posix/*.cpp)

else

ifeq ($(BUILD_OS), haiku)

LIBS 		= -lopenal -lGL $(SDL_BACKEND_LINK) $(LIBFREETYPE2)
OTHERINC	+= $(INCFREETYPE2)
PLATFORMSRC	= $(wildcard ./src/eepp/system/platform/posix/*.cpp)

else

ifeq ($(BUILD_OS), freebsd)

LIBS 		= -lrt -lpthread -lX11 -lopenal -lGL -lXcursor $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK) $(LIBFREETYPE2)
OTHERINC	+= $(INCFREETYPE2)
PLATFORMSRC	= $(wildcard ./src/eepp/window/platform/x11/*.cpp) $(wildcard ./src/eepp/system/platform/posix/*.cpp)

else

ifeq ($(BUILD_OS), mingw32)

LIBS 		= -lOpenAL32 -lopengl32 -lmingw32 -lglu32 -lgdi32 -static-libgcc -static-libstdc++ -mwindows $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK) $(LIBFREETYPE2)
OTHERINC	+= $(INCFREETYPE2)
PLATFORMSRC	= $(wildcard ./src/eepp/window/platform/win/*.cpp) $(wildcard ./src/eepp/system/platform/win/*.cpp)

else

#if it is cygwin
ifneq (,$(findstring cygwin,$(BUILD_OS)))

LIBS 		= -lOpenAL32 -lmingw32 -lopengl32 -lglu32 -lgdi32 -static-libgcc -mwindows $(LIBSNDFILE) $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK) $(LIBFREETYPE2)
OTHERINC	+= -I./src/eepp/helper/zlib $(INCFREETYPE2)
PLATFORMSRC	= $(wildcard ./src/eepp/window/platform/win/*.cpp) $(wildcard ./src/eepp/system/platform/win/*.cpp)

else

ifeq ($(BUILD_OS), ios)

LIBS 		= -static-libgcc -static-libstdc++ -framework OpenGLES -framework OpenAL -framework AudioToolbox -framework CoreAudio -framework Foundation -framework CoreFoundation -framework UIKit -framework QuartzCore -framework CoreGraphics $(SDL_BACKEND_LINK) $(ALLEGRO_BACKEND_LINK)
OTHERINC	+= $(INCFREETYPE2)

ifeq ($(ARCH),armv7)
OTHERINC += -DU_HAVE_GCC_ATOMICS=0
endif

PLATFORMSRC = $(wildcard ./src/eepp/system/platform/posix/*.cpp)

endif
#endif ios

endif
#endif cygwin

endif
#endif mingw32

endif
#endif freebsd

endif
#endif haiku

endif
#endif darwin

endif
#endif linux
##################### OS BUILD OPTIONS #####################

export CFLAGS     	= $(ARCHFLAGS) -Wall -Wno-unknown-pragmas $(FINALFLAGS) $(BUILDFLAGS) $(BACKENDFLAGS) $(PLATFORMFLAGS)
export CFLAGSEXT  	= $(ARCHFLAGS) $(FINALFLAGS) $(BUILDFLAGS) $(PLATFORMFLAGS)
export LDFLAGS    	= $(ARCHFLAGS) $(LINKFLAGS) $(FRAMEWORKFLAGS)
HELPERSFLAGS		= -DSTBI_FAILURE_USERMSG -DFT2_BUILD_LIBRARY
HELPERSINC			= -I./include/eepp/helper/chipmunk -I./src/eepp/helper/zlib -I./src/eepp/helper/freetype2/include -I./include/eepp/helper/SOIL -I./include/eepp/helper/glew

ifeq ($(BUILD_OS), mingw32)
	OSEXTENSION			= .exe
else
	OSEXTENSION			= 
endif

ifeq ($(BUILD_OS), haiku)
	SRCGLEW 			= 
else
	ifeq ($(BUILD_OS), ios)
		SRCGLEW 			= 
	else
		SRCGLEW 			= $(wildcard ./src/eepp/helper/glew/*.c)
	endif
endif

ifeq ($(STATIC_FT2), yes)
	SRCFREETYPE			= $(wildcard ./src/eepp/helper/freetype2/src/*/*.c)
else
	SRCFREETYPE			= 
endif

SRCHELPERS			= $(SRCFREETYPE) $(SRCGLEW) $(wildcard ./src/eepp/helper/SOIL/*.c) $(wildcard ./src/eepp/helper/stb_vorbis/*.c) $(wildcard ./src/eepp/helper/zlib/*.c) $(wildcard ./src/eepp/helper/libzip/*.c) $(wildcard ./src/eepp/helper/chipmunk/*.c) $(wildcard ./src/eepp/helper/chipmunk/constraints/*.c)
SRCMODULES			= $(wildcard ./src/eepp/helper/haikuttf/*.cpp) $(wildcard ./src/eepp/base/*.cpp) $(wildcard ./src/eepp/audio/*.cpp) $(wildcard ./src/eepp/gaming/*.cpp) $(wildcard ./src/eepp/gaming/mapeditor/*.cpp) $(wildcard ./src/eepp/graphics/*.cpp) $(wildcard ./src/eepp/graphics/renderer/*.cpp) $(wildcard ./src/eepp/math/*.cpp) $(wildcard ./src/eepp/system/*.cpp) $(wildcard ./src/eepp/ui/*.cpp) $(wildcard ./src/eepp/ui/tools/*.cpp) $(wildcard ./src/eepp/utils/*.cpp) $(wildcard ./src/eepp/window/*.cpp) $(wildcard ./src/eepp/window/backend/null/*.cpp) $(wildcard ./src/eepp/window/platform/null/*.cpp) $(SDL_BACKEND_SRC) $(ALLEGRO_BACKEND_SRC) $(PLATFORMSRC) $(wildcard ./src/eepp/physics/*.cpp) $(wildcard ./src/eepp/physics/constraints/*.cpp)

OBJHELPERS			= $(SRCHELPERS:.c=.o)
OBJMODULES			= $(SRCMODULES:.cpp=.o)

ifeq ($(ARCH),)
	OBJDIR				= obj/$(BUILD_OS)/$(RELEASETYPE)/
else
	ifeq ($(BUILD_OS), ios)
		OBJDIR				= obj/$(BUILD_OS)/$(RELEASETYPE)/$(ARCH)/$(GL_VERSION)/
	else
		OBJDIR				= obj/$(BUILD_OS)/$(RELEASETYPE)/$(ARCH)/
	endif
endif

FOBJHELPERS			= $(patsubst ./%, $(OBJDIR)%, $(OBJHELPERS) )
FOBJMODULES			= $(patsubst ./%, $(OBJDIR)%, $(OBJMODULES) )

# OUT OF EEPP LIB
SRCTEST     		= $(wildcard ./src/test/*.cpp)
SRCEEIV     		= $(wildcard ./src/eeiv/*.cpp)
SRCFLUID     		= $(wildcard ./src/fluid/*.cpp)
SRCPARTICLES    	= $(wildcard ./src/particles/*.cpp) $(wildcard ./src/eepp/particles/objects/*.cpp) $(wildcard ./src/eepp/particles/gameobjects/*.cpp)
SRCBNB     			= $(wildcard ./src/bnb/*.cpp)
SRCEMPTYWINDOW  	= $(wildcard ./src/examples/empty_window/*.cpp)
SRCRHYTHM		  	= $(wildcard ./src/rhythm/*.cpp)

OBJTEST     		= $(SRCTEST:.cpp=.o)
OBJEEIV     		= $(SRCEEIV:.cpp=.o)
OBJFLUID     		= $(SRCFLUID:.cpp=.o)
OBJBNB     			= $(SRCBNB:.cpp=.o)
OBJEMPTYWINDOW		= $(SRCEMPTYWINDOW:.cpp=.o)
OBJPARTICLES     	= $(SRCPARTICLES:.cpp=.o)
OBJRHYTHM			= $(SRCRHYTHM:.cpp=.o)

FOBJTEST     		= $(patsubst ./%, $(OBJDIR)%, $(SRCTEST:.cpp=.o) )
FOBJEEIV     		= $(patsubst ./%, $(OBJDIR)%, $(SRCEEIV:.cpp=.o) )
FOBJFLUID     		= $(patsubst ./%, $(OBJDIR)%, $(SRCFLUID:.cpp=.o) )
FOBJBNB     		= $(patsubst ./%, $(OBJDIR)%, $(SRCBNB:.cpp=.o) )
FOBJEMTPYWINDOW     = $(patsubst ./%, $(OBJDIR)%, $(SRCEMPTYWINDOW:.cpp=.o) )
FOBJPARTICLES     	= $(patsubst ./%, $(OBJDIR)%, $(SRCPARTICLES:.cpp=.o) )
FOBJRHYTHM		 	= $(patsubst ./%, $(OBJDIR)%, $(SRCRHYTHM:.cpp=.o) )

EXE     			= eetest-$(RELEASETYPE)$(OSEXTENSION)
EXEIV				= eeiv-$(RELEASETYPE)$(OSEXTENSION)
EXEFLUID			= eefluid-$(RELEASETYPE)$(OSEXTENSION)
EXEBNB				= bnb-$(RELEASETYPE)$(OSEXTENSION)
EXEEMPTYWINDOW		= eeew-$(RELEASETYPE)$(OSEXTENSION)
EXEPARTICLES		= eeparticles-$(RELEASETYPE)$(OSEXTENSION)
EXERHYTHM			= rhythm-$(RELEASETYPE)$(OSEXTENSION)
# OUT OF EEPP LIB

FOBJEEPP			= $(FOBJMODULES) $(FOBJTEST) $(FOBJEEIV) $(FOBJFLUID) $(FOBJBNB) $(FOBJEMTPYWINDOW) $(FOBJPARTICLES) $(FOBJRHYTHM)
FOBJALL 			= $(FOBJHELPERS) $(FOBJEEPP)

DEPSEEPP			= $(FOBJEEPP:.o=.d)
DEPSALL				= $(FOBJALL:.o=.d)

all: lib

dirs:
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/psaux
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/SOIL
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/zlib
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/autofit
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/base
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/bdf
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/bzip2
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/cache
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/cff
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/cid
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/gxvalid
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/gzip
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/lzw
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/otvalid
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/pcf
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/pfr
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/pshinter
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/psnames
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/raster
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/sfnt
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/smooth
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/truetype
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/type1
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/type42
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/freetype2/src/winfonts
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/glew
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/stb_vorbis
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/libzip
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/chipmunk
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/chipmunk/constraints
	@$(MKDIR) $(OBJDIR)/src/eepp/helper/haikuttf
	@$(MKDIR) $(LIBPATH)
	@$(MKDIR) $(OBJDIR)/src/eepp/base
	@$(MKDIR) $(OBJDIR)/src/eepp/audio
	@$(MKDIR) $(OBJDIR)/src/eepp/gaming
	@$(MKDIR) $(OBJDIR)/src/eepp/gaming/mapeditor
	@$(MKDIR) $(OBJDIR)/src/eepp/graphics
	@$(MKDIR) $(OBJDIR)/src/eepp/graphics/renderer
	@$(MKDIR) $(OBJDIR)/src/eepp/math
	@$(MKDIR) $(OBJDIR)/src/eepp/system
	@$(MKDIR) $(OBJDIR)/src/eepp/system/platform/posix
	@$(MKDIR) $(OBJDIR)/src/eepp/system/platform/win
	@$(MKDIR) $(OBJDIR)/src/eepp/ui
	@$(MKDIR) $(OBJDIR)/src/eepp/ui/tools
	@$(MKDIR) $(OBJDIR)/src/eepp/utils
	@$(MKDIR) $(OBJDIR)/src/eepp/window
	@$(MKDIR) $(OBJDIR)/src/eepp/window/backend/SDL
	@$(MKDIR) $(OBJDIR)/src/eepp/window/backend/SDL2
	@$(MKDIR) $(OBJDIR)/src/eepp/window/backend/null
	@$(MKDIR) $(OBJDIR)/src/eepp/window/backend/allegro5
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/x11
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/win
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/osx
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/null
	@$(MKDIR) $(OBJDIR)/src/eepp/physics
	@$(MKDIR) $(OBJDIR)/src/eepp/physics/constraints
	@$(MKDIR) $(OBJDIR)/src/test
	@$(MKDIR) $(OBJDIR)/src/examples/empty_window
	@$(MKDIR) $(OBJDIR)/src/eeiv
	@$(MKDIR) $(OBJDIR)/src/fluid
	@$(MKDIR) $(OBJDIR)/src/bnb
	@$(MKDIR) $(OBJDIR)/src/particles
	@$(MKDIR) $(OBJDIR)/src/particles/objects
	@$(MKDIR) $(OBJDIR)/src/particles/gameobjects
	@$(MKDIR) $(OBJDIR)/src/rhythm

lib: dirs $(LIB)

$(FOBJMODULES):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJHELPERS):
	$(CC) -o $@ -c $(patsubst $(OBJDIR)%.o,%.c,$@) $(CFLAGSEXT) $(HELPERSFLAGS) -std=gnu99 $(HELPERSINC)
	@$(CC) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.c,$@) $(HELPERSFLAGS) > $(patsubst %.o,%.d,$@) $(HELPERSINC)

$(FOBJTEST):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(BININC) > $(patsubst %.o,%.d,$@)

$(FOBJEEIV):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJFLUID):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJPARTICLES):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJBNB):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJEMTPYWINDOW):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJRHYTHM):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
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

$(EXERHYTHM): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJRHYTHM)
	$(CPP) -o ./$(EXERHYTHM) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJRHYTHM) $(LDFLAGS) $(LIBS)


libeepp-$(ARCH).a: $(FOBJHELPERS) $(FOBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES)

libeepp.a: $(FOBJHELPERS) $(FOBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES)

$(DYLIB): $(FOBJHELPERS) $(FOBJMODULES)
	$(CPP) $(LDFLAGS) -Wl,-soname,$(LIB).$(VERSION) -o $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES) $(LIBS)

os:
	@echo $(BUILD_OS)

objdir:
	@echo $(OBJDIR)

test: dirs $(EXE)

eeiv: dirs $(EXEIV)

fluid: dirs $(EXEFLUID)

bnb: dirs $(EXEBNB)

ew: dirs $(EXEEMPTYWINDOW)

particles: dirs $(EXEPARTICLES)

rhythm: dirs $(EXERHYTHM)

docs:
	doxygen ./Doxyfile

clean:
	$(RM) $(FOBJALL) $(DEPSALL)

cleantemp:
	@$(RM) $(FOBJEEPP) $(DEPSEEPP)

cleanall: clean
	@$(RM) $(LIBNAME)
	@$(RM) ./$(EXE)
	@$(RM) ./$(EXEFLUID)
	@$(RM) ./$(EXEPARTICLES)
	@$(RM) ./$(EXEIV)
	@$(RM) ./$(EXEBNB)
	@$(RM) ./log.log

depends:
	@echo $(DEPSALL)

install:
	@($(CP) $(LIBNAME) $(DESTLIBDIR) $(INSTALL))

-include $(DEPSALL)
