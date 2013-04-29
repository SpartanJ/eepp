APP_PROJECT_PATH := $(call my-dir)/..

#EE_GLES_VERSION		:= -DEE_GLES2 -DSOIL_GLES2 -DSDL_GLES2
#EE_GLES_LINK		:= -lGLESv2

EE_SDL_VERSION		:= EE_SDL_VERSION_2

EE_GLES_VERSION		:= -DEE_GLES1 -DSOIL_GLES1 -DSDL_GLES1
EE_GLES_LINK		:= -lGLESv1_CM

APP_STL				:= stlport_static

APP_LDLIBS			:= -llog $(EE_GLES_LINK) -lm -lz -lOpenSLES

#Debug Build
# arm-linux-androideabi-4.4.3 crashes in -O0 mode on SDL sources
APP_CFLAGS			:= -g -DDEBUG -DEE_DEBUG -DEE_MEMORY_MANAGER
APP_OPTIM :=debug

#Release Build
#APP_CFLAGS			:= -fno-strict-aliasing -O3 -s -DNDEBUG -ffast-math

APP_PLATFORM		:= android-9
APP_MODULES			:= main
APP_ABI				:= armeabi-v7a
#APP_ABI				:= x86
