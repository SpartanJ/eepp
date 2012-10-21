-include Makefile.base

# OUT OF EEPP LIB
SRCTEST     		= $(wildcard ./src/test/*.cpp)
SRCEMPTYWINDOW  	= $(wildcard ./src/examples/empty_window/*.cpp)
SRCEXTSHADER	  	= $(wildcard ./src/examples/external_shader/*.cpp)

OBJTEST     		= $(SRCTEST:.cpp=.o)
OBJEMPTYWINDOW		= $(SRCEMPTYWINDOW:.cpp=.o)
OBJEEXTSHADER		= $(SRCEXTSHADER:.cpp=.o)

FOBJTEST     		= $(patsubst ./%, $(OBJDIR)%, $(SRCTEST:.cpp=.o) )
FOBJEMTPYWINDOW     = $(patsubst ./%, $(OBJDIR)%, $(SRCEMPTYWINDOW:.cpp=.o) )
FOBJEXTSHADER    	= $(patsubst ./%, $(OBJDIR)%, $(SRCEXTSHADER:.cpp=.o) )

EXETEST    			= eetest-$(RELEASETYPE)$(OSEXTENSION)
EXEEMPTYWINDOW		= eeew-$(RELEASETYPE)$(OSEXTENSION)
EXEEXTSHADER		= eees-$(RELEASETYPE)$(OSEXTENSION)
# OUT OF EEPP LIB

FOBJEEPP			= $(FOBJMODULES) $(FOBJTEST) $(FOBJEMTPYWINDOW) $(FOBJEXTSHADER) $(FOBJPARTICLES) $(FOBJRHYTHM) $(FOBJEEIV) $(FOBJFLUID)
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
	@$(MKDIR) $(OBJDIR)/src/eepp/window/backend/SFML
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/x11
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/win
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/osx
	@$(MKDIR) $(OBJDIR)/src/eepp/window/platform/null
	@$(MKDIR) $(OBJDIR)/src/eepp/physics
	@$(MKDIR) $(OBJDIR)/src/eepp/physics/constraints
	@$(MKDIR) $(OBJDIR)/src/test
	@$(MKDIR) $(OBJDIR)/src/examples/empty_window
	@$(MKDIR) $(OBJDIR)/src/examples/external_shader

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

$(FOBJEMTPYWINDOW):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(FOBJEXTSHADER):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(BININC)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@)

$(EXETEST): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST)
	$(CPP) -o ./$(EXETEST) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST) $(LDFLAGS) $(LIBS)

$(EXEEMPTYWINDOW): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEMTPYWINDOW)
	$(CPP) -o ./$(EXEEMPTYWINDOW) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEMTPYWINDOW) $(LDFLAGS) $(LIBS)

$(EXEEXTSHADER): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEXTSHADER)
	$(CPP) -o ./$(EXEEXTSHADER) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEXTSHADER) $(LDFLAGS) $(LIBS)

libeepp-$(ARCH).a: $(FOBJHELPERS) $(FOBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES)

libeepp.a: $(FOBJHELPERS) $(FOBJMODULES)
	$(AR) $(ARFLAGS) $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES)

$(DYLIB): $(FOBJHELPERS) $(FOBJMODULES)
	$(CPP) $(LDFLAGS) $(DYLIB_EXTRA) -o $(LIBNAME) $(FOBJHELPERS) $(FOBJMODULES) $(LIBS)

os:
	@echo $(BUILD_OS)

objdir:
	@echo $(OBJDIR)

test: dirs $(EXETEST)

ew: dirs $(EXEEMPTYWINDOW)

es: dirs $(EXEEXTSHADER)

docs:
	doxygen ./Doxyfile

clean:
	$(RM) $(FOBJALL) $(DEPSALL)

cleantemp:
	@$(RM) $(FOBJEEPP) $(DEPSEEPP)

cleanall: clean
	@$(RM) $(LIBNAME)
	@$(RM) ./$(EXETEST)
	@$(RM) ./$(EXEEMPTYWINDOW)
	@$(RM) ./$(EXEEXTSHADER)
	@$(RM) ./log.log

depends:
	@echo $(DEPSALL)

install:
	@($(CP) $(LIBNAME) $(DESTLIBDIR) $(INSTALL))

-include $(DEPSALL)
