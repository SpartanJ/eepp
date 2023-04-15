newoption { trigger = "with-openssl", description = "Enables OpenSSL support ( and disables mbedtls backend )." }
newoption { trigger = "with-dynamic-freetype", description = "Dynamic link against freetype." }
newoption { trigger = "with-static-eepp", description = "Force to build the demos and tests with eepp compiled statically" }
newoption { trigger = "with-static-backend", description = "It will try to compile the library with a static backend (only for gcc and mingw).\n\t\t\t\tThe backend should be placed in libs/your_platform/libYourBackend.a" }
newoption { trigger = "with-gles2", description = "Compile with GLES2 support" }
newoption { trigger = "with-gles1", description = "Compile with GLES1 support" }
newoption { trigger = "with-mojoal", description = "Compile with mojoAL as OpenAL implementation instead of using openal-soft (requires SDL2 backend)" }
newoption { trigger = "use-frameworks", description = "In macOS it will try to link the external libraries from its frameworks. For example, instead of linking against SDL2 it will link against SDL2.framework." }
newoption { trigger = "windows-vc-build", description = "This is used to build the framework in Visual Studio downloading its external dependencies and making them available to the VS project without having to install them manually." }
newoption { trigger = "windows-mingw-build", description = "This is used to build the framework with mingw downloading its external dependencies." }
newoption { trigger = "with-emscripten-pthreads", description = "Enables emscripten build to use posix threads" }
newoption { trigger = "with-mold-linker", description = "Tries to use the mold linker instead of the default linker of the toolchain" }
newoption { trigger = "with-debug-symbols", description = "Release builds are built with debug symbols." }
newoption { trigger = "thread-sanitizer", description ="Compile with ThreadSanitizer." }
newoption {
	trigger = "with-backend",
	description = "Select the backend to use for window and input handling.\n\t\t\tIf no backend is selected or if the selected is not installed the script will search for a backend present in the system, and will use it.",
	allowed = {
		{ "SDL2",  "SDL2" },
	}
}

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
	if os.getenv("MSYSTEM") ~= "" then
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
remote_sdl2_version = "SDL2-2.26.2"
remote_sdl2_devel_src_url = "https://libsdl.org/release/SDL2-2.26.2.zip"
remote_sdl2_devel_vc_url = "https://www.libsdl.org/release/SDL2-devel-2.26.2-VC.zip"
remote_sdl2_devel_mingw_url = "https://www.libsdl.org/release/SDL2-devel-2.26.2-mingw.zip"

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
	if not os.isdir("src/thirdparty/" .. remote_sdl2_version) then
		if _OPTIONS["windows-vc-build"] then
			download_and_extract_sdl(remote_sdl2_devel_vc_url)
		elseif _OPTIONS["windows-mingw-build"] then
			download_and_extract_sdl(remote_sdl2_devel_mingw_url)
		elseif os.istarget("ios") then
			download_and_extract_sdl(remote_sdl2_devel_src_url)
		end
	end
end

function build_arch_configuration()
	filter {"architecture:x86", "options:cc=mingw"}
		buildoptions { "-D__USE_MINGW_ANSI_STDIO=1 -B /usr/bin/i686-w64-mingw32-" }

	filter {"architecture:x86_64", "options:cc=mingw"}
		buildoptions { "-D__USE_MINGW_ANSI_STDIO=1 -B /usr/bin/x86_64-w64-mingw32-" }
end

function build_base_configuration( package_name )
	incdirs { "src/thirdparty/zlib" }

	set_ios_config()
	set_xcode_config()
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
		if _OPTIONS["with-emscripten-pthreads"] then
			buildoptions { "-s USE_PTHREADS=1" }
		end
end

function build_base_cpp_configuration( package_name )
	if not os.istarget("windows") then
		buildoptions{ "-fPIC" }
	end

	set_ios_config()
	set_xcode_config()
	build_arch_configuration()

	if _OPTIONS["with-static-eepp"] then
		defines { "EE_STATIC" }
	end

	filter "action:vs*"
		buildoptions { "/utf-8" }

	filter "action:not vs*"
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
		if _OPTIONS["with-emscripten-pthreads"] then
			buildoptions { "-s USE_PTHREADS=1" }
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

	cppdialect "C++17"
	set_ios_config()
	set_xcode_config()
	build_arch_configuration()

	filter { "system:linux or system:macosx or system:haiku or system:bsd", "action:not vs*" }
		if package_name ~= "eepp" and package_name ~= "eepp-static" then
			linkoptions { "-Wl,-rpath,'$$ORIGIN'" }
		end

	filter { "system:windows", "action:not vs*", "architecture:x86" }
		if true == use_ee_icon then
			linkoptions { "../../bin/assets/icon/ee.res" }
		end

	filter { "system:windows", "action:not vs*", "architecture:x86_64" }
		if true == use_ee_icon then
			linkoptions { "../../bin/assets/icon/ee.x64.res" }
		end

	filter { "system:windows", "action:vs*" }
		if true == use_ee_icon then
			files { "bin/assets/icon/ee.rc", "bin/assets/icon/ee.ico" }
			vpaths { ['Resources/*'] = { "ee.rc", "ee.ico" } }
		end

	filter "action:vs*"
		buildoptions { "/utf-8" }

	filter "action:not vs*"
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
		defines { "NDEBUG" }
		targetname ( package_name .. extension )

	filter { "system:windows", "action:not vs*" }
		linkoptions { "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic" }

	filter { "system:windows", "action:vs*" }
		if table.contains( backends, "SDL2" ) then
			links { "SDL2", "SDL2main" }
		end

	filter { "options:windows-vc-build", "system:windows", "platforms:x86" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x86" }

	filter { "options:windows-vc-build", "system:windows", "platforms:x86_64" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/lib/x64" }

	filter { "options:windows-mingw-build", "architecture:x86", "options:cc=mingw" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/i686-w64-mingw32/lib/", "/usr/i686-w64-mingw32/sys-root/mingw/lib/" }

	filter { "options:windows-mingw-build", "architecture:x86_64", "options:cc=mingw" }
		syslibdirs { "src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/lib/", "/usr/x86_64-w64-mingw32/sys-root/mingw/lib/" }

	filter "system:emscripten"
		targetname ( package_name .. extension )
		linkoptions { "-O3 -s TOTAL_MEMORY=67108864" }
		linkoptions { "-s USE_SDL=2" }
		buildoptions { "-O3 -s USE_SDL=2 -s PRECISE_F32=1 -s ENVIRONMENT=worker,web" }

		if _OPTIONS["with-emscripten-pthreads"] then
			buildoptions { "-s USE_PTHREADS=1" }
			linkoptions { "-s USE_PTHREADS=1" }
		end

		if _OPTIONS["with-gles1"] and ( not _OPTIONS["with-gles2"] or _OPTIONS["force-gles1"] ) then
			linkoptions{ "-s LEGACY_GL_EMULATION=1" }
		end

		if _OPTIONS["with-gles2"] and not _OPTIONS["force-gles1"] then
			linkoptions{ "-s FULL_ES2=1" }
		end
end

function generate_os_links()
	if os.istarget("linux") then
		multiple_insert( os_links, { "rt", "pthread", "GL" } )

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
		multiple_insert( os_links, { "rt", "pthread", "GL" } )
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
		links { "freetype-static", "libpng-static" }
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

function can_add_static_backend()
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
		buildoptions { "-F /Library/Frameworks" }
		incdirs { "/Library/Frameworks/SDL2.framework/Headers" }
		defines { "EE_SDL2_FROM_ROOTPATH" }
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

	if _OPTIONS["with-dynamic-freetype"] and not os.istarget("ios") and os_findlib("freetype") then
		table.insert( link_list, get_backend_link_name( "freetype" ) )
	end

	cppdialect "C++17"

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

	filter { "options:windows-mingw-build", "architecture:x86", "options:cc=mingw" }
		incdirs { "src/thirdparty/" .. remote_sdl2_version .."/i686-w64-mingw32/include/" }

	filter { "options:windows-mingw-build", "architecture:x86_64", "options:cc=mingw" }
		incdirs { "src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/include/" }

	filter "action:vs*"
		incdirs { "src/thirdparty/libzip/vs" }
		buildoptions { "/bigobj" }
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
end

function target_dir_thirdparty()
	target_dir_lib("thirdparty")
end

function postsymlinklib_arch(name)
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/x86/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:x86" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/x86_64/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:x86_64" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:ARM" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/arm64/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:ARM64" )
	postsymlinklib( _MAIN_SCRIPT_DIR .. "/libs/" .. os.target() .. "/universal/", _MAIN_SCRIPT_DIR .. "/bin/", name, "architecture:universal" )
end

workspace "eepp"
	targetdir("./bin/")
	if os.istarget("ios") then
		configurations { "debug-x64", "debug-arm64", "release-x64", "release-arm64" }
		platforms { "x86_64", "arm64" }
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
		buildoptions { "-arch arm64" }
		linkoptions { "-arch arm64" }

	filter "platforms:x86_64"
		architecture "x86_64"

	filter "system:macosx"
		defines { "GL_SILENCE_DEPRECATION" }

	filter "configurations:debug*"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:release*"
		optimize "Speed"

	filter { "configurations:release*", "options:with-debug-symbols" }
		symbols "On"

	filter { "system:windows", "action:vs*" }
		flags { "MultiProcessorCompile" }

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

	project "mbedtls-static"
		kind "StaticLib"
		language "C"
		incdirs { "src/thirdparty/mbedtls/include/" }
		files { "src/thirdparty/mbedtls/library/*.c" }
		build_base_cpp_configuration( "mbedtls" )
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

	project "freetype-static"
		kind "StaticLib"
		language "C"
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/thirdparty/freetype2/src/**.c" }
		incdirs { "src/thirdparty/freetype2/include", "src/thirdparty/libpng" }
		build_base_configuration( "freetype" )
		target_dir_thirdparty()

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
		defines( "AL_LIBTYPE_STATIC" )
		files { "src/thirdparty/mojoAL/*.c" }
		build_base_cpp_configuration( "mojoal" )
		target_dir_thirdparty()
		filter "options:windows-vc-build"
			incdirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
		filter { "options:windows-mingw-build", "architecture:x86", "options:cc=mingw" }
			incdirs { "src/thirdparty/" .. remote_sdl2_version .."/i686-w64-mingw32/include/" }
		filter { "options:windows-mingw-build", "architecture:x86_64", "options:cc=mingw" }
			incdirs { "src/thirdparty/" .. remote_sdl2_version .."/x86_64-w64-mingw32/include/" }

	project "efsw-static"
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
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
		cppdialect "C++17"
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
		cppdialect "C++17"
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
		cppdialect "C++17"
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
		cppdialect "C++17"
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
		cppdialect "C++17"
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

	-- Library
	project "eepp-static"
		kind "StaticLib"
		language "C++"
		build_eepp( "eepp-static" )
		target_dir_lib("")

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
		eepp_module_maps_add()
		build_link_configuration( "eepp-MapEditor", true )

	project "eepp-uieditor"
		set_kind()
		language "C++"
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty" }
		links { "efsw-static", "pugixml-static" }
		files { "src/tools/uieditor/*.cpp" }
		eepp_module_maps_add()
		build_link_configuration( "eepp-UIEditor", true )
		filter "system:macosx"
			links { "CoreFoundation.framework", "CoreServices.framework" }
		filter { "system:not windows", "system:not haiku" }
			links { "pthread" }

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

	project "ecode"
		set_kind()
		language "C++"
		files { "src/tools/ecode/**.cpp" }
		incdirs { "src/thirdparty/efsw/include", "src/thirdparty", "src/modules/eterm/include/" }
		links { "efsw-static", "eterm-static" }
		build_link_configuration( "ecode", false )
		filter { "system:windows", "action:vs*" }
			files { "bin/assets/icon/ecode.rc", "bin/assets/icon/ecode.ico" }
			vpaths { ['Resources/*'] = { "ecode.rc", "ecode.ico" } }
		filter { "system:windows", "action:not vs*", "architecture:x86" }
			linkoptions { "../../bin/assets/icon/ecode.res" }
		filter { "system:windows", "action:not vs*", "architecture:x86_64" }
			linkoptions { "../../bin/assets/icon/ecode.x64.res" }
		filter "options:with-debug-symbols"
			defines { "ECODE_USE_BACKWARD" }
		filter "system:macosx"
			links { "CoreFoundation.framework", "CoreServices.framework", "Cocoa.framework" }
			links { "ecode-macos-helper-static" }
		filter { "system:not windows", "system:not haiku" }
			links { "pthread" }
		filter "system:linux"
			links { "util", "stdc++fs" }
		filter { "system:windows", "options:with-debug-symbols" }
			links { "dbghelp", "psapi" }
		filter { "system:windows", "options:with-debug-symbols", "options:cc=mingw" }
			links { "msvcr90" }
		filter { "system:linux", "options:with-debug-symbols" }
			links { "bfd", "dw", "dl" }
		filter "system:haiku"
			links { "bsd" }

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
			linkoptions { "../../bin/assets/icon/eterm.res" }
		filter { "system:windows", "action:not vs*", "architecture:x86_64" }
			linkoptions { "../../bin/assets/icon/eterm.x64.res" }
		filter "system:linux"
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
		includedirs { "src/thirdparty" }
		build_link_configuration( "eepp-ui-perf-test", true )

if os.isfile("external_projects.lua") then
	dofile("external_projects.lua")
end
