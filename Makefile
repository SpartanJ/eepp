-include Makefile.base

# OUT OF EEPP LIB
SRCTEST     		= $(wildcard ./src/test/*.cpp)
SRCEMPTYWINDOW  	= $(wildcard ./src/examples/empty_window/*.cpp)
SRCEXTSHADER	  	= $(wildcard ./src/examples/external_shader/*.cpp)

SRCEEIV     		= $(wildcard ./src/eeiv/*.cpp)
SRCFLUID     		= $(wildcard ./src/fluid/*.cpp)
SRCPARTICLES    	= $(wildcard ./src/particles/*.cpp) $(wildcard ./src/eepp/particles/objects/*.cpp) $(wildcard ./src/eepp/particles/gameobjects/*.cpp)
SRCBNB     			= $(wildcard ./src/bnb/*.cpp)
SRCRHYTHM		  	= $(wildcard ./src/rhythm/*.cpp)

OBJTEST     		= $(SRCTEST:.cpp=.o)
OBJEMPTYWINDOW		= $(SRCEMPTYWINDOW:.cpp=.o)
OBJEEXTSHADER		= $(SRCEXTSHADER:.cpp=.o)

OBJEEIV     		= $(SRCEEIV:.cpp=.o)
OBJFLUID     		= $(SRCFLUID:.cpp=.o)
OBJBNB     			= $(SRCBNB:.cpp=.o)
OBJPARTICLES     	= $(SRCPARTICLES:.cpp=.o)
OBJRHYTHM			= $(SRCRHYTHM:.cpp=.o)

FOBJTEST     		= $(patsubst ./%, $(OBJDIR)%, $(SRCTEST:.cpp=.o) )
FOBJEMTPYWINDOW     = $(patsubst ./%, $(OBJDIR)%, $(SRCEMPTYWINDOW:.cpp=.o) )
FOBJEXTSHADER    	= $(patsubst ./%, $(OBJDIR)%, $(SRCEXTSHADER:.cpp=.o) )

FOBJEEIV     		= $(patsubst ./%, $(OBJDIR)%, $(SRCEEIV:.cpp=.o) )
FOBJFLUID     		= $(patsubst ./%, $(OBJDIR)%, $(SRCFLUID:.cpp=.o) )
FOBJBNB     		= $(patsubst ./%, $(OBJDIR)%, $(SRCBNB:.cpp=.o) )
FOBJPARTICLES     	= $(patsubst ./%, $(OBJDIR)%, $(SRCPARTICLES:.cpp=.o) )
FOBJRHYTHM		 	= $(patsubst ./%, $(OBJDIR)%, $(SRCRHYTHM:.cpp=.o) )


EXE     			= eetest-$(RELEASETYPE)$(OSEXTENSION)
EXEEMPTYWINDOW		= eeew-$(RELEASETYPE)$(OSEXTENSION)
EXEEXTSHADER		= eees-$(RELEASETYPE)$(OSEXTENSION)

EXEIV				= eeiv-$(RELEASETYPE)$(OSEXTENSION)
EXEFLUID			= eefluid-$(RELEASETYPE)$(OSEXTENSION)
EXEBNB				= bnb-$(RELEASETYPE)$(OSEXTENSION)
EXEPARTICLES		= eeparticles-$(RELEASETYPE)$(OSEXTENSION)
EXERHYTHM			= rhythm-$(RELEASETYPE)$(OSEXTENSION)
# OUT OF EEPP LIB

FOBJEEPP			= $(FOBJMODULES) $(FOBJTEST) $(FOBJEMTPYWINDOW) $(FOBJEXTSHADER) $(FOBJPARTICLES) $(FOBJRHYTHM) $(FOBJEEIV) $(FOBJFLUID) $(FOBJBNB) 
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
	@$(MKDIR) $(OBJDIR)/src/examples/external_shader
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

$(FOBJEXTSHADER):
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

$(EXEEXTSHADER): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEXTSHADER)
	$(CPP) -o ./$(EXEEXTSHADER) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJEXTSHADER) $(LDFLAGS) $(LIBS)

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

es: dirs $(EXEEXTSHADER)

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
