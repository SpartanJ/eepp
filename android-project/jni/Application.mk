APP_PROJECT_PATH := $(call my-dir)/..

#EE_GLES_VERSION		:= -DEE_GLES2 -DSOIL_GLES2 -DSDL_GLES2
#EE_GLES_LINK		:= -lGLESv2

EE_SDL_VERSION		:= EE_SDL_VERSION_1_3
EE_GLES_VERSION		:= -DEE_GLES1 -DSOIL_GLES1 -DSDL_GLES1
EE_GLES_LINK		:= -lGLESv1_CM

APP_STL				:= stlport_static

APP_LDLIBS			:= -llog $(EE_GLES_LINK) -lm -lz

APP_CFLAGS			:= -DDEBUG										# arm-linux-androideabi-4.4.3 crashes in -O0 mode on SDL sources
APP_PLATFORM		:= android-7
APP_MODULES			:= main
