function newplatform(plf)
	local name = plf.name
	local description = plf.description

	-- Register new platform
	premake.platforms[name] = {
		cfgsuffix = "_"..name,
		iscrosscompiler = true
	}

	-- Allow use of new platform in --platfroms
	table.insert(premake.option.list["platform"].allowed, { name, description })
	table.insert(premake.fields.platforms.allowed, name)

	-- Add compiler support
	premake.gcc.platforms[name] = plf.gcc
end

function newgcctoolchain(toolchain)
	newplatform {
		name = toolchain.name,
		description = toolchain.description,
		gcc = {
			cc = toolchain.prefix .. "gcc",
			cxx = toolchain.prefix .. "g++",
			ar = toolchain.prefix .. "ar",
			cppflags = "-MMD " .. toolchain.cppflags
		}
	}
end

newplatform {
	name = "clang",
	description = "Clang",
	gcc = {
		cc = "clang",
		cxx = "clang++",
		ar = "ar",
		cppflags = "-MMD "
	}
}

newgcctoolchain {
	name = "mingw32",
	description = "Mingw32 to cross-compile windows binaries from *nix",
	prefix = "i686-w64-mingw32-",
	cppflags = ""
}

newgcctoolchain {
	name ="android-arm7",
	description = "Android ARMv7 (not implemented)",
	prefix = iif( os.getenv("ANDROID_NDK"), os.getenv("ANDROID_NDK"), "" ) .. "arm-linux-androideabi-",
	cppflags = "-MMD -arch=armv7 -march=armv7 -marm -mcpu=cortex-a8"
}

toolchain_path = os.getenv("TOOLCHAINPATH")

if not toolchain_path then
	toolchain_path = ""
end

-- cross compiling from linux, totally experimental, using: http://code.google.com/p/ios-toolchain-based-on-clang-for-linux/
newplatform {
	name = "ios-cross-arm7",
	description = "iOS ARMv7 (not implemented)",
	gcc = {
		cc = iif( toolchain_path, toolchain_path .. "ios-clang", "clang" ),
		cxx = iif( toolchain_path, toolchain_path .. "ios-clang++", "clang++" ),
		ar = iif( toolchain_path, "ar", "ar" ),
		cppflags = "-MMD"
	}
}

newgcctoolchain {
	name ="ios-arm7",
	description = "iOS ARMv7 (not implemented)",
	prefix = iif( os.getenv("TOOLCHAINPATH"), os.getenv("TOOLCHAINPATH"), "" ),
	cppflags = "-MMD -arch=armv7 -march=armv7 -marm -mcpu=cortex-a8"
}

newgcctoolchain {
	name ="ios-x86",
	description = "iOS x86 (not implemented)",
	prefix = iif( os.getenv("TOOLCHAINPATH"), os.getenv("TOOLCHAINPATH"), "" ),
	cppflags = "-m32 -march=i386"
}

if _OPTIONS.platform then
	-- overwrite the native platform with the options::platform
	premake.gcc.platforms['Native'] = premake.gcc.platforms[_OPTIONS.platform]
end

newoption { trigger = "with-libsndfile", description = "Build with libsndfile support." }
newoption { trigger = "with-static-freetype", description = "Build freetype as a static library." }
newoption { trigger = "with-static-eepp", description = "Force to build the demos and tests with eepp compiled statically" }
newoption { trigger = "with-static-backend", description = "It will try to compile the library with a static backend (only for gcc and mingw).\n\t\t\t\tThe backend should be placed in libs/your_platform/libYourBackend.a" }
newoption { trigger = "with-gles2", description = "Compile with GLES2 support" }
newoption { trigger = "with-gles1", description = "Compile with GLES1 support" }
newoption { 
	trigger = "with-backend", 
	description = "Select the backend to use for window and input handling.\n\t\t\tIf no backend is selected or if the selected is not installed the script will search for a backend present in the system, and will use it.\n\t\t\tIt's possible to build with more than one backend support.\n\t\t\t\tUse comma to separate the backends to build ( you can't mix SDL and SDL2, you'll get random crashes ).\n\t\t\t\tExample: --with-backend=SDL2,SFML",
	allowed = {
		{ "SDL",    "SDL 1.2" },
		{ "SDL2",  "SDL2 (default and recommended)" },
		{ "SFML",  "SFML2 ( SFML 1.6 not supported )" }
	}
}

function os.get_real()
	if 	_OPTIONS.platform == "ios-arm7" or 
		_OPTIONS.platform == "ios-x86" or
		_OPTIONS.platform == "ios-cross-arm7" then
		return "ios"
	end
	
	if _OPTIONS.platform == "android-arm7" then
		return "android"
	end
	
	if 	_OPTIONS.platform == "mingw32" then
		return _OPTIONS.platform
	end

	return os.get()
end

function os.is_real( os_name )
	return os.get_real() == os_name
end

function print_table( table_ref )
	for _, value in pairs( table_ref ) do
		print(value)
	end
end

function args_contains( element )
	return table.contains( _ARGS, element )
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

function string.starts(String,Start)
	if ( _ACTION ) then
		return string.sub(String,1,string.len(Start))==Start
	end
	
	return false
end

function is_vs()
	return ( string.starts(_ACTION,"vs") )
end

link_list = { }
os_links = { }
backends = { }
static_backends = { }
backend_selected = false

function build_base_configuration( package_name )
	includedirs { "src/eepp/helper/zlib" }
	
	configuration "debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		if not is_vs() then
			buildoptions{ "-Wall", "-std=gnu99" }
		end
		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		flags { "Optimize" }
		if not is_vs() then
			buildoptions{ "-Wall", "-std=gnu99" }
		end
		targetname ( package_name )
end

function build_base_cpp_configuration( package_name )
	configuration "debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		if not is_vs() then
			buildoptions{ "-Wall" }
		end
		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		flags { "Optimize" }
		if not is_vs() then
			buildoptions{ "-Wall" }
		end
		targetname ( package_name )
end

function add_cross_config_links()
	if not is_vs() then
		if os.is_real("mingw32") or os.is_real("ios") then -- if is crosscompiling from *nix
			linkoptions { "-static-libgcc", "-static-libstdc++" }
		end
	end
end

function build_link_configuration( package_name, use_ee_icon )
	includedirs { "include", "src" }

	if package_name == "eepp" then
		defines { "EE_EXPORTS" }
	elseif package_name == "eepp-static" then
		defines { "EE_STATIC" }
	end
	
	if package_name ~= "eepp" and package_name ~= "eepp-static" then
		if not _OPTIONS["with-static-eepp"] then
			links { "eepp-shared" }
		else
			links { "eepp-static" }
			defines { "EE_STATIC" }
			add_static_links()
			links { link_list }
		end
		
		if os.is("windows") and not is_vs() then	
			if ( true == use_ee_icon ) then
				linkoptions { "../../assets/icon/ee.res" }
			end
		end
	end	

	configuration "debug"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		flags { "Symbols" }

		if not is_vs() then
			buildoptions{ "-Wall -Wno-long-long" }
		end

		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		flags { "Optimize" }

		if not is_vs() then
			buildoptions { "-fno-strict-aliasing -O3 -s -ffast-math" }
		end

		targetname ( package_name )
		
	configuration "windows"
		add_cross_config_links()
	
	set_ios_config()
end

function generate_os_links()
	if os.is_real("linux") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
		
		if _OPTIONS["with-static-eepp"] then
			table.insert( os_links, "dl" )
		end
	elseif os.is_real("windows") then
		multiple_insert( os_links, { "OpenAL32", "opengl32", "glu32", "gdi32", "ws2_32" } )
	elseif os.is_real("mingw32") then
		multiple_insert( os_links, { "OpenAL32", "opengl32", "glu32", "gdi32", "ws2_32" } )
	elseif os.is_real("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "OpenAL.framework", "CoreFoundation.framework", "AGL.framework" } )
	elseif os.is_real("freebsd") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "openal", "GL", "Xcursor" } )
	elseif os.is_real("haiku") then
		multiple_insert( os_links, { "openal", "GL" } )
	elseif os.is_real("ios") then
		multiple_insert( os_links, { "OpenGLES.framework", "OpenAL.framework", "AudioToolbox.framework", "CoreAudio.framework", "Foundation.framework", "CoreFoundation.framework", "UIKit.framework", "QuartzCore.framework", "CoreGraphics.framework" } )
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
			"libzip-static",
			"stb_vorbis-static",
			"jpeg-compressor-static",
			"zlib-static",
			"imageresampler-static"
	}
	
	if not os.is_real("haiku") and not os.is_real("ios") and not os.is_real("android") then
		links{ "glew-static" }
	end
end

function can_add_static_backend( name )
	if _OPTIONS["with-static-backend"] then
		local path = "libs/" .. os.get_real() .. "/lib" .. name .. ".a"
		return os.isfile(path)
	end
end

function insert_static_backend( name )
	table.insert( static_backends, path.getrelative( "libs/" .. os.get_real(), "./" ) .. "/libs/" .. os.get_real() .. "/lib" .. name .. ".a" )
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

function add_sfml()
	files { "src/eepp/window/backend/SFML/*.cpp" }
	defines { "EE_BACKEND_SFML_ACTIVE" }
	
	if not can_add_static_backend("SFML") then
		table.insert( link_list, get_backend_link_name( "sfml-system" ) )
		table.insert( link_list, get_backend_link_name( "sfml-window" ) )
	else
		insert_static_backend( "libsfml-system" )
		insert_static_backend( "libsfml-window" )
	end
end

function set_ios_config()
	if 	_OPTIONS.platform == "ios-arm7" or 
		_OPTIONS.platform == "ios-x86" or 
		_OPTIONS.platform == "ios-cross-arm7" then
	
		local err = false
		
		if nil == os.getenv("TOOLCHAINPATH") then
			print("You must set TOOLCHAINPATH enviroment variable.")
			print("\tExample: /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/")
			err = true
		end
		
		if nil == os.getenv("SYSROOTPATH") then
			print("You must set SYSROOTPATH enviroment variable.")
			print("\tExample: /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk")
			err = true
		end

		if nil == os.getenv("IOSVERSION") then
			print("You must set IOSVERSION enviroment variable.")
			print("\tExample: 5.0")
			err = true
		end
		
		if err then
			os.exit(1)
		end

		local sysroot_path = os.getenv("SYSROOTPATH")
		local framework_path = sysroot_path .. "/System/Library/Frameworks"
		local framework_libs_path = framework_path .. "/usr/lib"
		local sysroot_ver = "-miphoneos-version-min=" .. os.getenv("IOSVERSION") .. " -isysroot " .. sysroot_path
		
		configuration { "ios-arm", "ios-x86" }
			buildoptions { sysroot_ver .. "-I" .. sysroot_path .. "/usr/include" }
			linkoptions { sysroot_ver }
			libdirs { framework_libs_path }
			linkoptions { "-F" .. framework_path .. " -L" .. framework_libs_path .. " -isysroot " .. sysroot_path }
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

	if os.is_real("mingw32") then
		ret_val = backend_sel
	end

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

	if backend_is( "SFML" ) then
		add_sfml()
	end
	
	-- If the selected backend is not present, try to find one present
	if not backend_selected then
		if os_findlib("SDL") then
			add_sdl()
		elseif os_findlib("SDL2") then
			add_sdl2()
		elseif os_findlib("SFML") then
			add_sfml()
		else
			print("ERROR: Couldnt find any backend. Forced SDL.")
			add_sdl()
		end
	end
end

function build_eepp( build_name )
	includedirs { "include", "src", "src/eepp/helper/freetype2/include", "src/eepp/helper/zlib" }

	add_static_links()

	if os.is("windows") then
		files { "src/eepp/system/platform/win/*.cpp" }
		files { "src/eepp/network/platform/win/*.cpp" }
	else
		files { "src/eepp/system/platform/posix/*.cpp" }
		files { "src/eepp/network/platform/unix/*.cpp" }
	end

	files { "src/eepp/base/*.cpp",
			"src/eepp/math/*.cpp",
			"src/eepp/system/*.cpp",
			"src/eepp/audio/*.cpp",
			"src/eepp/graphics/*.cpp",
			"src/eepp/graphics/renderer/*.cpp",
			"src/eepp/window/*.cpp",
			"src/eepp/window/platform/null/*.cpp",
			"src/eepp/network/*.cpp",
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
	
	build_link_configuration( build_name )
	
	set_ios_config()
	
	configuration "windows"
		files { "src/eepp/window/platform/win/*.cpp" }
		add_cross_config_links()
	
	configuration "linux"
		files { "src/eepp/window/platform/x11/*.cpp" }
	
	configuration "macosx"
		files { "src/eepp/window/platform/osx/*.cpp" }
end

solution "eepp"
	location("./make/" .. os.get_real() .. "/")
	targetdir("./")
	configurations { "debug", "release" }
	objdir("obj/" .. os.get_real() .. "/")

	generate_os_links()
	parse_args()

	project "SOIL2-static"
		kind "StaticLib"

		if is_vs() then
			language "C++"
			buildoptions { "/TP" }
		else
			language "C"
		end

		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/SOIL2/src/SOIL2/*.c" }
		includedirs { "include/eepp/helper/SOIL2" }
		build_base_configuration( "SOIL2" )

	project "glew-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		if not os.is_real("haiku") and not os.is_real("ios") and not os.is_real("android") then
			files { "src/eepp/helper/glew/*.c" }
		end
		includedirs { "include/eepp/helper/glew" }
		build_base_configuration( "glew" )
		
	project "zlib-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/zlib/*.c", "src/eepp/helper/libzip/*.c" }
		build_base_configuration( "zlib" )

	project "libzip-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/libzip/*.c" }
		includedirs { "src/eepp/helper/zlib" }

		if is_vs() then
			includedirs { "src/eepp/helper/libzip/vs" }
		end

		build_base_configuration( "libzip" )

	project "freetype-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/eepp/helper/freetype2/src/**.c" }
		includedirs { "src/eepp/helper/freetype2/include" }
		build_base_configuration( "freetype" )
	
	project "stb_vorbis-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/stb_vorbis/*.c" }
		build_base_configuration( "stb_vorbis" )
		
	project "chipmunk-static"
		kind "StaticLib"

		if is_vs() then
			language "C++"
			buildoptions { "/TP" }
		else
			language "C"
		end

		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/chipmunk/*.c", "src/eepp/helper/chipmunk/constraints/*.c" }
		includedirs { "include/eepp/helper/chipmunk" }
		build_base_configuration( "chipmunk" )

	project "haikuttf-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/haikuttf/*.cpp" }
		includedirs { "src/eepp/helper/freetype2/include" }
		build_base_cpp_configuration( "haikuttf" )

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/jpeg-compressor/*.cpp" }
		build_base_cpp_configuration( "jpeg-compressor" )

	project "imageresampler-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get_real() .. "/helpers/")
		files { "src/eepp/helper/imageresampler/*.cpp" }
		build_base_cpp_configuration( "imageresampler" )

	project "eepp-main"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get_real() .. "/")
		files { "src/eepp/main/eepp_main.cpp" }

	project "eepp-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.get_real() .. "/")
		build_eepp( "eepp-static" )
	
	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		targetdir("libs/" .. os.get_real() .. "/")
		build_eepp( "eepp" )

	-- Examples
	project "eepp-test"
		kind "WindowedApp"
		language "C++"
		files { "src/test/*.cpp" }
		build_link_configuration( "eetest", true )

	project "eepp-es"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/external_shader/*.cpp" }
		build_link_configuration( "eees", true )

	project "eepp-ew"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/empty_window/*.cpp" }
		build_link_configuration( "eeew", true )

	project "eepp-sound"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/sound/*.cpp" }
		build_link_configuration( "eesound", true )

	project "eepp-sprites"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/sprites/*.cpp" }
		build_link_configuration( "eesprites", true )

	project "eepp-fonts"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/fonts/*.cpp" }
		build_link_configuration( "eefonts", true )

	project "eepp-vbo-fbo-batch"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/vbo_fbo_batch/*.cpp" }
		build_link_configuration( "eevbo-fbo-batch", true )

	project "eepp-physics"
		kind "WindowedApp"
		language "C++"
		files { "src/examples/physics/*.cpp" }
		build_link_configuration( "eephysics", true )

	project "eepp-http-request"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/http_request/*.cpp" }
		build_link_configuration( "eehttp-request", true )

if os.isfile("external_projects.lua") then
	dofile("external_projects.lua")
end
