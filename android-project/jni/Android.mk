LOCAL_PATH := $(call my-dir)
MY_PATH := $(LOCAL_PATH)/src

MY_C_INCLUDES := \
	$(MY_PATH)/helper/android/openal/include/ \
	$(MY_PATH)/helper/android/freetype/include \
	$(MY_PATH)/helper/android/SDL2/include \
	$(MY_PATH)/helper/chipmunk
	
MY_C_FLAGS	:=	-DANDROID \
				-DANDROID_NDK \
				-DDISABLE_IMPORTGL \
				-Wall \
				-Wno-unknown-pragmas \
				$(EE_GLES_VERSION) \
				-DEE_NO_SNDFILE \
				-DEE_SDL_VERSION_2

MY_LDLIBS 	:= $(APP_LDLIBS)

include $(call all-subdir-makefiles) 

#*************** EEPP ***************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)

LOCAL_MODULE := eepp

LOCAL_LDLIBS 	:= $(MY_LDLIBS)

LOCAL_CFLAGS	:= $(MY_C_FLAGS)

CODE_SRCS :=  \
	helper/SOIL/*.c \
	helper/stb_vorbis/*.c \
	helper/zlib/*.c \
	helper/libzip/*.c \
	helper/haikuttf/*.cpp \
	utils/*.cpp \
	system/*.cpp \
	base/*.cpp \
	math/*.cpp \
	audio/*.cpp \
	window/*.cpp \
	window/backend/SDL/*.cpp \
	window/backend/SDL2/*.cpp \
	window/backend/allegro5/*.cpp \
	window/backend/null/*.cpp \
	window/platform/null/*.cpp \
	graphics/*.cpp \
	graphics/renderer/*.cpp \
	physics/*.cpp \
	physics/constraints/*.cpp \
	ui/*.cpp \
	ui/tools/*.cpp \
	gaming/*.cpp \
	gaming/mapeditor/*.cpp \
	
LOCAL_C_INCLUDES := $(MY_C_INCLUDES)

LOCAL_SRC_FILES := $(foreach F, $(CODE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES := SDL2 chipmunk freetype openal

include $(BUILD_STATIC_LIBRARY) 
#*************** EEPP ***************

#*************** CHIPMUNK ***************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)

LOCAL_MODULE := chipmunk

LOCAL_CFLAGS := -DANDROID_NDK \
                -DDISABLE_IMPORTGL \
                -std=gnu99 \
                -Wall \
                -Wno-unknown-pragmas

CHIPMUNK_SRCS :=  \
	helper/chipmunk/*.c \
	helper/chipmunk/constraints/*.c \

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)

LOCAL_SRC_FILES := $(foreach F, $(CHIPMUNK_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_LDLIBS := -lm

include $(BUILD_STATIC_LIBRARY)
#*************** CHIPMUNK ***************

#*************** FREETYPE ***************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/helper/android/freetype

LOCAL_MODULE := freetype

APP_SUBDIRS := $(patsubst $(LOCAL_PATH)/%, %, $(shell find $(LOCAL_PATH)/src -type d))

LOCAL_C_INCLUDES := $(foreach D, $(APP_SUBDIRS), $(LOCAL_PATH)/$(D)) $(LOCAL_PATH)/include
LOCAL_CFLAGS := -Os -DFT2_BUILD_LIBRARY

LOCAL_SRC_FILES += $(foreach F, $(APP_SUBDIRS), $(addprefix $(F)/,$(notdir $(wildcard $(LOCAL_PATH)/$(F)/*.c))))

include $(BUILD_STATIC_LIBRARY)
#*************** FREETYPE ***************

#*************** OPENAL *****************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/helper/android/openal

LOCAL_MODULE := openal

APP_SUBDIRS := $(patsubst $(LOCAL_PATH)/%, %, $(shell find $(LOCAL_PATH)/src -type d))

LOCAL_C_INCLUDES := $(foreach D, $(APP_SUBDIRS), $(LOCAL_PATH)/$(D)) $(LOCAL_PATH)/include
LOCAL_CFLAGS := -O3 -DHAVE_CONFIG_H -DAL_ALEXT_PROTOTYPES

LOCAL_SRC_FILES += $(foreach F, $(APP_SUBDIRS), $(addprefix $(F)/,$(notdir $(wildcard $(LOCAL_PATH)/$(F)/*.c))))

LOCAL_LDLIBS := -llog

include $(BUILD_STATIC_LIBRARY)
#*************** OPENAL *****************

#**************** SDL 2 ***************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/helper/android/SDL2

LOCAL_MODULE := SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS := -O3 -D__ANDROID__ -DANDROID -DGL_GLEXT_PROTOTYPES \
	$(EE_GLES_VERSION)

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,, \
	$(wildcard $(LOCAL_PATH)/src/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/audio/dummy/*.c) \
	$(LOCAL_PATH)/src/atomic/SDL_atomic.c \
	$(LOCAL_PATH)/src/atomic/SDL_spinlock.c.arm \
	$(wildcard $(LOCAL_PATH)/src/core/android/*.cpp) \
	$(wildcard $(LOCAL_PATH)/src/cpuinfo/*.c) \
	$(wildcard $(LOCAL_PATH)/src/events/*.c) \
	$(wildcard $(LOCAL_PATH)/src/file/*.c) \
	$(wildcard $(LOCAL_PATH)/src/haptic/*.c) \
	$(wildcard $(LOCAL_PATH)/src/haptic/dummy/*.c) \
	$(wildcard $(LOCAL_PATH)/src/joystick/*.c) \
	$(wildcard $(LOCAL_PATH)/src/joystick/android/*.c) \
	$(wildcard $(LOCAL_PATH)/src/loadso/dlopen/*.c) \
	$(wildcard $(LOCAL_PATH)/src/power/*.c) \
	$(wildcard $(LOCAL_PATH)/src/render/*.c) \
	$(wildcard $(LOCAL_PATH)/src/render/opengles/*.c) \
	$(wildcard $(LOCAL_PATH)/src/render/opengles2/*.c) \
	$(wildcard $(LOCAL_PATH)/src/render/software/*.c) \
	$(wildcard $(LOCAL_PATH)/src/stdlib/*.c) \
	$(wildcard $(LOCAL_PATH)/src/thread/*.c) \
	$(wildcard $(LOCAL_PATH)/src/thread/pthread/*.c) \
	$(wildcard $(LOCAL_PATH)/src/timer/*.c) \
	$(wildcard $(LOCAL_PATH)/src/timer/unix/*.c) \
	$(wildcard $(LOCAL_PATH)/src/video/*.c) \
	$(wildcard $(LOCAL_PATH)/src/video/android/*.c))

LOCAL_LDLIBS := $(EE_GLES_LINK) -ldl -llog

include $(BUILD_STATIC_LIBRARY)
#**************** SDL 1.3 ***************

#**************** eetest ****************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/test

SDL_PATH := $(MY_PATH)/helper/android/SDL2

LOCAL_LDLIBS 	:= $(MY_LDLIBS)

LOCAL_CFLAGS	:= $(MY_C_FLAGS)

LOCAL_MODULE := eetest

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)

LOCAL_SRC_FILES := \
	../helper/android/SDL2/src/main/android/SDL_android_main.cpp \
	eetest.cpp

LOCAL_SHARED_LIBRARIES := eepp

include $(BUILD_SHARED_LIBRARY)
#**************** eetest ****************

#************* empty_window *************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/test/empty_window

LOCAL_MODULE := empty_window

SDL_PATH := $(MY_PATH)/helper/android/SDL2

LOCAL_LDLIBS 	:= $(MY_LDLIBS)

LOCAL_CFLAGS 	:= $(MY_C_FLAGS)

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)

CORE_SRCS :=  \
	../../helper/android/SDL2/src/main/android/*.cpp \
	*.cpp \

LOCAL_SRC_FILES := $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES := eepp

include $(BUILD_SHARED_LIBRARY)
#************ empty_window ************

#************* BnB *************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/bnb

LOCAL_MODULE := main

SDL_PATH := $(MY_PATH)/helper/android/SDL2

LOCAL_LDLIBS 	:= $(MY_LDLIBS)

LOCAL_CFLAGS 	:= $(MY_C_FLAGS)

CORE_SRCS :=  \
	../helper/android/SDL2/src/main/android/*.cpp \
	*.cpp \

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)

LOCAL_SRC_FILES := $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES := eepp

include $(BUILD_SHARED_LIBRARY)
#************ BnB ************

#************* full_test *************
include $(CLEAR_VARS)

LOCAL_PATH := $(MY_PATH)/test/

LOCAL_MODULE := full_test

SDL_PATH := $(MY_PATH)/helper/android/SDL2

LOCAL_LDLIBS 	:= $(MY_LDLIBS)

LOCAL_CFLAGS 	:= $(MY_C_FLAGS)

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)

CORE_SRCS :=  \
	../../helper/android/SDL2/src/main/android/*.cpp \
	*.cpp \

LOCAL_SRC_FILES := $(foreach F, $(CORE_SRCS), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))

LOCAL_STATIC_LIBRARIES := eepp

include $(BUILD_SHARED_LIBRARY)
#************ full_test ************
