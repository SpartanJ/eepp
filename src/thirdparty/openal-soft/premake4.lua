solution "openal-soft"
	location("./make/" .. os.get() .. "/")
	targetdir("./")
	configurations { "debug", "release" }
	objdir("obj/" .. os.get() .. "/premake4/")

	project "openal-soft-shared"
		kind "SharedLib"
		language "C"
		targetdir("libs/" .. os.get())
		files { "Alc/*.c", "OpenAL32/*.c" }
		
		defines { "AL_ALEXT_PROTOTYPES" }
		
		if os.is("linux") then
			files { "Alc/backends/alsa.c", "Alc/backends/pulseaudio.c"  }
			defines { "HAVE_ALSA", "HAVE_XMMINTRIN_H", "HAVE_PULSEAUDIO" }
			excludes { "Alc/mixer_neon.c" }
			buildoptions { "-msse" }
		elseif os.is("windows") then
			files { "Alc/backends/dsound.c", "Alc/backends/winmm.c" }
			defines { "HAVE_DSOUND", "HAVE_WINMM" }
			excludes { "Alc/mixer_neon.c" }
			buildoptions { "-msse" }
		elseif os.is("macosx") then
			files { "Alc/backends/coreaudio.c" }
			defines { "HAVE_COREAUDIO" }
			excludes { "Alc/mixer_neon.c" }
			buildoptions { "-msse" }
		end
		
		includedirs { "include", "OpenAL32/Include", "./" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "openal-soft-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "openal-soft"
	