require "premake/modules/androidmk"

newoption { trigger = "with-openssl", description = "Enables OpenSSL support ( and disables mbedtls backend )." }
newoption { trigger = "with-dynamic-freetype", description = "Dynamic link against freetype." }
newoption { trigger = "with-static-eepp", description = "Force to build the demos and tests with eepp compiled statically" }
newoption { trigger = "with-static-backend", description = "It will try to compile the library with a static backend (only for gcc and mingw).\n\t\t\t\tThe backend should be placed in libs/your_platform/libYourBackend.a" }
newoption { trigger = "with-gles2", description = "Compile with GLES2 support" }
newoption { trigger = "with-gles1", description = "Compile with GLES1 support" }
newoption { trigger = "with-mojoal", description = "Compile with mojoAL as OpenAL implementation instead of using openal-soft (requires SDL2 backend)" }
newoption { trigger = "use-frameworks", description = "In macOS it will try to link the external libraries from its frameworks. For example, instead of linking against SDL2 it will link against SDL2.framework." }
newoption { trigger = "windows-vc-build", description = "This is used to build the framework in Visual Studio downloading its external dependencies and making them available to the VS project without having to install them manually." }
newoption {
	trigger = "with-backend",
	description = "Select the backend to use for window and input handling.\n\t\t\tIf no backend is selected or if the selected is not installed the script will search for a backend present in the system, and will use it.",
	allowed = {
		{ "SDL2",  "SDL2" },
	}
}

function print_table( table_ref )
	for _, value in pairs( table_ref ) do
		print(value)
	end
end

function table_length(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

function multiple_insert( parent_table, insert_table )
	for _, value in pairs( insert_table ) do
		table.insert( parent_table, value )
	end
end

function get_ios_arch()
	local archs = string.explode( os.target(), "-" )
	return archs[ table_length( archs ) ]
end

function os_findlib( name )
	if os.istarget("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		local path = "/Library/Frameworks/" .. name .. ".framework"

		if os.isdir( path ) then
			return path
		end
	end

	return os.findlib( name )
end

function get_backend_link_name( name )
	if os.istarget("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		local fname = name .. ".framework"

		if os_findlib( name ) then -- Search for the framework
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

function is_xcode()
	return ( string.starts(_ACTION,"xcode") )
end

function set_kind()
	if os.istarget("macosx") then
		kind("ConsoleApp")
	else
		kind("WindowedApp")
	end
end

link_list = { }
os_links = { }
backends = { }
static_backends = { }
backend_selected = false
remote_sdl2_version = "SDL2-2.0.10"
remote_sdl2_devel_vc_url = "https://www.libsdl.org/release/SDL2-devel-2.0.10-VC.zip"

function incdirs( dirs )
	if is_xcode() then
		sysincludedirs { dirs }
	end
	includedirs { dirs }
end

function download_and_extract_dependencies()
	if _OPTIONS["windows-vc-build"] then
		print("Downloading: " .. remote_sdl2_devel_vc_url)
		local dest_dir = "src/thirdparty/"
		local local_file = dest_dir .. remote_sdl2_version .. ".zip"
		local result_str, response_code = http.download(remote_sdl2_devel_vc_url, local_file)
		if response_code == 200 then
			print("Downloaded successfully to: " .. local_file)
			zip.extract(local_file, dest_dir)
			print("Extracted " .. local_file .. " into " .. dest_dir)
		else
			print("Failed to download:  " .. remote_sdl2_rul)
			exit(1)
		end
	end
end

function build_base_configuration( package_name )
	incdirs { "src/thirdparty/zlib" }

	set_ios_config()
	set_xcode_config()

	filter "not system:windows"
		buildoptions{ "-fPIC" }

	filter "configurations:debug"
		targetname ( package_name .. "-debug" )

	filter "configurations:release"
		targetname ( package_name )

	filter "action:not vs*"
		cdialect "gnu99"
		buildoptions{ "-Wall" }

	filter "action:vs*"
		incdirs { "src/thirdparty/libzip/vs" }
end

function build_base_cpp_configuration( package_name )
	if not os.istarget("windows") then
		buildoptions{ "-fPIC" }
	end

	set_ios_config()
	set_xcode_config()

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"
		if not is_vs() then
			buildoptions{ "-Wall" }
		end
		targetname ( package_name .. "-debug" )

	filter "configurations:release"
		optimize "Speed"
		if not is_vs() then
			buildoptions{ "-Wall" }
		end
		targetname ( package_name )
end

function add_cross_config_links()
	if not is_vs() then
		if os.istarget("mingw32") or os.istarget("windows") or os.istarget("ios") then -- if is crosscompiling from *nix
			linkoptions { "-static-libgcc", "-static-libstdc++" }
		end

		if os.istarget("mingw32") then
			linkoptions { "-Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic" }
		end
	end
end

function build_link_configuration( package_name, use_ee_icon )
	incdirs { "include" }

	local extension = "";

	if package_name == "eepp" then
		defines { "EE_EXPORTS" }
	elseif package_name == "eepp-static" then
		defines { "EE_STATIC" }
	end

	if not is_vs() then
		buildoptions{ "-std=c++14" }
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

		if os.istarget("windows") and not is_vs() then
			if ( true == use_ee_icon ) then
				linkoptions { "../../bin/assets/icon/ee.res" }
			end
		end

		if os.istarget("emscripten") then
			extension = ".html"

			if (	package_name ~= "eeew" and
					package_name ~= "eees" and
					package_name ~= "eehttp-request" and
					package_name ~= "eephysics" and
					package_name ~= "eevbo-fbo-batch"
			) then
				linkoptions { "--preload-file assets/" }
			end
		end
	end

	set_ios_config()
	set_xcode_config()

	filter "configurations:debug"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		symbols "On"

		if not is_vs() and not os.istarget("emscripten") then
			buildoptions{ "-Wall -Wno-long-long" }
		end

		targetname ( package_name .. "-debug" .. extension )

	filter "configurations:release"
		optimize "Speed"

		if not is_vs() and not os.istarget("emscripten") then
			buildoptions { "-fno-strict-aliasing -ffast-math" }
		end

		if not is_vs() and not os.istarget("emscripten") and not os.istarget("macosx") then
			buildoptions { "-s" }
		end

		targetname ( package_name .. extension )

	filter "system:windows"
		add_cross_config_links()

		if _OPTIONS["windows-vc-build"] then
			syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x86" }
		end

		if is_vs() and table.contains( backends, "SDL2" ) then
			links { "SDL2", "SDL2main" }
		end

	filter "system:emscripten"
		linkoptions{ "-O2 -s TOTAL_MEMORY=67108864 -s ASM_JS=1 -s VERBOSE=1 -s DISABLE_EXCEPTION_CATCHING=0 -s USE_SDL=2 -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s ERROR_ON_MISSING_LIBRARIES=0 -s FULL_ES3=1 -s \"BINARYEN_TRAP_MODE='clamp'\"" }
		buildoptions { "-fno-strict-aliasing -O2 -s USE_SDL=2 -s PRECISE_F32=1 -s ENVIRONMENT=web" }

		if _OPTIONS["with-gles1"] and ( not _OPTIONS["with-gles2"] or _OPTIONS["force-gles1"] ) then
			linkoptions{ "-s LEGACY_GL_EMULATION=1" }
		end

		if _OPTIONS["with-gles2"] and not _OPTIONS["force-gles1"] then
			linkoptions{ "-s FULL_ES2=1" }
		end
end

function generate_os_links()
	if os.istarget("linux") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "GL", "Xcursor" } )

		if _OPTIONS["with-static-eepp"] then
			table.insert( os_links, "dl" )
		end
	elseif os.istarget("windows") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32" } )
	elseif os.istarget("mingw32") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32" } )
	elseif os.istarget("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "CoreFoundation.framework" } )
	elseif os.istarget("bsd") then
		multiple_insert( os_links, { "rt", "pthread", "X11", "GL", "Xcursor" } )
	elseif os.istarget("haiku") then
		multiple_insert( os_links, { "GL", "network" } )
	elseif os.istarget("ios") then
		multiple_insert( os_links, { "OpenGLES.framework", "AudioToolbox.framework", "CoreAudio.framework", "Foundation.framework", "CoreFoundation.framework", "UIKit.framework", "QuartzCore.framework", "CoreGraphics.framework" } )
	elseif os.istarget("android") then
		multiple_insert( os_links, { "GLESv1_CM", "GLESv2", "log" } )
	end

	if not _OPTIONS["with-mojoal"] then
		if os.istarget("linux") or os.istarget("bsd") or os.istarget("haiku") or os.istarget("emscripten") then
			multiple_insert( os_links, { "openal" } )
		elseif os.istarget("windows") or os.istarget("mingw32") then
			multiple_insert( os_links, { "OpenAL32" } )
		elseif os.istarget("macosx") or os.istarget("ios") then
			multiple_insert( os_links, { "OpenAL.framework" } )
		end
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

	if not _OPTIONS["with-dynamic-freetype"] then
		links { "freetype-static" }
	end

	links { "SOIL2-static",
			"chipmunk-static",
			"libzip-static",
			"jpeg-compressor-static",
			"zlib-static",
			"imageresampler-static",
			"pugixml-static",
			"vorbis-static"
	}

	if _OPTIONS["with-mojoal"] then
		links { "mojoal-static"}
	end

	if not _OPTIONS["with-openssl"] then
		links { "mbedtls-static" }
	end

	if not os.istarget("haiku") and not os.istarget("ios") and not os.istarget("android") and not os.istarget("emscripten") then
		links{ "glew-static" }
	end
end

function can_add_static_backend( name )
	if _OPTIONS["with-static-backend"] then
		local path = "libs/" .. os.target() .. "/lib" .. name .. ".a"
		return os.isfile(path)
	end
end

function insert_static_backend( name )
	table.insert( static_backends, path.getrelative( "libs/" .. os.target(), "./" ) .. "/libs/" .. os.target() .. "/lib" .. name .. ".a" )
end

function add_sdl2()
	print("Using SDL2 backend");
	files { "src/eepp/window/backend/SDL2/*.cpp" }
	defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }

	if not can_add_static_backend("SDL2") then
		table.insert( link_list, get_backend_link_name( "SDL2" ) )
	else
		insert_static_backend( "SDL2" )
	end

	table.insert( backends, "SDL2" )
end

function set_xcode_config()
	if is_xcode() or _OPTIONS["use-frameworks"] then
		linkoptions { "-F /Library/Frameworks" }
		incdirs { "/Library/Frameworks/SDL2.framework/Headers" }
		defines { "EE_SDL2_FROM_ROOTPATH" }
	end
end

function set_ios_config()
	if _OPTIONS.platform == "ios-arm7" or _OPTIONS.platform == "ios-x86" then
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
		local sysroot_ver = " -miphoneos-version-min=" .. os.getenv("IOSVERSION") .. " -isysroot " .. sysroot_path

		buildoptions { sysroot_ver .. " -I" .. sysroot_path .. "/usr/include" }
		linkoptions { sysroot_ver }
		libdirs { framework_libs_path }
		linkoptions { " -F" .. framework_path .. " -L" .. framework_libs_path .. " -isysroot " .. sysroot_path }
		incdirs { "src/thirdparty/SDL2/include" }
	end

	if _OPTIONS.platform == "ios-cross-arm7" or _OPTIONS.platform == "ios-cross-x86" then
		incdirs { "src/thirdparty/SDL2/include" }
	end
end

function backend_is( name, libname )
	if not _OPTIONS["with-backend"] then
		_OPTIONS["with-backend"] = "SDL2"
	end

	if next(backends) == nil then
		backends = string.explode(_OPTIONS["with-backend"],",")
	end

	local backend_sel = table.contains( backends, name )

	local ret_val = os_findlib( libname ) and backend_sel

	if os.istarget("mingw32") or os.istarget("emscripten") then
		ret_val = backend_sel
	end

	if ret_val then
		backend_selected = true
	end

	return ret_val
end

function select_backend()
	if backend_is("SDL2", "SDL2") then
		print("Selected SDL2")
		add_sdl2()
	end

	-- If the selected backend is not present, try to find one present
	if not backend_selected then
		if os_findlib("SDL2", "SDL2") then
			add_sdl2()
		else
			print("ERROR: Couldnt find any backend. Forced SDL2.")
			add_sdl2()
		end
	end
end

function check_ssl_support()
	if _OPTIONS["with-openssl"] then
		if os.istarget("windows") then
			table.insert( link_list, get_backend_link_name( "libssl" ) )
			table.insert( link_list, get_backend_link_name( "libcrypto" ) )
		else
			table.insert( link_list, get_backend_link_name( "ssl" ) )
			table.insert( link_list, get_backend_link_name( "crypto" ) )
		end

		files { "src/eepp/network/ssl/backend/openssl/*.cpp" }

		defines { "EE_OPENSSL" }
	else
		files { "src/eepp/network/ssl/backend/mbedtls/*.cpp" }

		defines { "EE_MBEDTLS" }
	end

	defines { "EE_SSL_SUPPORT" }
end

function set_macos_and_ios_config()
	if os.istarget("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		libdirs { "/System/Library/Frameworks", "/Library/Frameworks" }
	end

	if _OPTIONS["use-frameworks"] then
		defines { "EE_USE_FRAMEWORKS" }
	end
end

function build_eepp( build_name )
	incdirs { "include", "src", "src/thirdparty", "include/eepp/thirdparty", "src/thirdparty/freetype2/include", "src/thirdparty/zlib", "src/thirdparty/libogg/include", "src/thirdparty/libvorbis/include", "src/thirdparty/mbedtls/include" }

	if _OPTIONS["with-mojoal"] then
		defines( "AL_LIBTYPE_STATIC" )
		incdirs { "src/thirdparty/mojoAL" }
	end

	if _OPTIONS["windows-vc-build"] then
		incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
	end

	set_macos_and_ios_config()
	set_ios_config()
	set_xcode_config()

	add_static_links()

	if is_vs() then
		incdirs { "src/thirdparty/libzip/vs" }
	end

	if not is_vs() then
		buildoptions{ "-std=c++14" }
	end

	if os.istarget("windows") then
		files { "src/eepp/system/platform/win/*.cpp" }
		files { "src/eepp/network/platform/win/*.cpp" }
	else
		files { "src/eepp/system/platform/posix/*.cpp" }
		files { "src/eepp/network/platform/unix/*.cpp" }
	end

	files { "src/eepp/core/*.cpp",
			"src/eepp/math/*.cpp",
			"src/eepp/system/*.cpp",
			"src/eepp/audio/*.cpp",
			"src/eepp/graphics/*.cpp",
			"src/eepp/graphics/renderer/*.cpp",
			"src/eepp/window/*.cpp",
			"src/eepp/network/*.cpp",
			"src/eepp/network/ssl/*.cpp",
			"src/eepp/network/http/*.cpp",
			"src/eepp/scene/*.cpp",
			"src/eepp/scene/actions/*.cpp",
			"src/eepp/ui/*.cpp",
			"src/eepp/ui/actions/*.cpp",
			"src/eepp/ui/css/*.cpp",
			"src/eepp/ui/tools/*.cpp",
			"src/eepp/physics/*.cpp",
			"src/eepp/physics/constraints/*.cpp",
			"src/eepp/maps/*.cpp",
			"src/eepp/maps/mapeditor/*.cpp"
	}

	check_ssl_support()

	select_backend()

	if _OPTIONS["with-dynamic-freetype"] and os_findlib("freetype") then
		table.insert( link_list, get_backend_link_name( "freetype" ) )
	end

	multiple_insert( link_list, os_links )

	links { link_list }

	build_link_configuration( build_name )

	configuration "emscripten"
		if _OPTIONS["force-gles1"] then
			defines{ "EE_GLES1_DEFAULT" }
		end
end

function set_targetdir( dir )
	if os.istarget("ios") then
		targetdir(dir .. get_ios_arch() .. "/" )
	else
		targetdir(dir)
	end
end

workspace "eepp"
	targetdir("./bin/")
	configurations { "debug", "release" }
	rtti "On"

	download_and_extract_dependencies()

	if os.istarget("android") then
		ndkabi "arm64-v8a"
		ndkplatform "android-28"
		ndkstl "c++_static"
	end

	if os.istarget("ios") then
		location("./make/" .. os.target() .. "/" )
		objdir("obj/" .. os.target() .. "/" .. get_ios_arch() .. "/" )
	else
		location("./make/" .. os.target() .. "/")
		objdir("obj/" .. os.target() .. "/")
	end

	generate_os_links()
	parse_args()

	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:release"
		optimize "Speed"

	project "SOIL2-static"
		kind "StaticLib"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/SOIL2/src/SOIL2/*.c" }
		incdirs { "src/thirdparty/SOIL2" }
		build_base_configuration( "SOIL2" )
		filter "action:vs*"
			language "C++"
			buildoptions { "/TP" }
		filter "action:not vs*"
			language "C"

	project "glew-static"
		kind "StaticLib"
		language "C"
		defines { "GLEW_NO_GLU", "GLEW_STATIC" }
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/glew/*.c" }
		incdirs { "include/thirdparty/glew" }
		build_base_configuration( "glew" )

	project "mbedtls-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "src/thirdparty/mbedtls/include/" }
		files { "src/thirdparty/mbedtls/library/*.c" }
		build_base_cpp_configuration( "mbedtls" )

	project "vorbis-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "src/thirdparty/libvorbis/lib/", "src/thirdparty/libogg/include", "src/thirdparty/libvorbis/include" }
		files { "src/thirdparty/libogg/**.c", "src/thirdparty/libvorbis/**.c" }
		build_base_cpp_configuration( "vorbis" )

	project "pugixml-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/pugixml/*.cpp" }
		build_base_cpp_configuration( "pugixml" )

	project "zlib-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/zlib/*.c" }
		build_base_configuration( "zlib" )

	project "libzip-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/libzip/*.c" }
		incdirs { "src/thirdparty/zlib" }
		build_base_configuration( "libzip" )

	project "freetype-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/thirdparty/freetype2/src/**.c" }
		incdirs { "src/thirdparty/freetype2/include" }
		build_base_configuration( "freetype" )

	project "chipmunk-static"
		kind "StaticLib"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/chipmunk/*.c", "src/thirdparty/chipmunk/constraints/*.c" }
		incdirs { "include/eepp/thirdparty/chipmunk" }
		build_base_configuration( "chipmunk" )
		filter "action:vs*"
			language "C++"
			buildoptions { "/TP" }
		filter "action:not vs*"
			language "C"

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/jpeg-compressor/*.cpp" }
		build_base_cpp_configuration( "jpeg-compressor" )

	project "imageresampler-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/imageresampler/*.cpp" }
		build_base_cpp_configuration( "imageresampler" )

	project "mojoal-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "include/eepp/thirdparty/mojoAL" }
		defines( "AL_LIBTYPE_STATIC" )
		files { "src/thirdparty/mojoAL/*.c" }
		build_base_cpp_configuration( "mojoal" )
		filter "options:windows-vc-build"
			incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }

	project "efsw-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty/efsw/src" }
		files { "src/thirdparty/efsw/src/efsw/*.cpp" }
		build_base_cpp_configuration( "efsw" )
		filter "system:windows"
			files { "src/thirdparty/efsw/src/efsw/platform/win/*.cpp" }
			excludes { "src/thirdparty/efsw/src/efsw/WatcherKqueue.cpp", "src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp", "src/thirdparty/efsw/src/efsw/WatcherInotify.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherKqueue.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp" }
		filter "system:linux"
			excludes { "src/thirdparty/efsw/src/efsw/WatcherKqueue.cpp", "src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp", "src/thirdparty/efsw/src/efsw/WatcherWin32.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherKqueue.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp" }
		filter "system:macosx"
			excludes { "src/thirdparty/efsw/src/efsw/WatcherInotify.cpp", "src/thirdparty/efsw/src/efsw/WatcherWin32.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp" }
		filter "system:bsd"
			excludes { "src/thirdparty/efsw/src/efsw/WatcherInotify.cpp", "src/thirdparty/efsw/src/efsw/WatcherWin32.cpp", "src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp", "src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp" }
		filter "system:not windows"
			files { "src/thirdparty/efsw/src/efsw/platform/posix/*.cpp" }

	project "eepp-main"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/")
		files { "src/eepp/main/eepp_main.cpp" }

	project "eepp-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/")
		build_eepp( "eepp-static" )

	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		set_targetdir("libs/" .. os.target() .. "/")
		build_eepp( "eepp" )

	-- Examples
	project "eepp-test"
		set_kind()
		language "C++"
		files { "src/test/*.cpp" }
		build_link_configuration( "eetest", true )

	project "eepp-es"
		set_kind()
		language "C++"
		files { "src/examples/external_shader/*.cpp" }
		build_link_configuration( "eees", true )

	project "eepp-ew"
		set_kind()
		language "C++"
		files { "src/examples/empty_window/*.cpp" }
		build_link_configuration( "eeew", true )

	project "eepp-sound"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/sound/*.cpp" }
		build_link_configuration( "eesound", true )

	project "eepp-sprites"
		set_kind()
		language "C++"
		files { "src/examples/sprites/*.cpp" }
		build_link_configuration( "eesprites", true )

	project "eepp-fonts"
		set_kind()
		language "C++"
		files { "src/examples/fonts/*.cpp" }
		build_link_configuration( "eefonts", true )

	project "eepp-vbo-fbo-batch"
		set_kind()
		language "C++"
		files { "src/examples/vbo_fbo_batch/*.cpp" }
		build_link_configuration( "eevbo-fbo-batch", true )

	project "eepp-physics"
		set_kind()
		language "C++"
		files { "src/examples/physics/*.cpp" }
		build_link_configuration( "eephysics", true )

	project "eepp-http-request"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/http_request/*.cpp" }
		incdirs { "src/thirdparty" }
		build_link_configuration( "eehttp-request", true )

	project "eepp-ui-hello-world"
		set_kind()
		language "C++"
		files { "src/examples/ui_hello_world/*.cpp" }
		incdirs { "src/thirdparty" }
		build_link_configuration( "eeui-hello-world", true )

	-- Tools
	project "eepp-textureatlaseditor"
		set_kind()
		language "C++"
		files { "src/tools/textureatlaseditor/*.cpp" }
		build_link_configuration( "eepp-TextureAtlasEditor", true )

	project "eepp-mapeditor"
		set_kind()
		language "C++"
		files { "src/tools/mapeditor/*.cpp" }
		build_link_configuration( "eepp-MapEditor", true )

	project "eepp-uieditor"
		set_kind()
		language "C++"
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty" }
		links { "efsw-static", "pugixml-static" }
		files { "src/tools/uieditor/*.cpp" }
		build_link_configuration( "eepp-UIEditor", true )
		filter "system:macosx"
			links { "CoreFoundation.framework", "CoreServices.framework" }
		filter { "system:not windows", "system:not haiku" }
			links { "pthread" }

if os.isfile("external_projects.lua") then
	dofile("external_projects.lua")
end
