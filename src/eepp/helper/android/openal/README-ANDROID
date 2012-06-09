OpenAL Soft for Android
=======================

OpenAL Soft on Android is supported only starting from 1.5 version.

To build OpenAL Soft for Android you must have install SDK and NDK (at least r4) installed.

Go to Android directory and execute

$ PATH/TO/NDK/ndk-build

This will build libopenal.so and libexample.so under libs subdirectory. 
You can use libopenal.so in your own projects. If you want to build Java example, then
first update project to your local SDK installation:

$ PATH/TO/SDK/tools/android update project --path . --target android-3

Run this only once (it will create local.properties file, and update default.properties file).
After that use ant to compile and package Java code:

$ ant debug

Now you will find OpenAL-debug.apk under bin directory. Don't worry that it is debug. All the
decoding will be done from native C code from jni/example.c file.
Install it to your device:

$ adb install -r bin/OpenAL-debug.apk

And run it either manually, or with am from command-line:

$ adb shell am start -a android.intent.action.MAIN -n net.strangesoft.kcat/.OpenAL

Example will decode ogg file using Tremolo library and will play audio with streaming source.
Tremolo library is heavily optimized Tremor library (integer only Vorbis decoder).
It is BSD licensed: http://wss.co.uk/pinknoise/tremolo/

You can open DDMS to watch debug logging or any error messages if there is any. You can filter
out OpenAL error/info messages with tag "OpenAL".

Example is using trash80 song Three/Four Robot Slojam from http://trash80.net/music
It is distributed under Creative Commons license.

Take into consideration that Android mobile devices is not as powerful as your desktop, so do
not put on OpenAL too many work. Use low sample rate (22050 or better 11025, if not lower) data.
Also create context by specifying lower sample rrate. Take into consideration that not all
Android devices have hardware floting point calculations available. Many of them will execute
floating point calculations in software which is slower.

--
Martins Mozeiko
martins.mozeiko@gmail.com
