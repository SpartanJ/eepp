newoption { trigger = "with-libsndfile", description = "Build with libsndfile support." }
newoption { trigger = "with-static-freetype", description = "Build freetype as a static library." }
newoption { trigger = "with-static-eepp", description = "Force to build the demos and tests with eepp compiled statically" }
newoption { trigger = "with-static-backend", description = "It will try to compile the library with a static backend (only for gcc and mingw).\n\t\t\t\tThe backend should be placed in libs/your_platform/libYourBackend.a" }
newoption { trigger = "with-gles2", description = "Compile with GLES2 support" }
newoption { trigger = "with-gles1", description = "Compile with GLES1 support" }
newoption { 
	trigger = "with-backend", 
	description = "Select the backend to use for window and input handling.\n\t\t\tIf no backend is selected or if the selected is not installed the script will search for a backend present in the system, and will use it.\n\t\t\tIt's possible to build with more than one backend support.\n\t\t\t\tUse comma to separate the backends to build ( you can't mix SDL and SDL2, you'll get random crashes ).\n\t\t\t\tExample: --with-backend=SDL2,SFML2",
	allowed = {
		{ "SDL",    "SDL 1.2" },
		{ "SDL2",  "SDL2 (default and recommended)" },
		{ "allegro5",  "Allegro 5" },
		{ "SFML",  "SFML2 ( SFML 1.6 not supported )" }
	}
}

link_list = { }
os_links = { }
backends = { }
static_backends = { }
backend_selected = false

function args_contains( element )
	return table.contains( _ARGS, element )
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

function os_findlib( name )
	if os.is("macosx") then
		local path = os.findlib( name .. ".framework" )
		
		if path then
			return path
		end
	end
	
	return os.findlib( name )
end

function get_backend_link_name( name )
	if os.is("macosx") then
		local fname = name .. ".framework"
		
		if os.findlib( fname ) then -- Search for the framework
			return fname
		end
	end
	
	return name
end

function build_base_configuration( package_name )
	includedirs { "src/eepp/helper/zlib" }
	
	configuration "debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		buildoptions{ "-Wall", "-std=gnu99" }
		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		flags { "Optimize" }
		buildoptions{ "-Wall", "-std=gnu99" }
		targetname ( package_name )
end

function build_base_cpp_configuration( package_name )
	configuration "debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		buildoptions{ "-Wall" }
		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		flags { "Optimize" }
		buildoptions{ "-Wall" }
		targetname ( package_name )
end

function build_link_configuration( package_name )
	includedirs { "include", "src" }
	
	if package_name ~= "eepp" and package_name ~= "eepp-static" then
		if not _OPTIONS["with-static-eepp"] then
			links { "eepp-shared" }
		else
			links { "eepp-static" }
			add_static_links()
			links { link_list }
		end
	end
	
	configuration "windows"
		if _ACTION == "gmake" then
			links { "mingw32" }
			linkoptions { "-static-libgcc", "-static-libstdc++" }
		end
	
	configuration "debug"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		
		if package_name == "eepp" then
			defines { "EE_DYNAMIC", "EE_EXPORTS" }
		end
		
		flags { "Symbols" }
		buildoptions{ "-Wall -Wno-long-long" }
		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		
		if package_name == "eepp" then
			defines { "EE_DYNAMIC", "EE_EXPORTS" }
		end
		
		flags { "Optimize" }
		buildoptions { "-fno-strict-aliasing -O3 -s -ffast-math" }
		targetname ( package_name )
end

function generate_os_links()
	if os.is("linux") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
	elseif os.is("windows") then
		multiple_insert( os_links, { "OpenAL32", "opengl32", "glu32", "gdi32" } )
	elseif os.is("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "OpenAL.framework", "CoreFoundation.framework", "AGL.framework" } )
	elseif os.is("freebsd") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
	elseif os.is("haiku") then
		multiple_insert( os_links, { "openal", "GL" } )
	end
end

function parse_args()
	if _OPTIONS["with-gles2"] then
		defines { "EE_GLES2", "SOIL_GLES2" }
	end
	
	if _OPTIONS["with-gles1"] then
		defines { "EE_GLES1", "SOIL_GLES1" }
	end	
end

function add_static_links()
	-- The linking order DOES matter
	-- Expose the symbols that need one static library AFTER adding that static lib
	
	-- Add static backends
	if next(static_backends) ~= nil then
		for _, value in pairs( static_backends ) do
			linkoptions { value }
		end
	end
	
	links { "haikuttf-static" }
	
	if _OPTIONS["with-static-freetype"] or not os_findlib("freetype") then
		links { "freetype-static" }
	end
	
	links { "SOIL2-static",
			"chipmunk-static",
			"glew-static",
			"libzip-static",
			"stb_vorbis-static",
			"jpeg-compressor-static",
			"zlib-static"
	}
end

function can_add_static_backend( name )
	if _OPTIONS["with-static-backend"] then
		local path = "libs/linux/lib" .. name .. ".a"
		return os.isfile(path)
	end
end

function insert_static_backend( name )
	table.insert( static_backends, path.getrelative( "libs/" .. os.get(), "./" ) .. "/libs/linux/lib" .. name .. ".a" )
end

function add_sdl2()
	files { "src/eepp/window/backend/SDL2/*.cpp" }
	defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }
	
	if not can_add_static_backend("SDL2") then
		table.insert( link_list, get_backend_link_name( "SDL2" ) )
	else
		insert_static_backend( "SDL2" )
	end
end

function add_sdl()
	--- SDL is LGPL. It can't be build as static library
	table.insert( link_list, get_backend_link_name( "SDL" ) )
	files { "src/eepp/window/backend/SDL/*.cpp" }
	defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_1_2" }
end

function add_allegro5()
	files { "src/eepp/window/backend/allegro5/*.cpp" }
	defines { "EE_BACKEND_ALLEGRO_ACTIVE" }
	
	if not can_add_static_backend("allegro5") then
		table.insert( link_list, get_backend_link_name( "allegro5" ) )
	else
		insert_static_backend( "allegro5" )
	end
end

function add_sfml()
	files { "src/eepp/window/backend/SFML/*.cpp" }
	defines { "EE_BACKEND_SFML_ACTIVE" }
	
	if not can_add_static_backend("SFML") then
		table.insert( link_list, get_backend_link_name( "SFML" ) )
	else
		insert_static_backend( "SFML" )
	end
end

function backend_is( name )
	if not _OPTIONS["with-backend"] then
		_OPTIONS["with-backend"] = "SDL2"
	end
	
	if next(backends) == nil then
		backends = string.explode(_OPTIONS["with-backend"],",")
	end
	
	local backend_sel = table.contains( backends, name )
	
	local ret_val = os_findlib( name ) and backend_sel
	
	if ret_val then
		backend_selected = true
	end
	
	return ret_val
end

function select_backend()	
	if backend_is( "SDL2" ) then
		add_sdl2()
	end
	
	if backend_is( "SDL" ) then
		add_sdl()
	end
	
	if backend_is( "allegro5" ) then
		add_allegro5()
	end
	
	if backend_is( "SFML" ) then
		add_sfml()
	end
	
	-- If the selected backend is not present, try to find one present
	if not backend_selected then
		if os_findlib("SDL") then
			add_sdl()
		elseif os_findlib("SDL2") then
			add_sdl2()
		elseif os_findlib("allegro5") then
			add_allegro5()
		elseif os_findlib("SFML") then
			add_sfml()
		else
			print("ERROR: Couldnt find any backend")
		end
	end
end

function build_eepp( build_name )
	includedirs { "include", "src", "src/eepp/helper/freetype2/include", "src/eepp/helper/zlib" }

	add_static_links()

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
	
	select_backend()
	
	if not _OPTIONS["with-static-freetype"] and os_findlib("freetype") then
		table.insert( link_list, get_backend_link_name( "freetype" ) )
	end
	
	if _OPTIONS["with-libsndfile"] then
		defines { "EE_LIBSNDFILE_ENABLED" }
		
		if os.is("windows") then
			table.insert( link_list, "libsndfile-1" )
		else
			table.insert( link_list, "sndfile" )
		end
	end
	
	multiple_insert( link_list, os_links )

	links { link_list }
	
	configuration "windows"
		files { "src/eepp/window/platform/win/*.cpp" }

		if _ACTION == "gmake" then
			links { "mingw32" }
			linkoptions { "-static-libgcc", "-static-libstdc++" }
		end
	
	configuration "linux"
		files { "src/eepp/window/platform/x11/*.cpp" }
	
	configuration "macosx"
		files { "src/eepp/window/platform/osx/*.cpp" }
	
	build_link_configuration( build_name )
end

solution "eepp"
	location("./make/" .. os.get() .. "/")
	targetdir("./")
	configurations { "debug", "release" }
	objdir("obj/" .. os.get() .. "/")

	generate_os_links()
	parse_args()

	project "SOIL2-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/SOIL2/src/SOIL2/*.c" }
		includedirs { "include/eepp/helper/SOIL2" }
		build_base_configuration( "SOIL2" )

	project "glew-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/glew/*.c" }
		includedirs { "include/eepp/helper/glew" }
		build_base_configuration( "glew" )
		
	project "zlib-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/zlib/*.c", "src/eepp/helper/libzip/*.c" }
		build_base_configuration( "zlib" )

	project "libzip-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/libzip/*.c" }
		includedirs { "src/eepp/helper/zlib" }
		build_base_configuration( "libzip" )

	project "freetype-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/eepp/helper/freetype2/src/**.c" }
		includedirs { "src/eepp/helper/freetype2/include" }
		build_base_configuration( "freetype" )
	
	project "stb_vorbis-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/stb_vorbis/*.c" }
		build_base_configuration( "stb_vorbis" )
		
	project "chipmunk-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/chipmunk/*.c", "src/eepp/helper/chipmunk/constraints/*.c" }
		includedirs { "include/eepp/helper/chipmunk" }
		build_base_configuration( "chipmunk" )

	project "haikuttf-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/haikuttf/*.cpp" }
		includedirs { "src/eepp/helper/freetype2/include" }
		build_base_cpp_configuration( "haikuttf" )

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/jpeg-compressor/*.cpp" }
		build_base_cpp_configuration( "jpeg-compressor" )

	project "eepp-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/")
		build_eepp( "eepp-static" )
	
	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/")
		build_eepp( "eepp" )

	project "eepp-test"
		kind "WindowedApp"
		language "C++"
		files { "src/test/*.cpp" }
		build_link_configuration( "eetest" )

	project "eepp-es"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/external_shader/*.cpp" }
		build_link_configuration( "eees" )

	project "eepp-ew"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/empty_window/*.cpp" }
		build_link_configuration( "eeew" )
