LOCAL_PATH				:= $(call my-dir)
EEPP_BASE_PATH			:= $(LOCAL_PATH)/../../../../src
EEPP_PATH				:= $(LOCAL_PATH)/../../../../src/eepp
EEPP_INC_PATH			:= $(LOCAL_PATH)/../../../../include
EEPP_THIRD_PARTY_PATH	:= $(EEPP_BASE_PATH)/thirdparty
SDL_PATH				:= $(EEPP_THIRD_PARTY_PATH)/SDL2
SDL_MAIN_PATH			:= $(SDL_PATH)/src/main/android/*.c

EEPP_C_INCLUDES			:= \
	$(EEPP_THIRD_PARTY_PATH) \
	$(EEPP_THIRD_PARTY_PATH)/openal-soft/include/ \
	$(EEPP_THIRD_PARTY_PATH)/freetype2/include \
	$(SDL_PATH)/include \
	$(EEPP_THIRD_PARTY_PATH)/chipmunk \
	$(EEPP_INC_PATH)/eepp/thirdparty \
	$(EEPP_INC_PATH)/eepp/thirdparty/chipmunk \
	$(EEPP_THIRD_PARTY_PATH)/SOIL2/src/SOIL2 \
	$(EEPP_THIRD_PARTY_PATH)/stb_vorbis \
	$(EEPP_THIRD_PARTY_PATH)/libvorbis/lib \
	$(EEPP_THIRD_PARTY_PATH)/libvorbis/include \
	$(EEPP_THIRD_PARTY_PATH)/libogg/include \
	$(EEPP_THIRD_PARTY_PATH)/mbedtls/include

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
	system/*.cpp \
	system/platform/posix/*.cpp \
	network/*.cpp \
	network/ssl/*.cpp \
	network/ssl/backend/mbedtls/*.cpp \
	network/platform/unix/*.cpp \
	core/*.cpp \
	math/*.cpp \
	audio/*.cpp \
	window/*.cpp \
	window/backend/SDL2/*.cpp \
	window/platform/null/*.cpp \
	graphics/*.cpp \
	graphics/renderer/*.cpp \
	physics/*.cpp \
	physics/constraints/*.cpp \
	scene/*.cpp \
	scene/actions/*.cpp \
	ui/*.cpp \
	ui/actions/*.cpp \
	ui/tools/*.cpp \
	maps/*.cpp \
	maps/mapeditor/*.cpp

LOCAL_C_INCLUDES		:= $(EEPP_C_INCLUDES)

LOCAL_SRC_FILES			:= $(foreach F, $(CODE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES	:= openal SDL2 chipmunk freetype

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

LOCAL_C_INCLUDES		:= $(foreach D, $(APP_SUBDIRS), $(LOCAL_PATH)/$(D)) $(LOCAL_PATH)/include
LOCAL_CFLAGS			:= -Os -DFT2_BUILD_LIBRARY

LOCAL_SRC_FILES			+= $(foreach F, $(APP_SUBDIRS), $(addprefix $(F)/,$(notdir $(wildcard $(LOCAL_PATH)/$(F)/*.c))))

include $(BUILD_STATIC_LIBRARY)
#*************** FREETYPE ***************

#*************** OPENAL *****************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(EEPP_THIRD_PARTY_PATH)/openal-soft

LOCAL_MODULE			:= openal

LOCAL_CFLAGS			:= -O3 -DHAVE_CONFIG_H -DAL_ALEXT_PROTOTYPES -DHAVE_OPENSL

LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/ $(LOCAL_PATH)/include $(LOCAL_PATH)/OpenAL32/Include $(LOCAL_PATH)/Alc $(LOCAL_PATH)/common

LOCAL_SRC_FILES			:= \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/OpenAL32/*.c) \
	$(wildcard $(LOCAL_PATH)/common/*.c) \
	$(wildcard $(LOCAL_PATH)/Alc/effects/*.c) \
	$(LOCAL_PATH)/Alc/ALc.c \
	$(LOCAL_PATH)/Alc/alconfig.c \
	$(LOCAL_PATH)/Alc/ALu.c \
	$(LOCAL_PATH)/Alc/ambdec.c \
	$(LOCAL_PATH)/Alc/bformatdec.c \
	$(LOCAL_PATH)/Alc/bs2b.c \
	$(LOCAL_PATH)/Alc/converter.c \
	$(LOCAL_PATH)/Alc/helpers.c \
	$(LOCAL_PATH)/Alc/hrtf.c \
	$(LOCAL_PATH)/Alc/mastering.c \
	$(LOCAL_PATH)/Alc/mixer_c.c \
	$(LOCAL_PATH)/Alc/mixer.c \
	$(LOCAL_PATH)/Alc/nfcfilter.c \
	$(LOCAL_PATH)/Alc/panning.c \
	$(LOCAL_PATH)/Alc/ringbuffer.c \
	$(LOCAL_PATH)/Alc/uhjfilter.c \
	$(LOCAL_PATH)/Alc/backends/base.c \
	$(LOCAL_PATH)/Alc/backends/opensl.c \
	$(LOCAL_PATH)/Alc/backends/loopback.c \
	$(LOCAL_PATH)/Alc/backends/wave.c \
	$(LOCAL_PATH)/Alc/backends/null.c \
	$(wildcard $(LOCAL_PATH)/src/video/android/*.c))

LOCAL_LDLIBS			:= -llog -lOpenSLES

include $(BUILD_SHARED_LIBRARY)
#*************** OPENAL *****************

#**************** SDL 2 ***************
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SDL_PATH)

LOCAL_MODULE			:= SDL2

LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/include

LOCAL_CFLAGS			:= -D__ANDROID__ -DANDROID -DGL_GLEXT_PROTOTYPES $(EE_GLES_VERSION)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/src/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/dummy/*.c) \
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
	$(LOCAL_PATH)/src/joystick/steam/SDL_steamcontroller.c \
	$(wildcard $(LOCAL_PATH)/src/loadso/dlopen/*.c) \
	$(wildcard $(LOCAL_PATH)/src/power/*.c) \
	$(wildcard $(LOCAL_PATH)/src/power/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/filesystem/android/*.c) \
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

include $(BUILD_STATIC_LIBRARY)
#**************** SDL 2 ***************
 
