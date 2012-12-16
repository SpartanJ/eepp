function args_contains( element )
	for _, value in pairs(_ARGS) do
		if value == element then
			return true
	   	end
	end
	return false
end

function print_table( table_ref )
	for _, value in pairs( table_ref ) do
		print(value)
	end
end

function multiple_insert( parent_table, insert_table )
	for _, value in pairs( insert_table ) do
		table.insert( parent_table, value )
	end
end

solution "eepp"
	location("./make/" .. os.get() .. "/")
	targetdir("./")
	configurations { "debug", "release" }
	objdir("obj/" .. os.get() .. "/premake4/")

	link_list = { }
	os_links = { }
	
	if os.is("linux") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
	elseif os.is("windows") then
		multiple_insert( os_links, { "OpenAL32", "opengl32", "mingw32", "glu32", "gdi32" } )
	elseif os.is("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "OpenAL.framework", "CoreFoundation.framework", "AGL.framework" } )
	elseif os.is("freebsd") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
	elseif os.is("haiku") then
		multiple_insert( os_links, { "openal", "GL" } )
	end
	
	if args_contains( "GLES2" ) then
		defines { "EE_GLES2", "SOIL_GLES2" }
	end
	
	if args_contains( "GLES1" ) then
		defines { "EE_GLES1", "SOIL_GLES1" }
	end	

	if not args_contains( "STATIC_FT2" ) then
		table.insert( link_list, "freetype" )
	end
	
	project "SOIL2-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/SOIL2/src/SOIL2/*.c" }
		includedirs { "include/eepp/helper/SOIL2" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "SOIL2-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "SOIL2"
	
	project "glew-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/glew/*.c" }
		includedirs { "include/eepp/helper/glew" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "glew-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "glew"
	
	project "zlib-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/zlib/*.c" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "zlib-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "zlib"
	
	project "libzip-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/libzip/*.c" }
		includedirs { "src/eepp/helper/zlib" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "libzip-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "libzip"
	
	project "stb_vorbis-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/stb_vorbis/*.c" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "stb_vorbis-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "stb_vorbis"
	
	project "chipmunk-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/chipmunk/*.c", "src/eepp/helper/chipmunk/constraints/*.c" }
		includedirs { "include/eepp/helper/chipmunk" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall", "-std=gnu99" }
			targetname "chipmunk-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall", "-std=gnu99" }
			targetname "chipmunk"
	
	project "haikuttf-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/haikuttf/*.cpp" }
		includedirs { "src/eepp/helper/freetype2/include" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "haikuttf-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "haikuttf"

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/jpeg-compressor/*.cpp" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "jpeg-compressor-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "jpeg-compressor"

	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/")
		includedirs { "include", "src", "src/eepp/helper/freetype2/include" }
		links { "SOIL2-static",
				"chipmunk-static",
				"glew-static",
				"haikuttf-static",
				"zlib-static",
				"libzip-static",
				"stb_vorbis-static",
				"jpeg-compressor-static"
		}
		
		if os.is("windows") then
			files { "src/eepp/system/platform/win/*.cpp" }
		else
			files { "src/eepp/system/platform/posix/*.cpp" }
		end

		files { "src/eepp/base/*.cpp",
				"src/eepp/math/*.cpp",
				"src/eepp/system/*.cpp",
				"src/eepp/audio/*.cpp",
				"src/eepp/graphics/*.cpp",
				"src/eepp/graphics/renderer/*.cpp",
				"src/eepp/window/*.cpp",
				"src/eepp/window/platform/null/*.cpp",
				"src/eepp/ui/*.cpp",
				"src/eepp/ui/tools/*.cpp",
				"src/eepp/physics/*.cpp",
				"src/eepp/physics/constraints/*.cpp",
				"src/eepp/gaming/*.cpp",
				"src/eepp/gaming/mapeditor/*.cpp"
		}
		
		if args_contains( "SDL2" ) then
			table.insert( link_list, "SDL2" )
			files { "src/eepp/window/backend/SDL2/*.cpp" }
			defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }
		elseif args_contains( "SDL" ) then
			table.insert( link_list, "SDL" )
			files { "src/eepp/window/backend/SDL/*.cpp" }
			defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_1_2" }
		end
		
		if args_contains( "allegro5" ) then
			table.insert( link_list, "allegro5" )
			files { "src/eepp/window/backend/allegro5/*.cpp" }
			defines { "EE_BACKEND_ALLEGRO_ACTIVE" }
		end
		
		if args_contains( "SFML" ) then
			table.insert( link_list, "SFML" )
			files { "src/eepp/window/backend/SFML/*.cpp" }
			defines { "EE_BACKEND_SFML_ACTIVE" }
		end
		
		if next(link_list) == nil then
			table.insert( link_list, "SDL" )
			files { "src/eepp/window/backend/SDL/*.cpp" }
			defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_1_2" }
		end

		multiple_insert( link_list, os_links )

		links { link_list }
		
		configuration "windows"
			files { "src/eepp/window/platform/win/*.cpp" }
			linkoptions { "static-libgcc", "static-libstdc++", "mwindows" }
		
		configuration "linux"
			files { "src/eepp/window/platform/x11/*.cpp" }
		
		configuration "macosx"
			files { "src/eepp/window/platform/osx/*.cpp" }

		configuration "debug"
			defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER", "EE_DYNAMIC", "EE_EXPORTS" }
			flags { "Symbols" }
			buildoptions{ "-Wall -Wno-long-long" }
			targetname "eepp-debug"

		configuration "release"
			defines { "NDEBUG", "EE_DYNAMIC", "EE_EXPORTS" }
			flags { "Optimize" }
			buildoptions { "-fno-strict-aliasing -O3 -s -ffast-math" }
			targetname "eepp"
		
	project "eepp-test"
		kind "ConsoleApp"
		language "C++"
		links { link_list, "eepp-shared" }
		
		files { "src/test/*.cpp" }
		includedirs { "include", "src" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall -Wno-long-long" }
			targetname "eetest-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions { "-fno-strict-aliasing -O3 -s -ffast-math" }
			targetname "eetest-release"

	project "eepp-es"
		kind "ConsoleApp"
		language "C++"
		links { link_list, "eepp-shared" }
		
		files { "src/examples/external_shader/*.cpp" }
		includedirs { "include", "src" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall -Wno-long-long" }
			targetname "eees-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions { "-fno-strict-aliasing -O3 -s -ffast-math" }
			targetname "eees-release"

	project "eepp-ew"
		kind "ConsoleApp"
		language "C++"
		links { link_list, "eepp-shared" }
		
		files { "src/examples/empty_window/*.cpp" }
		includedirs { "include", "src" }
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall -Wno-long-long" }
			targetname "eeew-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions { "-fno-strict-aliasing -O3 -s -ffast-math" }
			targetname "eeew-release"
