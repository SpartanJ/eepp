LOCAL_PATH				:= $(call my-dir)
EEPP_BASE_PATH			:= $(LOCAL_PATH)/../../../../src
EEPP_PATH				:= $(LOCAL_PATH)/../../../../src/eepp
EEPP_INC_PATH			:= $(LOCAL_PATH)/../../../../include
EEPP_THIRD_PARTY_PATH	:= $(EEPP_BASE_PATH)/thirdparty
EEPP_MODULES_PATH		:= $(EEPP_BASE_PATH)/modules
SDL_PATH				:= $(EEPP_THIRD_PARTY_PATH)/SDL2
SDL_MAIN_PATH			:= $(SDL_PATH)/src/main/android/*.c

EEPP_C_INCLUDES			:= \
	$(EEPP_THIRD_PARTY_PATH) \
	$(EEPP_THIRD_PARTY_PATH)/freetype2/include \
	$(EEPP_THIRD_PARTY_PATH)/libpng \
	$(SDL_PATH)/include \
	$(EEPP_THIRD_PARTY_PATH)/chipmunk \
	$(EEPP_INC_PATH)/eepp/thirdparty \
	$(EEPP_INC_PATH)/eepp/thirdparty/chipmunk \
	$(EEPP_THIRD_PARTY_PATH)/SOIL2/src/SOIL2 \
	$(EEPP_THIRD_PARTY_PATH)/stb_vorbis \
	$(EEPP_THIRD_PARTY_PATH)/libvorbis/lib \
	$(EEPP_THIRD_PARTY_PATH)/libvorbis/include \
	$(EEPP_THIRD_PARTY_PATH)/libogg/include \
	$(EEPP_THIRD_PARTY_PATH)/mbedtls/include \
	$(EEPP_THIRD_PARTY_PATH)/mojoAL \
	$(EEPP_THIRD_PARTY_PATH)/efsw/include \
	$(EEPP_BASE_PATH)/modules/eterm/include \
	$(EEPP_BASE_PATH)/modules/eterm/src \
	$(EEPP_BASE_PATH)/modules/maps/include \
	$(EEPP_BASE_PATH)/modules/maps/src

EEPP_C_FLAGS				:= \
	-Wl,--undefined=Java_org_libsdl_app_SDLActivity_nativeInit \
	-DANDROID \
	-DANDROID_NDK \
	-DDISABLE_IMPORTGL \
	-Wall \
	-Wno-unknown-pragmas \
	$(EE_GLES_VERSION) \
	-DEE_NO_SNDFILE \
	-DEE_SSL_SUPPORT \
	-DEE_MBEDTLS \
	-D$(EE_SDL_VERSION) \
	-DAL_LIBTYPE_STATIC \
	-I$(EEPP_INC_PATH) \
	-I$(EEPP_BASE_PATH)

EEPP_LDLIBS				:= $(APP_LDLIBS)

include $(call all-subdir-makefiles)

#*************** EEPP ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_PATH)

LOCAL_MODULE			:= eepp

LOCAL_CFLAGS			:= $(EEPP_C_FLAGS)

CODE_SRCS				:=  \
	../thirdparty/SOIL2/src/SOIL2/*.c \
	../thirdparty/zlib/*.c \
	../thirdparty/libzip/*.c \
	../thirdparty/jpeg-compressor/*.cpp \
	../thirdparty/imageresampler/*.cpp \
	../thirdparty/pugixml/*.cpp \
	../thirdparty/libogg/src/*.c \
	../thirdparty/libvorbis/lib/*.c \
	../thirdparty/mbedtls/library/*.c \
	../thirdparty/mojoAL/*.c \
	system/*.cpp \
	system/platform/posix/*.cpp \
	network/*.cpp \
	network/http/*.cpp \
	network/ssl/*.cpp \
	network/ssl/backend/mbedtls/*.cpp \
	network/platform/unix/*.cpp \
	core/*.cpp \
	math/*.cpp \
	audio/*.cpp \
	window/*.cpp \
	window/backend/SDL2/*.cpp \
	graphics/*.cpp \
	graphics/renderer/*.cpp \
	physics/*.cpp \
	physics/constraints/*.cpp \
	scene/*.cpp \
	scene/actions/*.cpp \
	ui/*.cpp \
	ui/css/*.cpp \
	ui/doc/*.cpp \
	ui/abstract/*.cpp \
	ui/models/*.cpp \
	ui/tools/*.cpp

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

LOCAL_SRC_FILES			:= $(foreach F, $(CODE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= chipmunk freetype libpng

LOCAL_SHARED_LIBRARIES	:= SDL2

include $(BUILD_STATIC_LIBRARY)
#*************** EEPP ***************

#*************** CHIPMUNK ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_THIRD_PARTY_PATH)

LOCAL_MODULE			:= chipmunk

LOCAL_CFLAGS			:= \
	-DANDROID_NDK \
	-DDISABLE_IMPORTGL \
	-std=gnu99 \
	-Wall \
	-Wno-unknown-pragmas

CHIPMUNK_SRCS			:=  \
	chipmunk/*.c \
	chipmunk/constraints/*.c

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

LOCAL_SRC_FILES			:= $(foreach F, $(CHIPMUNK_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

include $(BUILD_STATIC_LIBRARY)
#*************** CHIPMUNK ***************

#*************** FREETYPE ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_THIRD_PARTY_PATH)/freetype2

LOCAL_MODULE			:= freetype

APP_SUBDIRS				:= $(patsubst $(LOCAL_PATH)/%, %, $(shell find $(LOCAL_PATH)/src -type d))

LOCAL_C_INCLUDES		:= $(foreach D, $(APP_SUBDIRS), $(LOCAL_PATH)/$(D)) $(LOCAL_PATH)/include $(EEPP_THIRD_PARTY_PATH)/libpng
LOCAL_CFLAGS			:= -Os -DFT2_BUILD_LIBRARY

LOCAL_SRC_FILES			+= $(foreach F, $(APP_SUBDIRS), $(addprefix $(F)/,$(notdir $(wildcard $(LOCAL_PATH)/$(F)/*.c))))

include $(BUILD_STATIC_LIBRARY)
#*************** FREETYPE ***************

#*************** LIBPNG ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_THIRD_PARTY_PATH)

LOCAL_MODULE			:= libpng

LIBPNG_SRCS			:=  \
	libpng/*.c \
	libpng/arm/*.c \
	libpng/intel/*.c \
	libpng/mips/*.c \
	libpng/powerpc/*.c

LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/libpng/
LOCAL_CFLAGS			:= -Os

LOCAL_SRC_FILES			:= $(foreach F, $(LIBPNG_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

include $(BUILD_STATIC_LIBRARY)
#*************** LIBPNG ***************

#**************** SDL 2 ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SDL_PATH)

LOCAL_MODULE := SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/src/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/dummy/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/openslES/*.c) \
	$(LOCAL_PATH)/src/atomic/SDL_atomic.c.arm \
	$(LOCAL_PATH)/src/atomic/SDL_spinlock.c.arm \
	$(wildcard $(LOCAL_PATH)/src/core/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/cpuinfo/*.c) \
	$(wildcard $(LOCAL_PATH)/src/dynapi/*.c) \
	$(wildcard $(LOCAL_PATH)/src/events/*.c) \
	$(wildcard $(LOCAL_PATH)/src/file/*.c) \
	$(wildcard $(LOCAL_PATH)/src/haptic/*.c) \
	$(wildcard $(LOCAL_PATH)/src/haptic/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/joystick/*.c) \
	$(wildcard $(LOCAL_PATH)/src/joystick/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/joystick/hidapi/*.c) \
	$(wildcard $(LOCAL_PATH)/src/loadso/dlopen/*.c) \
	$(wildcard $(LOCAL_PATH)/src/power/*.c) \
	$(wildcard $(LOCAL_PATH)/src/power/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/filesystem/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/sensor/*.c) \
	$(wildcard $(LOCAL_PATH)/src/sensor/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/render/*.c) \
	$(wildcard $(LOCAL_PATH)/src/render/*/*.c) \
	$(wildcard $(LOCAL_PATH)/src/stdlib/*.c) \
	$(wildcard $(LOCAL_PATH)/src/thread/*.c) \
	$(wildcard $(LOCAL_PATH)/src/thread/pthread/*.c) \
	$(wildcard $(LOCAL_PATH)/src/timer/*.c) \
	$(wildcard $(LOCAL_PATH)/src/timer/unix/*.c) \
	$(wildcard $(LOCAL_PATH)/src/video/*.c) \
	$(wildcard $(LOCAL_PATH)/src/video/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/video/yuv2rgb/*.c) \
	$(wildcard $(LOCAL_PATH)/src/test/*.c))

LOCAL_SHARED_LIBRARIES := hidapi

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES
LOCAL_CFLAGS += \
	-Wall -Wextra \
	-Wdocumentation \
	-Wdocumentation-unknown-command \
	-Wmissing-prototypes \
	-Wunreachable-code-break \
	-Wunneeded-internal-declaration \
	-Wmissing-variable-declarations \
	-Wfloat-conversion \
	-Wshorten-64-to-32 \
	-Wunreachable-code-return \
	-Wshift-sign-overflow \
	-Wstrict-prototypes \
	-Wkeyword-macro \


# Warnings we haven't fixed (yet)
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare


LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

ifeq ($(NDK_DEBUG),1)
    cmd-strip :=
endif

LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_SHARED_LIBRARY)

###########################
#
# hidapi library
#
###########################

include $(CLEAR_VARS)

LOCAL_PATH				:= $(SDL_PATH)

LOCAL_CPPFLAGS += -std=c++11

LOCAL_SRC_FILES := src/hidapi/android/hid.cpp

LOCAL_MODULE := libhidapi
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/cpufeatures)
#**************** SDL 2 ***************

#*************** EFSW ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_THIRD_PARTY_PATH)

LOCAL_MODULE			:= efsw

LIBEFSW_SRCS			:=  \
	efsw/src/efsw/*.cpp \
	efsw/src/efsw/platform/posix/*.cpp

LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/efsw/include $(LOCAL_PATH)/efsw/src
LOCAL_CFLAGS			:= -Os -DEFSW_USE_CXX11

LOCAL_SRC_FILES			:= $(foreach F, $(LIBEFSW_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

include $(BUILD_STATIC_LIBRARY)
#*************** EFSW ***************

#*************** MAPS ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_MODULES_PATH)

LOCAL_MODULE			:= eepp-maps

LIBMAPS_SRCS			:=  \
	maps/src/maps/*.cpp \
	maps/src/maps/mapeditor/*.cpp

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES) $(EEPP_INC_PATH)
LOCAL_CFLAGS			:= -Os

LOCAL_SRC_FILES			:= $(foreach F, $(LIBMAPS_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

include $(BUILD_STATIC_LIBRARY)
#*************** MAPS ***************

#*************** ETERM ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_MODULES_PATH)

LOCAL_MODULE			:= eterm

LIBETERM_SRCS			:=  \
	eterm/src/eterm/system/*.cpp \
	eterm/src/eterm/terminal/*.cpp \
	eterm/src/eterm/ui/*.cpp

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES) $(EEPP_INC_PATH)
LOCAL_CFLAGS			:= -Os

LOCAL_SRC_FILES			:= $(foreach F, $(LIBETERM_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

include $(BUILD_STATIC_LIBRARY)
#*************** ETERM ***************
