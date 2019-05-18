# premake-androidmk

Application.mk &amp; Android.mk generator for Premake5.

This module requires the latest premake version to work (5.0.0 alpha5).


## Android.mk API

Note : All the fields starting with `ndk` are solution-wide, but filters can be applied.

*****

`ndkabi` : ABI targets

Supported values :
Wanted ABIs separated by spaces (e.g. `"armeabi x86 mips"`)
* `armeabi`
* `armeabi-v7a`
* `arm64-v8a`
* `x86`
* `x86_64`
* `mips`
* `mips64`

Or one of :
* `default`
* `all`

*****

`ndkplatform` : Android API version

Supported values :
* `default`
* `android-3`
* `android-4`
* `android-5`
* `android-8`
* `android-9`
* `android-12`
* `android-13`
* `android-14`
* `android-15`
* `android-16`
* `android-17`
* `android-18`
* `android-19`
* `android-21`

*****

`ndkstl` : Standard library

Supported values :
* `default`
* `libstdc++`
* `gabi++_static`
* `gabi++_shared`
* `stlport_static`
* `stlport_shared`
* `gnustl_static`
* `gnustl_shared`
* `c++_static`
* `c++_shared`

*****

`ndktoolchainversion` : Android toolchain version

Supported values :
* `default`
* `4.8`
* `4.9`
* `clang`
* `clang3.4`
* `clang3.5`

*****

`amk_includes` : Include existing Android.mk files

Supported values : List of files

*****

`amk_importmodules` : Import Android.mk modules

Supported values : List of strings

*****

`amk_staticlinks` and `amk_sharedlinks` : Link libs from included Android.mk files and imported modules

Supported values : List of libraries

If you want to link to system libraries or other projects in the solution, use `links` instead.



## Premake API support

Most functions should work as expected.
Some limitations apply, for example you cannot change the output folder.

`system` and `architecture` are completely ignored.

You cannot do per ABI filtering yet, but it's technically doable.

Some useful working commands :

`rtti "On"` : Enable RTTI

`exceptionhandling "On"` : Enable C++ exceptions

`flags { "C++11" }` : Enable C++11

`optimize "Off"` or `"Debug"` : Enable debug mode (must be used solution-wide)

Symbols are always enabled, use ndk-gdb to debug.


## Sample code

```lua
require "androidmk" -- Adjust path if needed

solution "MySolution"
	configurations { "Debug", "Release" }
	language "C++"

	location "android/jni" -- Generate files in android project jni folder (recommended)

	ndkabi "all"
	ndkplatform "android-12"

	filter "configurations:Release"
		optimize "On"
	filter "configurations:Debug"
		optimize "Debug"


	project "MyProject"
		kind "SharedLib"
		includedirs "SDL2/include"
		files {
			"src/*.cpp",
			"SDL2/src/main/android/SDL_android_main.c",
		}

		links {
			"GLESv1_CM",
			"GLESv2",
			"log",
		}

		amk_includes {
			"SDL2/Android.mk",
		}

		amk_sharedlinks {
			"SDL2",
		}
```


It is recommended to create an Application.mk file including your generated one :

File android/jni/Application.mk :
```
include $(call my-dir)/MySolution_Application.mk
```


Generate command (in android folder) :
`ndk-build PM5_CONFIG=debug -j`

Use your own deploy command, mine is :
`ant debug`

Debug command :
`ndk-gdb-py --gnumake-flag PM5_CONFIG=debug`
