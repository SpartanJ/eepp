require "premake.export-compile-commands.export-compile-commands"
require "premake.premake-cmake.cmake"
require "premake.premake-ninja.ninja"

newoption { trigger = "with-openssl", description = "Enables OpenSSL support ( and disables mbedtls backend )." }
newoption { trigger = "with-dynamic-freetype", description = "Dynamic link against freetype." }
newoption { trigger = "with-static-eepp", description = "Force to build the demos and tests with eepp compiled statically" }
newoption { trigger = "with-static-backend", description = "It will try to compile the library with a static backend (only for gcc and mingw).\n\t\t\t\tThe backend should be placed in libs/your_platform/libYourBackend.a" }
newoption { trigger = "with-gles2", description = "Compile with GLES2 support" }
newoption { trigger = "with-gles1", description = "Compile with GLES1 support" }
newoption { trigger = "without-mojoal", description = "Compile without mojoAL as OpenAL implementation (that requires SDL2 backend). Instead it will use openal-soft." }
newoption { trigger = "use-frameworks", description = "In macOS it will try to link the external libraries from its frameworks. For example, instead of linking against SDL2 it will link against SDL2.framework." }
newoption { trigger = "windows-vc-build", description = "This is used to build the framework in Visual Studio downloading its external dependencies and making them available to the VS project without having to install them manually." }
newoption { trigger = "windows-mingw-build", description = "This is used to build the framework with mingw downloading its external dependencies." }
newoption { trigger = "with-mold-linker", description = "Tries to use the mold linker instead of the default linker of the toolchain" }
newoption { trigger = "with-debug-symbols", description = "Release builds are built with debug symbols." }
newoption { trigger = "thread-sanitizer", description = "Compile with ThreadSanitizer." }
newoption { trigger = "address-sanitizer", description = "Compile with AddressSanitizer." }
newoption { trigger = "time-trace", description = "Compile with time tracing." }
newoption { trigger = "disable-static-build", description = "Disables eepp static build project, this is just a helper to avoid rebuilding twice eepp while developing the library." }
newoption { trigger = "without-text-shaper", description = "Disables text-shaping capabilities." }
newoption {
	trigger = "with-backend",
	description = "Select the backend to use for window and input handling.\n\t\t\tIf no backend is selected or if the selected is not installed the script will search for a backend present in the system, and will use it.",
	allowed = {
		{ "SDL2",  "SDL2" },
	}
}
newoption {
    trigger     = "sharedir",
    value       = "PATH",
    description = "Set the shared data directory",
}
newoption { trigger = "with-static-cpp", description = "Builds statically libstdc++" }

function get_dll_extension()
	if os.target() == "macosx" then
		return "dylib"
	elseif os.target() == "windows" then
		return "dll"
	else
		return "so"
	end
end

function get_host()
	if os.getenv("MSYSTEM") ~= "" and not is_vs() then
		return "msys"
	end
	return os.host()
end

function os_ishost(host)
	return get_host() == host
end

function postsymlinklib(src_path, dst_path, lib, arch)
	filter { "configurations:release*", "system:windows", arch }
		if os_ishost("windows") then
			postbuildcommands { "mklink \"" .. dst_path .. lib .. ".dll\"" .. " \"" .. src_path .. lib .. ".dll\" || ver>nul" }
		else
			postbuildcommands { "ln -sf \"" .. src_path .. lib .. "." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }
		end

	filter { "configurations:debug*", "system:windows", arch }
		if os_ishost("windows") then
			postbuildcommands { "mklink \"" .. dst_path .. lib .. "-debug.dll\"" .. " \"" .. src_path .. lib .. "-debug.dll\" || ver>nul" }
		else
			postbuildcommands { "ln -sf \"" .. src_path .. lib .. "-debug." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }
		end

	filter { "configurations:release*", "not system:windows", arch }
		postbuildcommands { "ln -sf \"" .. src_path .. "lib" .. lib .. "." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }

	filter { "configurations:debug*", "not system:windows", arch }
		postbuildcommands { "ln -sf \"" .. src_path .. "lib" .. lib .. "-debug." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }

	filter {}
end

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
		local path = _MAIN_SCRIPT_DIR .. "src/thirdparty/" .. name .. ".framework"

		if os.isdir( path ) then
			return path
		end

		path = "/Library/Frameworks/" .. name .. ".framework"

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
	cppdialect "C++20"
end

link_list = { }
os_links = { }
backends = { }
static_backends = { }
backend_selected = false
remote_sdl2_version_number = "2.32.8"
remote_sdl2_version = "SDL2-" .. remote_sdl2_version_number
remote_sdl2_devel_src_url = "https://libsdl.org/release/" .. remote_sdl2_version .. ".zip"
remote_sdl2_devel_vc_url = "https://www.libsdl.org/release/SDL2-devel-" .. remote_sdl2_version_number .. "-VC.zip"
remote_sdl2_devel_vc_arm64_url = "https://github.com/mmozeiko/build-sdl2/releases/download/2025-12-14/SDL2-arm64-2025-12-14.zip"
remote_sdl2_devel_mingw_url = "https://www.libsdl.org/release/SDL2-devel-" .. remote_sdl2_version_number .. "-mingw.zip"
remote_sdl2_arm64_cross_tools_path = "/usr/local/cross-tools/aarch64-w64-mingw32"

function incdirs( dirs )
	if is_xcode() then
		externalincludedirs { dirs }
	end
	includedirs { dirs }
end

function popen( executable_path )
	local handle = io.popen(executable_path)
	local result = handle:read("*a")
	handle:close()
	return result
end

function download_and_extract_sdl(sdl_url)
	print("Downloading: " .. sdl_url)
	local dest_dir = "src/thirdparty/"
	local local_file = dest_dir .. remote_sdl2_version .. ".zip"
	local res, response_code = http.download(sdl_url, local_file)
	if response_code == 200 then
		print("Downloaded successfully to: " .. local_file)
		zip.extract(local_file, dest_dir)
		print("Extracted " .. local_file .. " into " .. dest_dir)
	else
		print("Failed to download: " .. sdl_url .. " res: " .. res)
		exit(1)
	end
end

function copy_sdl()
	if _OPTIONS["windows-vc-build"] and _OPTIONS["arch"] == "arm64" then
		os.copyfile( _MAIN_SCRIPT_DIR .. "/src/thirdparty/" .. remote_sdl2_version .."/bin/SDL2.dll", _MAIN_SCRIPT_DIR .. "/bin/SDL2.dll" )
		os.copyfile( _MAIN_SCRIPT_DIR .. "/src/thirdparty/" .. remote_sdl2_version .."/bin/SDL2.dll", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/SDL2.dll" )
	elseif _OPTIONS["windows-vc-build"] then
		os.copyfile( _MAIN_SCRIPT_DIR .. "/src/thirdparty/" .. remote_sdl2_version .."/lib/x64/SDL2.dll", _MAIN_SCRIPT_DIR .. "/bin/SDL2.dll" )
		os.copyfile( _MAIN_SCRIPT_DIR .. "/src/thirdparty/" .. remote_sdl2_version .."/lib/x64/SDL2.dll", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/SDL2.dll" )
	elseif _OPTIONS["windows-mingw-build"] and _OPTIONS["arch"] ~= "arm64" then
		os.copyfile( _MAIN_SCRIPT_DIR .. "/src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/bin/SDL2.dll", _MAIN_SCRIPT_DIR .. "/bin/SDL2.dll" )
		os.copyfile( _MAIN_SCRIPT_DIR .. "/src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/bin/SDL2.dll", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/SDL2.dll" )
	end
end

function version_to_number( version )
	versionpart = 0
	versionnum = 0
	versionmod = 1000
	for str in string.gmatch(version, "[^%.]+") do
		versionnum = versionnum + tonumber(str) * versionmod
		versionpart = versionpart + 1
		if versionpart == 1 then
			versionmod = 100
		elseif versionpart == 2 then
			versionmod = 10
		end
	end
	return versionnum
end

function download_and_extract_dependencies()
	if _OPTIONS["windows-vc-build"] and _OPTIONS["arch"] == "arm64" then
		remote_sdl2_version = "SDL2-arm64"
	end

	if not os.isdir("src/thirdparty/" .. remote_sdl2_version) then
		if _OPTIONS["windows-vc-build"] and _OPTIONS["arch"] == "arm64" then
			download_and_extract_sdl(remote_sdl2_devel_vc_arm64_url)
			copy_sdl()
		elseif _OPTIONS["windows-vc-build"] then
			download_and_extract_sdl(remote_sdl2_devel_vc_url)
			copy_sdl()
		elseif _OPTIONS["windows-mingw-build"] and _OPTIONS["arch"] ~= "arm64" then
			download_and_extract_sdl(remote_sdl2_devel_mingw_url)
			copy_sdl()
		elseif _OPTIONS["windows-mingw-build"] and _OPTIONS["arch"] == "arm64" then
			download_and_extract_sdl(remote_sdl2_devel_src_url)
		elseif os.istarget("ios") then
			download_and_extract_sdl(remote_sdl2_devel_src_url)
		end
	end
end

function build_arch_configuration()
	filter {"architecture:x86", "options:cc=mingw"}
		buildoptions { "-D__USE_MINGW_ANSI_STDIO=1 -B /usr/bin/i686-w64-mingw32-" }

	filter {"architecture:x86_64", "options:cc=mingw"}
		if _OPTIONS["arch"] ~= "arm64" then
			buildoptions { "-D__USE_MINGW_ANSI_STDIO=1 -B /usr/bin/x86_64-w64-mingw32-" }
		end

	filter {}
end

function build_base_configuration( package_name )
	incdirs { "src/thirdparty/zlib" }

	set_ios_config()
	set_apple_config()
	build_arch_configuration()

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
		buildoptions { "/utf-8" }

	filter "system:emscripten"
		buildoptions { "-O3 -s USE_SDL=2 -s PRECISE_F32=1 -s ENVIRONMENT=worker,web" }
		buildoptions { "-s USE_PTHREADS=1" }

	filter {}
end

function build_base_cpp_configuration( package_name )
	if not os.istarget("windows") then
		buildoptions{ "-fPIC" }
	end

	set_ios_config()
	set_apple_config()
	build_arch_configuration()

	if _OPTIONS["with-static-eepp"] then
		defines { "EE_STATIC" }
	end

	filter "action:vs*"
		buildoptions{ "/std:c++20", "/utf-8" }

	filter "action:not vs*"
		cppdialect "C++20"
		buildoptions { "-Wall" }

	filter "configurations:debug*"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		symbols "On"
		targetname ( package_name .. "-debug" )

	filter "configurations:release*"
		defines { "NDEBUG" }
		optimize "Speed"
		targetname ( package_name )

	filter { "configurations:release*", "options:with-debug-symbols" }
		symbols "On"

	filter "system:emscripten"
		buildoptions { "-O3 -s USE_SDL=2 -s PRECISE_F32=1 -s ENVIRONMENT=worker,web" }
		buildoptions { "-s USE_PTHREADS=1" }

	filter {}
end

function get_architecture()
	if jit then
		return jit.arch
	end

	local handle = io.popen("uname -m 2>/dev/null")
	if handle then
		local arch = handle:read("*l")
		handle:close()
		if arch then return arch end
	end

	local arch = os.getenv("PROCESSOR_ARCHITECTURE")
	if arch then
		if arch == "AMD64" or arch == "IA64" then
		  return "x86_64"
		end
		return string.lower(arch)
	end

	return "x86_64"
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

	if _OPTIONS["with-mold-linker"] then
		if _OPTIONS.platform == "clang" or _OPTIONS.platform == "clang-analyzer" then
			linkoptions { "-fuse-ld=mold" }
		else
			gccversion = os.outputof( "gcc -dumpfullversion" )
			gccversionnumber = version_to_number( gccversion )
			if gccversionnumber >= 12110 then
				linkoptions { "-fuse-ld=mold" }
			else
				linkoptions { "-B/usr/bin/mold" }
			end
		end
	end

	if not _OPTIONS["without-text-shaper"] then
		defines { "EE_TEXT_SHAPER_ENABLED" }
	end

	if _OPTIONS["with-static-cpp"] then
		linkoptions { "-static-libgcc -static-libstdc++" }
	end

	if _OPTIONS["sharedir"] then
		defines { 'ECODE_SHAREDIR="' .. _OPTIONS["sharedir"] .. '"' }
	end

	cppdialect "C++20"
	set_ios_config()
	set_apple_config()
	build_arch_configuration()

	filter { "system:linux or system:macosx or system:haiku or system:bsd", "action:not vs*" }
		if package_name ~= "eepp" and package_name ~= "eepp-static" then
			linkoptions { "-Wl,-rpath,'$$ORIGIN'" }
		end

	filter { "system:bsd" }
		if package_name ~= "eepp" and package_name ~= "eepp-static" then
			flags { "RelativeLinks" }
		end

	filter { "system:windows", "action:not vs*", "architecture:x86" }
		if true == use_ee_icon then
			linkoptions { _MAIN_SCRIPT_DIR .. "/bin/assets/icon/ee.res" }
		end

	filter { "system:windows", "action:not vs*", "architecture:x86_64" }
		if true == use_ee_icon then
			linkoptions { _MAIN_SCRIPT_DIR .. "/bin/assets/icon/ee.x64.res" }
		end

	filter { "system:windows", "action:vs*" }
		if true == use_ee_icon then
			files { "bin/assets/icon/ee.rc", "bin/assets/icon/ee.ico" }
			vpaths { ['Resources/*'] = { "ee.rc", "ee.ico" } }
		end

	filter "action:vs*"
		buildoptions{ "/std:c++20", "/utf-8", "/bigobj" }

	filter "action:not vs*"
		buildoptions { "-Wall" }

	filter { "configurations:debug*", "action:not vs*" }
		buildoptions{ "-Wno-long-long" }

	filter { "configurations:release*", "action:not vs*" }
		buildoptions { "-fno-strict-aliasing" }

	filter "configurations:debug*"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		targetname ( package_name .. "-debug" .. extension )

	filter "configurations:release*"
		defines { "NDEBUG" }
		targetname ( package_name .. extension )

	filter { "system:windows", "action:not vs*" }
		linkoptions { "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic" }

	filter { "system:windows", "action:vs*" }
		if table.contains( backends, "SDL2" ) then
			links { "SDL2", "SDL2main" }
		end

	filter { "options:windows-vc-build", "options:arch=arm64" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib" }

	filter { "options:windows-vc-build", "system:windows", "platforms:x86" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x86" }

	filter { "options:windows-vc-build", "system:windows", "platforms:x86_64", "not options:arch=arm64" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x64" }

	filter { "options:windows-mingw-build", "architecture:x86" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/i686-w64-mingw32/lib/", "/usr/i686-w64-mingw32/sys-root/mingw/lib/" }

	filter { "options:windows-mingw-build", "architecture:x86_64" }
		if _OPTIONS["arch"] ~= "arm64" then
			syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/lib/", "/usr/x86_64-w64-mingw32/sys-root/mingw/lib/" }
		end

	filter { "options:windows-mingw-build", "options:arch=arm64" }
		syslibdirs { remote_sdl2_arm64_cross_tools_path.. "/lib/" }

	filter "system:emscripten"
		targetname ( package_name .. extension )
		linkoptions { "-O3 -s TOTAL_MEMORY=536870912 -s ALLOW_MEMORY_GROWTH=1 -s USE_SDL=2" }
		buildoptions { "-O3 -s USE_SDL=2 -s PRECISE_F32=1 -s ENVIRONMENT=worker,web" }
		buildoptions { "-s USE_PTHREADS=1" }
		linkoptions { "-s USE_PTHREADS=1 -sPTHREAD_POOL_SIZE=8" }

		if _OPTIONS["with-gles1"] and ( not _OPTIONS["with-gles2"] or _OPTIONS["force-gles1"] ) then
			linkoptions{ "-s LEGACY_GL_EMULATION=1" }
		end

		if _OPTIONS["with-gles2"] and not _OPTIONS["force-gles1"] then
			linkoptions{ "-s FULL_ES2=1" }
		end

	filter { "action:export-compile-commands", "system:macosx" }
		buildoptions { "-std=c++20" }

	filter {}
end

function generate_os_links()
	if os.istarget("linux") then
		multiple_insert( os_links, { "rt", "pthread", "GL" } )

		if _OPTIONS["with-static-eepp"] then
			table.insert( os_links, "dl" )
		end
	elseif os.istarget("windows") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32", "uuid" } )
	elseif os.istarget("mingw32") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32", "uuid" } )
	elseif os.istarget("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "CoreFoundation.framework" } )
	elseif os.istarget("bsd") then
		multiple_insert( os_links, { "rt", "pthread", "GL" } )
	elseif os.istarget("haiku") then
		multiple_insert( os_links, { "GL", "network" } )
	elseif os.istarget("ios") then
		multiple_insert( os_links, { "OpenGLES.framework", "AudioToolbox.framework", "CoreAudio.framework", "Foundation.framework", "CoreFoundation.framework", "UIKit.framework", "QuartzCore.framework", "CoreGraphics.framework", "CoreMotion.framework", "AVFoundation.framework", "GameController.framework" } )
	elseif os.istarget("android") then
		multiple_insert( os_links, { "GLESv1_CM", "GLESv2", "log" } )
	end

	if _OPTIONS["without-mojoal"] then
		if os.istarget("linux") or os.istarget("bsd") or os.istarget("haiku") or os.istarget("emscripten") then
			multiple_insert( os_links, { "openal" } )
		elseif os.istarget("windows") or os.istarget("mingw32") then
			if os_ishost("msys") then
				multiple_insert( os_links, { "openal" } )
			else
				multiple_insert( os_links, { "OpenAL32" } )
			end
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

	if _OPTIONS["thread-sanitizer"] then
		buildoptions { "-fsanitize=thread" }
		linkoptions { "-fsanitize=thread" }
		if not os.istarget("macosx") then
			links { "tsan" }
		end
	end

	if _OPTIONS["address-sanitizer"] then
		buildoptions { "-fsanitize=address" }
		linkoptions { "-fsanitize=address" }
		if not os.istarget("macosx") then
			links { "asan" }
		end
	end

	if _OPTIONS["time-trace"] then
		buildoptions { "-ftime-trace" }
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

	if not _OPTIONS["without-text-shaper"] then
		links { "harfbuzz-static", "SheenBidi-static" }
		includedirs { "src/thirdparty/SheenBidi/Headers" }
		defines { "EE_TEXT_SHAPER_ENABLED" }
	end

	links { "SOIL2-static",
			"chipmunk-static",
			"libzip-static",
			"jpeg-compressor-static",
			"zlib-static",
			"imageresampler-static",
			"pugixml-static",
			"vorbis-static",
			"pcre2-8-static",
			"oniguruma-static",
			"libwebp-static",
			"libpng-static",
	}

	if not _OPTIONS["without-mojoal"] then
		links { "mojoal-static"}
	end

	if not _OPTIONS["with-openssl"] then
		links { "mbedtls-static" }
	end

	if not os.istarget("haiku") and not os.istarget("ios") and not os.istarget("android") and not os.istarget("emscripten") then
		links{ "glew-static" }
	end
end

function can_add_static_backend()
	if _OPTIONS["with-static-backend"] then
		return true
	end
	return false
end

function insert_static_backend( name )
	table.insert( static_backends, _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/lib" .. name .. ".a" )
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

function set_apple_config()
	if is_xcode() or _OPTIONS["use-frameworks"] then
		linkoptions { "-F /Library/Frameworks" }
		buildoptions { "-F /Library/Frameworks" }
		incdirs { "/Library/Frameworks/SDL2.framework/Headers" }
	end
	if os.istarget("macosx") then
		defines { "EE_SDL2_FROM_ROOTPATH" }
		if not is_xcode() and not _OPTIONS["use-frameworks"] then
			local sdl2flags = popen("sdl2-config --cflags"):gsub("\n", "")
			buildoptions { sdl2flags }
		end
	end
end

function set_ios_config()
	if os.istarget("ios") then
		-- local toolchainpath = os.getenv("TOOLCHAINPATH")
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
		local sysroot_ver = " -miphoneos-version-min=16.3 -isysroot " .. sysroot_path

		buildoptions { sysroot_ver .. " -I" .. sysroot_path .. "/usr/include" }
		linkoptions { sysroot_ver }
		libdirs { framework_libs_path }
		linkoptions { " -F" .. framework_path .. " -L" .. framework_libs_path .. " -isysroot " .. sysroot_path }
		incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
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

function eepp_module_maps_add()
	links { "eepp-maps-static" }
	defines { "EE_MAPS_STATIC" }
	incdirs { "src/modules/maps/include/", "src/modules/maps/src/" }
end

function eepp_module_physics_add()
	links { "eepp-physics-static", "chipmunk-static" }
	defines { "EE_PHYSICS_STATIC" }
	incdirs { "src/modules/physics/include/", "src/modules/physics/src/" }
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
			"src/eepp/ui/abstract/*.cpp",
			"src/eepp/ui/models/*.cpp",
			"src/eepp/ui/css/*.cpp",
			"src/eepp/ui/doc/*.cpp",
			"src/eepp/ui/doc/languages/*.cpp",
			"src/eepp/ui/tools/*.cpp",
			"src/eepp/physics/*.cpp",
			"src/eepp/physics/constraints/*.cpp"
	}

	incdirs {
		"include",
		"src",
		"src/thirdparty",
		"include/eepp/thirdparty",
		"src/thirdparty/freetype2/include",
		"src/thirdparty/zlib",
		"src/thirdparty/libogg/include",
		"src/thirdparty/libvorbis/include",
		"src/thirdparty/mbedtls/include",
		"src/thirdparty/pcre2/src",
		"src/thirdparty/oniguruma",
		"src/thirdparty/libwebp/src",
		"src/thirdparty/SheenBidi/Headers",
		"src/thirdparty/SheenBidi/Headers/SheenBidi",
	}

	add_static_links()
	check_ssl_support()

	defines { "PCRE2_STATIC", "PCRE2_CODE_UNIT_WIDTH=8", "ONIG_STATIC" }

	if table.contains( backends, "SDL2" ) then
		files { "src/eepp/window/backend/SDL2/*.cpp" }
		defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }
	end

	multiple_insert( link_list, os_links )

	links { link_list }

	build_link_configuration( build_name )

	if _OPTIONS["with-dynamic-freetype"] and not os.istarget("ios") and os_findlib("freetype") then
		table.insert( link_list, get_backend_link_name( "freetype" ) )
	end

	cppdialect "C++20"

	filter "options:use-frameworks"
		defines { "EE_USE_FRAMEWORKS" }

	filter { "system:macosx", "action:xcode* or options:use-frameworks" }
		libdirs { "/System/Library/Frameworks", "/Library/Frameworks", "src/thirdparty" }

	filter { "system:macosx", "not action:xcode*", "not options:use-frameworks" }
		libdirs { "/opt/homebrew/lib" }

	filter "system:windows"
		files { "src/eepp/system/platform/win/*.cpp" }
		files { "src/eepp/network/platform/win/*.cpp" }
		links { "bcrypt" }

	filter "system:not windows"
		files { "src/eepp/system/platform/posix/*.cpp" }
		files { "src/eepp/network/platform/unix/*.cpp" }

	filter "options:not without-mojoal"
		defines { "AL_LIBTYPE_STATIC", "EE_MOJOAL" }
		incdirs { "src/thirdparty/mojoAL" }

	filter "options:windows-vc-build"
		incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
		incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include/SDL2" }

	filter { "options:windows-mingw-build", "architecture:x86" }
		incdirs { "src/thirdparty/" .. remote_sdl2_version .."/i686-w64-mingw32/include/" }

	filter { "options:windows-mingw-build", "architecture:x86_64" }
		if _OPTIONS["arch"] ~= "arm64" then
			incdirs { "src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/include/" }
		end

	filter { "options:windows-mingw-build", "options:arch=arm64" }
		incdirs { remote_sdl2_arm64_cross_tools_path .. "/include/" }

	filter "action:vs*"
		incdirs { "src/thirdparty/libzip/vs" }
		buildoptions{ "/std:c++20", "/utf-8", "/bigobj" }

	filter { "action:export-compile-commands", "system:macosx" }
		buildoptions { "-std=c++20" }

	filter {}
end

function target_dir_lib(path)
	filter "architecture:x86"
		targetdir("libs/" .. os.target() .. "/x86/" .. path .. "/")
	filter "architecture:x86_64"
		targetdir("libs/" .. os.target() .. "/x86_64/" .. path .. "/")
	filter "architecture:ARM"
		targetdir("libs/" .. os.target() .. "/arm/" .. path .. "/")
	filter "architecture:ARM64"
		targetdir("libs/" .. os.target() .. "/arm64/" .. path .. "/")
	filter "architecture:universal"
		targetdir("libs/" .. os.target() .. "/universal/" .. path .. "/")
	filter {}
end

function target_dir_thirdparty()
	target_dir_lib("thirdparty")
end

function postsymlinklib_arch(name)
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/x86/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:x86" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/x86_64/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:x86_64" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:ARM" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm64/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:ARM64" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm64/", _MAIN_SCRIPT_DIR .. "/bin/", name, "options:arch=arm64" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/universal/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:universal" )
	if name == "eepp" then
		postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/x86/", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/", name, "architecture:x86" )
		postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/x86_64/", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/", name, "architecture:x86_64" )
		postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm/", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/", name, "architecture:ARM" )
		postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm64/", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/", name, "architecture:ARM64" )
		postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm64/", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/", name, "options:arch=arm64" )
		postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/universal/", _MAIN_SCRIPT_DIR .. "/bin/unit_tests/", name, "architecture:universal" )
	end
end

workspace "eepp"
	targetdir(_MAIN_SCRIPT_DIR .. "/bin/")

	if _ACTION == "ninja" then
		configurations { "debug", "release" }
		platforms { get_architecture() }
	else
		configurations { "debug", "release" }
		platforms { "x86_64", "x86", "arm64" }
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

	filter "platforms:arm64"
		architecture "ARM64"

	filter { "platforms:arm64", "system:macosx" }
		architecture "ARM64"
		buildoptions { "-arch arm64" }
		linkoptions { "-arch arm64" }

	filter "platforms:x86_64"
		architecture "x86_64"

	filter { "platforms:x86_64", "system:macosx" }
		architecture "x86_64"
		buildoptions { "-arch x86_64" }
		linkoptions { "-arch x86_64" }

	filter "system:macosx"
		defines { "GL_SILENCE_DEPRECATION" }

	filter "system:ios"
		defines { "GLES_SILENCE_DEPRECATION" }

	filter "configurations:debug*"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release*"
		optimize "Speed"

	filter { "configurations:release*", "options:with-debug-symbols" }
		symbols "On"

	filter { "system:windows", "action:vs*" }
		flags { "MultiProcessorCompile" }
		disablewarnings{ "4305", "4146", "4996", "4244", "4267" }

	filter "system:bsd"
		syslibdirs { "/usr/local/lib" }
		externalincludedirs { "/usr/local/include" }

	filter {}

	project "SOIL2-static"
		kind "StaticLib"
		language "C"
		files { "src/thirdparty/SOIL2/src/SOIL2/*.c" }
		incdirs { "src/thirdparty/SOIL2" }
		build_base_configuration( "SOIL2" )
		target_dir_thirdparty()

	project "glew-static"
		kind "StaticLib"
		language "C"
		defines { "GLEW_NO_GLU", "GLEW_STATIC" }
		files { "src/thirdparty/glew/*.c" }
		incdirs { "include/thirdparty/glew" }
		build_base_configuration( "glew" )
		target_dir_thirdparty()
		filter { "action:vs*", "options:arch=arm64" }
			buildoptions{ "/bigobj", "/O1", "/Zm200" }

	project "mbedtls-static"
		kind "StaticLib"
		language "C"
		incdirs { "src/thirdparty/mbedtls/include/" }
		files { "src/thirdparty/mbedtls/library/*.c" }
		build_base_configuration( "mbedtls" )
		target_dir_thirdparty()

	project "vorbis-static"
		kind "StaticLib"
		language "C"
		incdirs { "src/thirdparty/libvorbis/lib/", "src/thirdparty/libogg/include", "src/thirdparty/libvorbis/include" }
		files { "src/thirdparty/libogg/**.c", "src/thirdparty/libvorbis/**.c" }
		build_base_cpp_configuration( "vorbis" )
		target_dir_thirdparty()

	project "pugixml-static"
		kind "StaticLib"
		language "C++"
		files { "src/thirdparty/pugixml/*.cpp" }
		build_base_cpp_configuration( "pugixml" )
		target_dir_thirdparty()

	project "zlib-static"
		kind "StaticLib"
		language "C"
		defines { "_LARGEFILE64_SOURCE" }
		files { "src/thirdparty/zlib/*.c" }
		build_base_configuration( "zlib" )
		target_dir_thirdparty()

	project "libzip-static"
		kind "StaticLib"
		language "C"
		files { "src/thirdparty/libzip/*.c" }
		incdirs { "src/thirdparty/zlib" }
		build_base_configuration( "libzip" )
		target_dir_thirdparty()

	project "libpng-static"
		kind "StaticLib"
		language "C"
		files { "src/thirdparty/libpng/**.c" }
		incdirs { "src/thirdparty/libpng/include" }
		build_base_configuration( "libpng" )
		target_dir_thirdparty()

	project "libwebp-static"
		kind "StaticLib"
		language "C"
		files { "src/thirdparty/libwebp/**.c" }
		incdirs { "src/thirdparty/libwebp" }
		build_base_configuration( "libwebp" )
		target_dir_thirdparty()

	project "freetype-static"
		kind "StaticLib"
		language "C"
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/thirdparty/freetype2/src/**.c" }
		incdirs { "src/thirdparty/freetype2/include", "src/thirdparty/libpng" }
		build_base_configuration( "freetype" )
		target_dir_thirdparty()

	project "pcre2-8-static"
		kind "StaticLib"
		language "C"
		defines { "HAVE_CONFIG_H", "PCRE2_STATIC", "PCRE2_CODE_UNIT_WIDTH=8" }
		files {
			'src/thirdparty/pcre2/src/pcre2_auto_possess.c',
			'src/thirdparty/pcre2/src/pcre2_chartables.c',
			'src/thirdparty/pcre2/src/pcre2_chkdint.c',
			'src/thirdparty/pcre2/src/pcre2_compile.c',
			'src/thirdparty/pcre2/src/pcre2_config.c',
			'src/thirdparty/pcre2/src/pcre2_context.c',
			'src/thirdparty/pcre2/src/pcre2_convert.c',
			'src/thirdparty/pcre2/src/pcre2_dfa_match.c',
			'src/thirdparty/pcre2/src/pcre2_error.c',
			'src/thirdparty/pcre2/src/pcre2_extuni.c',
			'src/thirdparty/pcre2/src/pcre2_find_bracket.c',
			'src/thirdparty/pcre2/src/pcre2_maketables.c',
			'src/thirdparty/pcre2/src/pcre2_match.c',
			'src/thirdparty/pcre2/src/pcre2_match_data.c',
			'src/thirdparty/pcre2/src/pcre2_newline.c',
			'src/thirdparty/pcre2/src/pcre2_ord2utf.c',
			'src/thirdparty/pcre2/src/pcre2_pattern_info.c',
			'src/thirdparty/pcre2/src/pcre2_script_run.c',
			'src/thirdparty/pcre2/src/pcre2_serialize.c',
			'src/thirdparty/pcre2/src/pcre2_string_utils.c',
			'src/thirdparty/pcre2/src/pcre2_study.c',
			'src/thirdparty/pcre2/src/pcre2_substitute.c',
			'src/thirdparty/pcre2/src/pcre2_substring.c',
			'src/thirdparty/pcre2/src/pcre2_tables.c',
			'src/thirdparty/pcre2/src/pcre2_ucd.c',
			'src/thirdparty/pcre2/src/pcre2_valid_utf.c',
			'src/thirdparty/pcre2/src/pcre2_xclass.c',
		}
		if not os.istarget("emscripten") and not os.istarget("ios") then
			files { 'src/thirdparty/pcre2/src/pcre2_jit_compile.c' }
		end
		incdirs { "src/thirdparty/pcre2/src" }
		build_base_configuration( "pcre2-8" )
		target_dir_thirdparty()

	project "oniguruma-static"
		kind "StaticLib"
		language "C"
		target_dir_thirdparty()
		files {
			'src/thirdparty/oniguruma/regcomp.c',
			'src/thirdparty/oniguruma/regenc.c',
			'src/thirdparty/oniguruma/regerror.c',
			'src/thirdparty/oniguruma/regext.c',
			'src/thirdparty/oniguruma/regexec.c',
			'src/thirdparty/oniguruma/regparse.c',
			'src/thirdparty/oniguruma/regsyntax.c',
			'src/thirdparty/oniguruma/regtrav.c',
			'src/thirdparty/oniguruma/regversion.c',
			'src/thirdparty/oniguruma/st.c',
			'src/thirdparty/oniguruma/reggnu.c',
			'src/thirdparty/oniguruma/regposerr.c',
			'src/thirdparty/oniguruma/regposix.c',
			'src/thirdparty/oniguruma/mktable.c',
			'src/thirdparty/oniguruma/ascii.c',
			'src/thirdparty/oniguruma/euc_jp.c',
			'src/thirdparty/oniguruma/euc_tw.c',
			'src/thirdparty/oniguruma/euc_kr.c',
			'src/thirdparty/oniguruma/sjis.c',
			'src/thirdparty/oniguruma/big5.c',
			'src/thirdparty/oniguruma/gb18030.c',
			'src/thirdparty/oniguruma/koi8.c',
			'src/thirdparty/oniguruma/koi8_r.c',
			'src/thirdparty/oniguruma/cp1251.c',
			'src/thirdparty/oniguruma/iso8859_1.c',
			'src/thirdparty/oniguruma/iso8859_2.c',
			'src/thirdparty/oniguruma/iso8859_3.c',
			'src/thirdparty/oniguruma/iso8859_4.c',
			'src/thirdparty/oniguruma/iso8859_5.c',
			'src/thirdparty/oniguruma/iso8859_6.c',
			'src/thirdparty/oniguruma/iso8859_7.c',
			'src/thirdparty/oniguruma/iso8859_8.c',
			'src/thirdparty/oniguruma/iso8859_9.c',
			'src/thirdparty/oniguruma/iso8859_10.c',
			'src/thirdparty/oniguruma/iso8859_11.c',
			'src/thirdparty/oniguruma/iso8859_13.c',
			'src/thirdparty/oniguruma/iso8859_14.c',
			'src/thirdparty/oniguruma/iso8859_15.c',
			'src/thirdparty/oniguruma/iso8859_16.c',
			'src/thirdparty/oniguruma/utf8.c',
			'src/thirdparty/oniguruma/utf16_be.c',
			'src/thirdparty/oniguruma/utf16_le.c',
			'src/thirdparty/oniguruma/utf32_be.c',
			'src/thirdparty/oniguruma/utf32_le.c',
			'src/thirdparty/oniguruma/unicode.c',
			'src/thirdparty/oniguruma/unicode_fold_data.c',
			'src/thirdparty/oniguruma/unicode_fold1_key.c',
			'src/thirdparty/oniguruma/unicode_fold2_key.c',
			'src/thirdparty/oniguruma/unicode_fold3_key.c',
			'src/thirdparty/oniguruma/onig_init.c',
			'src/thirdparty/oniguruma/unicode_unfold_key.c',
		}
		defines { "ONIG_STATIC" }
		incdirs { "src/thirdparty/oniguruma" }
		build_base_configuration( "oniguruma" )

	if not _OPTIONS["without-text-shaper"] then
		project "harfbuzz-static"
			kind "StaticLib"
			language "C++"
			defines { "HAVE_CONFIG_H" }
			files { "src/thirdparty/harfbuzz/**.cc" }
			incdirs { "src/thirdparty/freetype2/include", "src/thirdparty/harfbuzz" }
			build_base_cpp_configuration( "harfbuzz" )
			target_dir_thirdparty()
			filter "action:vs*"
				buildoptions{ "/bigobj" }

		project "SheenBidi-static"
			kind "StaticLib"
			language "C"
			files { "src/thirdparty/SheenBidi/Source/**.c" }
			incdirs { "src/thirdparty/SheenBidi/Headers" }
			target_dir_thirdparty()
			build_base_configuration( "sheenbidi" )
	end

	project "chipmunk-static"
		kind "StaticLib"
		files { "src/thirdparty/chipmunk/*.c", "src/thirdparty/chipmunk/constraints/*.c" }
		incdirs { "src/modules/physics/include/eepp/thirdparty/chipmunk" }
		build_base_configuration( "chipmunk" )
		target_dir_thirdparty()
		filter "action:vs*"
			language "C++"
			buildoptions { "/TP" }
		filter "action:not vs*"
			language "C"

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		files { "src/thirdparty/jpeg-compressor/*.cpp" }
		build_base_cpp_configuration( "jpeg-compressor" )
		target_dir_thirdparty()

	project "imageresampler-static"
		kind "StaticLib"
		language "C++"
		files { "src/thirdparty/imageresampler/*.cpp" }
		build_base_cpp_configuration( "imageresampler" )
		target_dir_thirdparty()

	project "mojoal-static"
		kind "StaticLib"
		language "C"
		incdirs { "include/eepp/thirdparty/mojoAL" }
		defines { "AL_LIBTYPE_STATIC", "EE_MOJOAL" }
		files { "src/thirdparty/mojoAL/*.c" }
		build_base_cpp_configuration( "mojoal" )
		target_dir_thirdparty()
		filter "options:windows-vc-build"
			incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
			incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include/SDL2" }
		filter { "options:windows-mingw-build", "architecture:x86" }
				incdirs { "src/thirdparty/" .. remote_sdl2_version .."/i686-w64-mingw32/include/" }
		filter { "options:windows-mingw-build", "architecture:x86_64" }
			if _OPTIONS["arch"] ~= "arm64" then
				incdirs { "src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/include/" }
			end
		filter { "options:windows-mingw-build", "options:arch=arm64" }
			incdirs { remote_sdl2_arm64_cross_tools_path .."/include/" }

	project "efsw-static"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty/efsw/src" }
		files { "src/thirdparty/efsw/src/efsw/*.cpp" }
		build_base_cpp_configuration( "efsw" )
		target_dir_thirdparty()
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

	project "eepp-maps-static"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "include", "src/modules/maps/include/","src/modules/maps/src/" }
		files { "src/modules/maps/src/**.cpp" }
		defines { "EE_MAPS_STATIC" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		build_base_cpp_configuration( "eepp-maps-static" )
		target_dir_lib("")
		filter "action:not vs*"
			buildoptions { "-Wall" }

	project "eepp-maps"
		kind "SharedLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "include", "src/modules/maps/include/","src/modules/maps/src/" }
		files { "src/modules/maps/src/**.cpp" }
		links { "eepp-shared" }
		defines { "EE_MAPS_EXPORTS" }
		build_base_cpp_configuration( "eepp-maps" )
		postsymlinklib_arch( "eepp-maps" )
		target_dir_lib("")
		filter "action:not vs*"
			buildoptions { "-Wall" }

	project "eepp-physics-static"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "include", "src/modules/physics/include/","src/modules/physics/src/" }
		files { "src/modules/physics/src/**.cpp", "src/eepp/physics/constraints/*.cpp" }
		defines { "EE_PHYSICS_STATIC" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		build_base_cpp_configuration( "eepp-physics-static" )
		target_dir_lib("")
		filter "action:not vs*"
			buildoptions { "-Wall" }

	project "eepp-physics"
		kind "SharedLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "include", "src/modules/physics/include/","src/modules/physics/src/" }
		files { "src/modules/physics/src/**.cpp", "src/eepp/physics/constraints/*.cpp" }
		links { "chipmunk-static", "eepp-shared" }
		defines { "EE_PHYSICS_EXPORTS" }
		build_base_cpp_configuration( "eepp-physics" )
		postsymlinklib_arch( "eepp-physics" )
		target_dir_lib("")
		filter "action:not vs*"
			buildoptions { "-Wall" }

	project "eterm-static"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "include", "src/modules/eterm/include/","src/modules/eterm/src/" }
		files { "src/modules/eterm/src/**.cpp" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		if os_ishost("msys") then
			defines { "WINVER=0x0602" }
		end
		build_base_cpp_configuration( "eterm" )
		target_dir_lib("")
		filter "action:not vs*"
			buildoptions { "-Wall" }
		filter { "action:export-compile-commands", "system:macosx" }
			buildoptions { "-std=c++20" }

	project "languages-syntax-highlighting-static"
		kind "StaticLib"
		language "C++"
		cppdialect "C++20"
		incdirs { "include", "src/modules/languages-syntax-highlighting/src" }
		files { "src/modules/languages-syntax-highlighting/src/**.cpp" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		build_base_cpp_configuration( "languages-syntax-highlighting" )
		target_dir_lib("")
		filter "action:not vs*"
			buildoptions { "-Wall" }
		filter { "action:export-compile-commands", "system:macosx" }
			buildoptions { "-std=c++20" }

	-- Library
	if not _OPTIONS["disable-static-build"] then
	project "eepp-static"
		kind "StaticLib"
		language "C++"
		build_eepp( "eepp-static" )
		target_dir_lib("")
	end

	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		build_eepp( "eepp" )
		postsymlinklib_arch( "eepp" )
		target_dir_lib("")

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

	project "eepp-physics-demo"
		set_kind()
		language "C++"
		files { "src/examples/physics/*.cpp" }
		eepp_module_physics_add()
		build_link_configuration( "eepp-physics-demo", true )

	project "eepp-http-request"
		kind "ConsoleApp"
		language "C++"
		files { "src/examples/http_request/*.cpp" }
		incdirs { "src/thirdparty" }
		build_link_configuration( "eepp-http-request", true )

	project "eepp-ui-custom-widget"
		set_kind()
		language "C++"
		files { "src/examples/ui_custom_widget/*.cpp" }
		build_link_configuration( "eepp-ui-custom-widget", true )

	project "eepp-ui-hello-world"
		set_kind()
		language "C++"
		files { "src/examples/ui_hello_world/*.cpp" }
		build_link_configuration( "eepp-ui-hello-world", true )

	project "eepp-7guis-counter"
		set_kind()
		language "C++"
		files { "src/examples/7guis/counter/*.cpp" }
		build_link_configuration( "eepp-7guis-counter", true )

	project "eepp-7guis-temperature-converter"
		set_kind()
		language "C++"
		files { "src/examples/7guis/temperature_converter/*.cpp" }
		build_link_configuration( "eepp-7guis-temperature-converter", true )

	project "eepp-7guis-flight-booker"
		set_kind()
		language "C++"
		files { "src/examples/7guis/flight_booker/*.cpp" }
		build_link_configuration( "eepp-7guis-flight-booker", true )

	project "eepp-7guis-timer"
		set_kind()
		language "C++"
		files { "src/examples/7guis/timer/*.cpp" }
		build_link_configuration( "eepp-7guis-timer", true )

	project "eepp-7guis-crud"
		set_kind()
		language "C++"
		files { "src/examples/7guis/crud/*.cpp" }
		build_link_configuration( "eepp-7guis-crud", true )

	project "eepp-7guis-circle-drawer"
		set_kind()
		language "C++"
		files { "src/examples/7guis/circle_drawer/*.cpp" }
		build_link_configuration( "eepp-7guis-circle-drawer", true )

	project "eepp-7guis-cells"
		set_kind()
		language "C++"
		files { "src/examples/7guis/cells/*.cpp" }
		build_link_configuration( "eepp-7guis-cells", true )

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
		eepp_module_maps_add()
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

	if os.istarget("macosx") then
	project "ecode-macos-helper-static"
		kind "StaticLib"
		language "C++"
		target_dir_thirdparty()
		filter "system:macosx"
			files { "src/tools/ecode/macos/*.m" }
		filter { "configurations:debug*", "action:not vs*" }
			defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
			symbols "On"
			buildoptions{ "-Wall" }
			targetname ( "ecode-macos-helper-static-debug" )
		filter { "configurations:release*", "action:not vs*" }
			defines { "NDEBUG" }
			optimize "Speed"
			targetname ( "ecode-macos-helper-static" )
		filter { "configurations:release*", "action:not vs*", "options:with-debug-symbols" }
			symbols "On"
	end

	project "ecode"
		set_kind()
		language "C++"
		files { "src/tools/ecode/**.cpp" }
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty", "src/modules/eterm/include/", "src/modules/languages-syntax-highlighting/src" }
		links { "efsw-static", "eterm-static", "languages-syntax-highlighting-static" }
		build_link_configuration( "ecode", false )
		filter { "system:windows", "action:not vs*" }
			buildoptions{ "-Wa,-mbig-obj" }
		filter { "system:windows", "action:vs*" }
			files { "bin/assets/icon/ecode.rc", "bin/assets/icon/ecode.ico" }
			vpaths { ['Resources/*'] = { "ecode.rc", "ecode.ico" } }
		filter { "system:windows", "action:not vs*", "architecture:x86" }
			linkoptions { _MAIN_SCRIPT_DIR .. "/bin/assets/icon/ecode.res" }
		filter { "system:windows", "action:not vs*", "architecture:x86_64" }
			linkoptions { _MAIN_SCRIPT_DIR .. "/bin/assets/icon/ecode.x64.res" }
		filter "system:macosx"
			links { "CoreFoundation.framework", "CoreServices.framework", "Cocoa.framework" }
			links { "ecode-macos-helper-static" }
		filter { "system:not windows", "system:not haiku" }
			links { "pthread" }
		filter "system:linux"
			links { "util" }
			if os_findlib("dw") then
				links { "dw" }
				defines { "ECODE_HAS_DW" }
			end
		filter { "system:linux or system:macosx or system:haiku or system:bsd", "configurations:release*" }
			buildoptions { "-g1", "-fvisibility=default" }
		filter { "system:linux or system:bsd", "configurations:release*" }
			linkoptions { "-rdynamic" }
		filter { "system:windows" }
			links { "dbghelp", "psapi" }
		filter "system:haiku"
			links { "bsd", "network" }
		filter "system:bsd"
			links { "util" }
		filter { "system:windows", "action:not vs*", "configurations:release*" }
			buildoptions { "-g1" }

	project "eterm"
		set_kind()
		language "C++"
		incdirs { "src/modules/eterm/include/", "src/thirdparty" }
		files { "src/tools/eterm/**.cpp" }
		links { "eterm-static" }
		build_link_configuration( "eterm", false )
		filter { "system:windows", "action:vs*" }
			files { "bin/assets/icon/eterm.rc", "bin/assets/icon/eterm.ico" }
			vpaths { ['Resources/*'] = { "eterm.rc", "eterm.ico" } }
		filter { "system:windows", "action:not vs*", "architecture:x86" }
			linkoptions { _MAIN_SCRIPT_DIR .. "/bin/assets/icon/eterm.res" }
		filter { "system:windows", "action:not vs*", "architecture:x86_64" }
			linkoptions { _MAIN_SCRIPT_DIR .. "/bin/assets/icon/eterm.x64.res" }
		filter "system:linux or system:bsd"
			links { "util" }
		filter "system:haiku"
			links { "bsd" }

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
		eepp_module_maps_add()
		eepp_module_physics_add()
		build_link_configuration( "eepp-test", true )

	project "eepp-ui-perf-test"
		set_kind()
		language "C++"
		files { "src/tests/ui_perf_test/*.cpp" }
		incdirs { "src/thirdparty" }
		build_link_configuration( "eepp-ui-perf-test", true )

	project "eepp-unit_tests"
		kind "ConsoleApp"
		targetdir(_MAIN_SCRIPT_DIR .. "/bin/unit_tests")
		language "C++"
		files { "src/tests/unit_tests/*.cpp" }
		build_link_configuration( "eepp-unit_tests", true )

if os.isfile("external_projects.lua") then
	dofile("external_projects.lua")
end

if os.isfile("external_projects_premake5.lua") then
	dofile("external_projects_premake5.lua")
end
