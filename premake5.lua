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
remote_sdl2_devel_src_url = "https://libsdl.org/release/SDL2-2.0.10.zip"
remote_sdl2_devel_vc_url = "https://www.libsdl.org/release/SDL2-devel-2.0.10-VC.zip"

function incdirs( dirs )
	if is_xcode() then
		sysincludedirs { dirs }
	end
	includedirs { dirs }
end

function download_and_extract_sdl(sdl_url)
	print("Downloading: " .. sdl_url)
	local dest_dir = "src/thirdparty/"
	local local_file = dest_dir .. remote_sdl2_version .. ".zip"
	local result_str, response_code = http.download(sdl_url, local_file)
	if response_code == 200 then
		print("Downloaded successfully to: " .. local_file)
		zip.extract(local_file, dest_dir)
		print("Extracted " .. local_file .. " into " .. dest_dir)
	else
		print("Failed to download:  " .. sdl_url)
		exit(1)
	end
end

function download_and_extract_dependencies()
	if not os.isdir("src/thirdparty/" .. remote_sdl2_version) then
		if _OPTIONS["windows-vc-build"] then
			download_and_extract_sdl(remote_sdl2_devel_vc_url)
		elseif os.istarget("ios") then
			download_and_extract_sdl(remote_sdl2_devel_src_url)
			os.execute("patch -t --forward -p1 -d src/thirdparty/SDL2-2.0.10/ < projects/ios/SDL2-sensors.patch")
		end
	end
end

function build_base_configuration( package_name )
	incdirs { "src/thirdparty/zlib" }

	set_ios_config()
	set_xcode_config()

	filter "not system:windows"
		buildoptions{ "-fPIC" }

	filter "configurations:debug*"
		targetname ( package_name .. "-debug" )

	filter "configurations:release*"
		targetname ( package_name )

	filter "action:not vs*"
		cdialect "gnu99"
		buildoptions { "-Wall" }

	filter "action:vs*"
		incdirs { "src/thirdparty/libzip/vs" }
end

function build_base_cpp_configuration( package_name )
	if not os.istarget("windows") then
		buildoptions{ "-fPIC" }
	end

	set_ios_config()
	set_xcode_config()

	filter "action:not vs*"
		buildoptions { "-Wall" }

	filter "configurations:debug*"
		defines { "DEBUG" }
		symbols "On"
		targetname ( package_name .. "-debug" )

	filter "configurations:release*"
		optimize "Speed"
		targetname ( package_name )
end

function build_link_configuration( package_name, use_ee_icon )
	incdirs { "include" }
	local extension = "";

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
	end

	set_ios_config()
	set_xcode_config()

	filter { "system:windows", "action:not vs*" }
		if ( true == use_ee_icon ) then
			linkoptions { "../../bin/assets/icon/ee.res" }
		end

	filter "action:not vs*"
		cppdialect "C++14"
		buildoptions { "-Wall" }

	filter { "configurations:debug*", "action:not vs*" }
		buildoptions{ "-Wno-long-long" }

	filter { "configurations:release*", "action:not vs*" }
		buildoptions { "-fno-strict-aliasing -ffast-math" }

	filter { "configurations:release*", "action:not vs*", "system:not macosx" }
		buildoptions { "-s" }

	filter "configurations:debug*"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		targetname ( package_name .. "-debug" .. extension )

	filter "configurations:release*"
		targetname ( package_name .. extension )

	filter { "system:windows", "action:not vs*" }
		linkoptions { "-static-libgcc", "-static-libstdc++" }

	filter { "system:windows", "action:vs*" }
		if table.contains( backends, "SDL2" ) then
			links { "SDL2", "SDL2main" }
		end

	filter { "options:windows-vc-build", "system:windows", "platforms:x86" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x86" }

	filter { "options:windows-vc-build", "system:windows", "platforms:x86_64" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x64" }

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
		multiple_insert( os_links, { "OpenGLES.framework", "AudioToolbox.framework", "CoreAudio.framework", "Foundation.framework", "CoreFoundation.framework", "UIKit.framework", "QuartzCore.framework", "CoreGraphics.framework", "CoreMotion.framework", "AVFoundation.framework", "GameController.framework" } )
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
		return true
	end
	return false
end

function insert_static_backend( name )
	table.insert( static_backends, "../../libs/" .. os.target() .. "/lib" .. name .. ".a" )
end

function add_sdl2()
	print("Using SDL2 backend");
	if not can_add_static_backend("SDL2") then
		table.insert( link_list, get_backend_link_name( "SDL2" ) )
	else
		print("Using static backend")
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
	if os.istarget("ios") then
		local toolchainpath = os.getenv("TOOLCHAINPATH")
		local iosversion = os.getenv("IOSVERSION")
		local sysroot_path = os.getenv("SYSROOTPATH")

		if nil == os.getenv("TOOLCHAINPATH") then
			toolchainpath = os.outputof("xcrun -find -sdk iphonesimulator clang")
		end

		if nil == os.getenv("IOSVERSION") then
			iosversion = os.outputof("xcrun --sdk iphonesimulator --show-sdk-version")
		end

		if nil == os.getenv("SYSROOTPATH") then
			local platform_path = os.outputof("xcrun --sdk iphonesimulator --show-sdk-platform-path")
			sysroot_path = platform_path .. "/Developer/SDKs/iPhoneSimulator" .. iosversion .. ".sdk/"
		end

		local framework_path = sysroot_path .. "/System/Library/Frameworks"
		local framework_libs_path = framework_path .. "/usr/lib"
		local sysroot_ver = " -miphoneos-version-min=9.0 -isysroot " .. sysroot_path

		buildoptions { sysroot_ver .. " -I" .. sysroot_path .. "/usr/include" }
		linkoptions { sysroot_ver }
		libdirs { framework_libs_path }
		linkoptions { " -F" .. framework_path .. " -L" .. framework_libs_path .. " -isysroot " .. sysroot_path }
		includedirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
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

function build_eepp( build_name )
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

	incdirs {	"include",
				"src",
				"src/thirdparty",
				"include/eepp/thirdparty",
				"src/thirdparty/freetype2/include",
				"src/thirdparty/zlib",
				"src/thirdparty/libogg/include",
				"src/thirdparty/libvorbis/include",
				"src/thirdparty/mbedtls/include"
	}

	add_static_links()
	check_ssl_support()

	if table.contains( backends, "SDL2" ) then
		files { "src/eepp/window/backend/SDL2/*.cpp" }
		defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }
	end

	multiple_insert( link_list, os_links )

	links { link_list }

	build_link_configuration( build_name )

	filter "options:use-frameworks"
		defines { "EE_USE_FRAMEWORKS" }

	filter { "system:macosx", "action:xcode* or options:use-frameworks" }
		libdirs { "/System/Library/Frameworks", "/Library/Frameworks" }

	filter "system:windows"
		files { "src/eepp/system/platform/win/*.cpp" }
		files { "src/eepp/network/platform/win/*.cpp" }

	filter "system:not windows"
		files { "src/eepp/system/platform/posix/*.cpp" }
		files { "src/eepp/network/platform/unix/*.cpp" }

	filter "options:with-mojoal"
		defines( "AL_LIBTYPE_STATIC" )
		incdirs { "src/thirdparty/mojoAL" }

	filter "options:windows-vc-build"
		incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }

	filter "action:vs*"
		incdirs { "src/thirdparty/libzip/vs" }

	filter "action:not vs*"
		cppdialect "C++14"

	filter "options:with-dynamic-freetype"
		if not os.istarget("ios") and os_findlib("freetype") then
			table.insert( link_list, get_backend_link_name( "freetype" ) )
		end
end

workspace "eepp"
	targetdir("./bin/")
	if os.istarget("ios") then
		configurations { "debug-x64", "debug-arm64", "release-x64", "release-arm64" }
		platforms { "x86_64", "arm64" }
	else
		configurations { "debug", "release" }
		platforms { "x86_64", "x86" }
	end
	rtti "On"
	download_and_extract_dependencies()
	select_backend()
	generate_os_links()
	parse_args()
	location("./make/" .. os.target() .. "/")
	objdir("obj/" .. os.target() .. "/")

	filter "platforms:x86"
		architecture "x86"

	filter "platforms:arm64 or configurations:debug-arm64 or configurations:release-arm64"
		architecture "arm64"

	filter "platforms:x86_64 or configurations:debug-x64 or configurations:release-x64"
		architecture "x86_64"

	filter "system:macosx"
		defines { "GL_SILENCE_DEPRECATION" }

	filter "configurations:debug*"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:release*"
		optimize "Speed"

	project "SOIL2-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/SOIL2/src/SOIL2/*.c" }
		incdirs { "src/thirdparty/SOIL2" }
		build_base_configuration( "SOIL2" )

	project "glew-static"
		kind "StaticLib"
		language "C"
		defines { "GLEW_NO_GLU", "GLEW_STATIC" }
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/glew/*.c" }
		incdirs { "include/thirdparty/glew" }
		build_base_configuration( "glew" )

	project "mbedtls-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "src/thirdparty/mbedtls/include/" }
		files { "src/thirdparty/mbedtls/library/*.c" }
		build_base_cpp_configuration( "mbedtls" )

	project "vorbis-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "src/thirdparty/libvorbis/lib/", "src/thirdparty/libogg/include", "src/thirdparty/libvorbis/include" }
		files { "src/thirdparty/libogg/**.c", "src/thirdparty/libvorbis/**.c" }
		build_base_cpp_configuration( "vorbis" )

	project "pugixml-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/pugixml/*.cpp" }
		build_base_cpp_configuration( "pugixml" )

	project "zlib-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/zlib/*.c" }
		build_base_configuration( "zlib" )

	project "libzip-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/libzip/*.c" }
		incdirs { "src/thirdparty/zlib" }
		build_base_configuration( "libzip" )

	project "freetype-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/thirdparty/freetype2/src/**.c" }
		incdirs { "src/thirdparty/freetype2/include" }
		build_base_configuration( "freetype" )

	project "chipmunk-static"
		kind "StaticLib"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
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
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/jpeg-compressor/*.cpp" }
		build_base_cpp_configuration( "jpeg-compressor" )

	project "imageresampler-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		files { "src/thirdparty/imageresampler/*.cpp" }
		build_base_cpp_configuration( "imageresampler" )

	project "mojoal-static"
		kind "StaticLib"
		language "C"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "include/eepp/thirdparty/mojoAL" }
		defines( "AL_LIBTYPE_STATIC" )
		files { "src/thirdparty/mojoAL/*.c" }
		build_base_cpp_configuration( "mojoal" )
		filter "options:windows-vc-build"
			incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }

	project "efsw-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.target() .. "/thirdparty/")
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty/efsw/src" }
		files { "src/thirdparty/efsw/src/efsw/*.cpp" }
		build_base_cpp_configuration( "efsw" )
		filter "system:windows"
			files { "src/thirdparty/efsw/src/efsw/platform/win/*.cpp" }
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp"
			}
		filter "system:linux"
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp"
			}
		filter "system:macosx or system:ios"
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp"
			}
		filter "system:bsd"
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp"
			}
		filter "system:not windows"
			files { "src/thirdparty/efsw/src/efsw/platform/posix/*.cpp" }

	-- Library
	project "eepp-main"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.target() .. "/")
		files { "src/eepp/main/eepp_main.cpp" }

	project "eepp-static"
		kind "StaticLib"
		language "C++"
		targetdir("libs/" .. os.target() .. "/")
		build_eepp( "eepp-static" )

	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		targetdir("libs/" .. os.target() .. "/")
		build_eepp( "eepp" )

	-- Examples
	project "eepp-external-shader"
		set_kind()
		language "C++"
		files { "src/examples/external_shader/*.cpp" }
		build_link_configuration( "eepp-external-shader", true )

	project "eepp-empty-window"
		set_kind()
		language "C++"
		files { "src/examples/empty_window/*.cpp" }
		build_link_configuration( "eepp-empty-window", true )

	project "eepp-sound"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/sound/*.cpp" }
		build_link_configuration( "eepp-sound", true )

	project "eepp-sprites"
		set_kind()
		language "C++"
		files { "src/examples/sprites/*.cpp" }
		build_link_configuration( "eepp-sprites", true )

	project "eepp-fonts"
		set_kind()
		language "C++"
		files { "src/examples/fonts/*.cpp" }
		build_link_configuration( "eepp-fonts", true )

	project "eepp-vbo-fbo-batch"
		set_kind()
		language "C++"
		files { "src/examples/vbo_fbo_batch/*.cpp" }
		build_link_configuration( "eepp-vbo-fbo-batch", true )

	project "eepp-physics"
		set_kind()
		language "C++"
		files { "src/examples/physics/*.cpp" }
		build_link_configuration( "eepp-physics", true )

	project "eepp-http-request"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/http_request/*.cpp" }
		incdirs { "src/thirdparty" }
		build_link_configuration( "eepp-http-request", true )

	project "eepp-ui-hello-world"
		set_kind()
		language "C++"
		files { "src/examples/ui_hello_world/*.cpp" }
		incdirs { "src/thirdparty" }
		build_link_configuration( "eepp-ui-hello-world", true )

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

	project "eepp-texturepacker"
		kind "ConsoleApp"
		language "C++"
		incdirs { "src/thirdparty" }
		files { "src/tools/texturepacker/*.cpp" }
		build_link_configuration( "eepp-TexturePacker", true )

	-- Tests
	project "eepp-test"
		set_kind()
		language "C++"
		files { "src/tests/test_all/*.cpp" }
		build_link_configuration( "eepp-test", true )

	project "eepp-ui-perf-test"
		set_kind()
		language "C++"
		files { "src/tests/ui_perf_test/*.cpp" }
		includedirs { "src/thirdparty" }
		build_link_configuration( "eepp-ui-perf-test", true )

if os.isfile("external_projects.lua") then
	dofile("external_projects.lua")
end
