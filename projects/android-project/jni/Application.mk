APP_PROJECT_PATH := $(call my-dir)/..

EE_SDL_VERSION		:= EE_SDL_VERSION_2

EE_GLES_VERSION		:= -DEE_GLES2 -DSOIL_GLES2 -DSDL_GLES2 -DEE_GLES1 -DSOIL_GLES1 -DSDL_GLES1
EE_GLES_LINK		:= -lGLESv2 -lGLESv1_CM

APP_STL				:= stlport_static

APP_LDLIBS			:= -llog $(EE_GLES_LINK) -lm -lz -lOpenSLES -lEGL -landroid

#Debug Build
#APP_CFLAGS			:= -g -DDEBUG -DEE_DEBUG
#APP_OPTIM 			:=debug

#Release Build
APP_CFLAGS			:= -fno-strict-aliasing -O3 -s -DNDEBUG -ffast-math

APP_PLATFORM		:= android-10
APP_MODULES			:= main
APP_ABI				:= armeabi-v7a x86

APP_CPPFLAGS		+= -Wno-error=format-security
APP_CFLAGS			+= -Wno-error=format-security
