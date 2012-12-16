link_list = { }
os_links = { }

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

function build_link_configuration( package_name )
	links { link_list }
	
	if package_name ~= "eepp" and package_name ~= "eepp-static" then
		links { "eepp-static" }
		add_static_links()
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
		multiple_insert( os_links, { "OpenAL32", "opengl32", "mingw32", "glu32", "gdi32" } )
	elseif os.is("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "OpenAL.framework", "CoreFoundation.framework", "AGL.framework" } )
	elseif os.is("freebsd") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
	elseif os.is("haiku") then
		multiple_insert( os_links, { "openal", "GL" } )
	end
end

function parse_args()
	if args_contains( "GLES2" ) then
		defines { "EE_GLES2", "SOIL_GLES2" }
	end
	
	if args_contains( "GLES1" ) then
		defines { "EE_GLES1", "SOIL_GLES1" }
	end	

	if not args_contains( "STATIC_FT2" ) and os.findlib("freetype") then
		table.insert( link_list, "freetype" )
	end
end

function add_static_links()
	links { "SOIL2-static",
			"chipmunk-static",
			"glew-static",
			"haikuttf-static",
			"libzip-static",
			"stb_vorbis-static",
			"jpeg-compressor-static"
	}
	
	if args_contains( "STATIC_FT2" ) or not os.findlib("freetype") then
		links { "freetype-static", "z" }
	end
end

function select_backend()
	local selected = false
	
	if args_contains( "SDL2" ) then
		table.insert( link_list, "SDL2" )
		files { "src/eepp/window/backend/SDL2/*.cpp" }
		defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }
		selected = true
	elseif args_contains( "SDL" ) then
		table.insert( link_list, "SDL" )
		files { "src/eepp/window/backend/SDL/*.cpp" }
		defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_1_2" }
		selected = true
	end
	
	if args_contains( "allegro5" ) then
		table.insert( link_list, "allegro5" )
		files { "src/eepp/window/backend/allegro5/*.cpp" }
		defines { "EE_BACKEND_ALLEGRO_ACTIVE" }
		selected = true
	end
	
	if args_contains( "SFML" ) then
		table.insert( link_list, "SFML" )
		files { "src/eepp/window/backend/SFML/*.cpp" }
		defines { "EE_BACKEND_SFML_ACTIVE" }
		selected = true
	end
	
	if not selected then
		if os.findlib("SDL2") then
			table.insert( link_list, "SDL2" )
			files { "src/eepp/window/backend/SDL2/*.cpp" }
			defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }
		else
			table.insert( link_list, "SDL" )
			files { "src/eepp/window/backend/SDL/*.cpp" }
			defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_1_2" }
		end
	end

end

function build_eepp( build_name )
	includedirs { "include", "src", "src/eepp/helper/freetype2/include" }

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
	
	multiple_insert( link_list, os_links )

	links { link_list }
	
	configuration "windows"
		files { "src/eepp/window/platform/win/*.cpp" }
		linkoptions { "static-libgcc", "static-libstdc++", "mwindows" }
	
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
	objdir("obj/" .. os.get() .. "/premake4/")

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

	project "libzip-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get() .. "/helpers/")
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/zlib/*.c", "src/eepp/helper/libzip/*.c" }
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
		build_base_configuration( "haikuttf" )

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/helpers/")
		files { "src/eepp/helper/jpeg-compressor/*.cpp" }
		build_base_configuration( "jpeg-compressor" )

	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/")
		build_eepp( "eepp" )

	project "eepp-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get() .. "/")
		build_eepp( "eepp-static" )
	
	project "eepp-test"
		kind "ConsoleApp"
		language "C++"
		files { "src/test/*.cpp" }
		includedirs { "include", "src" }
		build_link_configuration( "eetest" )

	project "eepp-es"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/external_shader/*.cpp" }
		includedirs { "include", "src" }
		build_link_configuration( "eees" )

	project "eepp-ew"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/empty_window/*.cpp" }
		includedirs { "include", "src" }
		build_link_configuration( "eeew" )
