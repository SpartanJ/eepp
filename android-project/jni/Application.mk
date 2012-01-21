APP_PROJECT_PATH := $(call my-dir)/..

EE_GLES_VERSION := -DEE_GLES1 -DSOIL_GLES1
EE_GLES_LINK := -lGLESv1_CM

# Replace this with our path
# The namespace in Java file, with dots replaced with underscores
SDL_JAVA_PACKAGE_PATH := eepp

# Path to files with application data - they should be downloaded from Internet on first app run inside
# Java sources, or unpacked from resources (TODO)
# Typically /sdcard/alienblaster 
# Or /data/data/de.schwardtnet.alienblaster/files if you're planning to unpack data in application private folder
# Your application will just set current directory there
SDL_CURDIR_PATH := eepp

# Android Dev Phone G1 has trackball instead of cursor keys, and 
# sends trackball movement events as rapid KeyDown/KeyUp events,
# this will make Up/Down/Left/Right key up events with X frames delay,
# so if application expects you to press and hold button it will process the event correctly.
# TODO: create a libsdl config file for that option and for key mapping/on-screen keyboard
SDL_TRACKBALL_KEYUP_DELAY := 1

# If the application designed for higher screen resolution enable this to get the screen
# resized in HW-accelerated way, however it eats a tiny bit of CPU
SDL_VIDEO_RENDER_RESIZE := 1
SDL_VIDEO_RENDER_RESIZE_KEEP_ASPECT := 0
SDL_ADDITIONAL_CFLAGS := -DSDL_ANDROID_KEYCODE_MOUSE=UNKNOWN -DSDL_ANDROID_KEYCODE_0=LCTRL -DSDL_ANDROID_KEYCODE_1=LALT -DSDL_ANDROID_KEYCODE_2=SPACE -DSDL_ANDROID_KEYCODE_3=RETURN -DSDL_ANDROID_KEYCODE_4=RETURN
SDL_VERSION := 1.3

APP_STL := gnustl_static

# arm-linux-androideabi-4.4.3 crashes in -O0 mode on SDL sources
#APP_CFLAGS := -O2 -DNDEBUG -g -fexceptions
APP_CFLAGS := -DDEBUG
APP_PLATFORM := android-10

APP_MODULES := bnb
