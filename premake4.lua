
function newplatform(plf)
	local name = plf.name
	local description = plf.description

	-- Register new platform
	premake.platforms[name] = {
		cfgsuffix = "_"..name,
		iscrosscompiler = true
	}

	-- Allow use of new platform in --platforms
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

function newclangtoolchain(toolchain)
	newplatform {
		name = toolchain.name,
		description = toolchain.description,
		gcc = {
			cc = toolchain.prefix .. "clang",
			cxx = toolchain.prefix .. "clang++",
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
		cppflags = "-MMD"
	}
}

newplatform {
	name = "clang-static-analyze",
	description = "Clang static analysis build",
	gcc = {
		cc = "clang --analyze",
		cxx = "clang++ --analyze",
		ar = "ar",
		cppflags = "-MMD"
	}
}

newplatform {
	name = "emscripten",
	description = "Emscripten",
	gcc = {
		cc = "emcc",
		cxx = "em++",
		ar = "emar",
		cppflags = "-MMD -D__emscripten__"
	}
}

newgcctoolchain {
	name = "mingw32",
	description = "Mingw32 to cross-compile windows binaries from *nix",
	prefix = "i686-w64-mingw32-",
	cppflags = "-B /usr/bin/i686-w64-mingw32-"
}

newgcctoolchain {
	name = "mingw64",
	description = "Mingw64 to cross-compile windows binaries from *nix",
	prefix = "x86_64-w64-mingw32-",
	cppflags = "-B /usr/bin/x86_64-w64-mingw32-"
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
	description = "iOS ARMv7 ( cross-compiling )",
	gcc = {
		cc = "ios-clang",
		cxx = "ios-clang++",
		ar = "arm-apple-darwin11-ar",
		cppflags = "-MMD -march=armv7 -marm -mcpu=cortex-a8"
	}
}

newplatform {
	name = "ios-cross-x86",
	description = "iOS x86 ( cross-compiling )",
	gcc = {
		cc = "ios-clang",
		cxx = "ios-clang++",
		ar = "arm-apple-darwin11-ar",
		cppflags = "-MMD -march=i386 -m32"
	}
}

newclangtoolchain {
	name ="ios-arm64",
	description = "iOS ARM64",
	prefix = iif( os.getenv("TOOLCHAINPATH"), os.getenv("TOOLCHAINPATH"), "" ),
	cppflags = "-arch arm64"
}

newclangtoolchain {
	name ="ios-x86",
	description = "iOS x86",
	prefix = iif( os.getenv("TOOLCHAINPATH"), os.getenv("TOOLCHAINPATH"), "" ),
	cppflags = "-m32 -arch i386"
}

newclangtoolchain {
	name ="ios-x86_64",
	description = "iOS x86_64",
	prefix = iif( os.getenv("TOOLCHAINPATH"), os.getenv("TOOLCHAINPATH"), "" ),
	cppflags = "-m64 -arch x86_64"
}

if _OPTIONS.platform then
	-- overwrite the native platform with the options::platform
	premake.gcc.platforms['Native'] = premake.gcc.platforms[_OPTIONS.platform]
end

newoption { trigger = "with-openssl", description = "Enables OpenSSL support ( and disables mbedtls backend )." }
newoption { trigger = "with-dynamic-freetype", description = "Dynamic link against freetype." }
newoption { trigger = "with-static-eepp", description = "Force to build the demos and tests with eepp compiled statically" }
newoption { trigger = "with-static-backend", description = "It will try to compile the library with a static backend (only for gcc and mingw).\n\t\t\t\tThe backend should be placed in libs/your_platform/libYourBackend.a" }
newoption { trigger = "with-gles2", description = "Compile with GLES2 support" }
newoption { trigger = "with-gles1", description = "Compile with GLES1 support" }
newoption { trigger = "without-mojoal", description = "Compile without mojoAL as OpenAL implementation (that requires SDL2 backend). Instead it will use openal-soft." }
newoption { trigger = "use-frameworks", description = "In macOS it will try to link the external libraries from its frameworks. For example, instead of linking against SDL2 it will link against SDL2.framework." }
newoption { trigger = "with-mold-linker", description = "Tries to use the mold linker instead of the default linker of the toolchain" }
newoption { trigger = "with-debug-symbols", description = "Release builds are built with debug symbols." }
newoption { trigger = "thread-sanitizer", description ="Compile with ThreadSanitizer." }
newoption { trigger = "address-sanitizer", description = "Compile with AddressSanitizer." }
newoption { trigger = "time-trace", description = "Compile with time trace." }
newoption { trigger = "disable-static-build", description = "Disables eepp static build project, this is just a helper to avoid rebuilding twice eepp while developing the library." }
newoption { trigger = "with-text-shaper", description = "Enables text-shaping capabilities by relying on harfbuzz." }
newoption {
	trigger = "with-backend",
	description = "Select the backend to use for window and input handling.\n\t\t\tIf no backend is selected or if the selected is not installed the script will search for a backend present in the system, and will use it.",
	allowed = {
		{ "SDL2",  "SDL2" }
	}
}
newoption { trigger = "with-static-cpp", description = "Builds statically libstdc++" }

function explode(div,str)
	if (div=='') then return false end
	local pos,arr = 0,{}
	for st,sp in function() return string.find(str,div,pos,true) end do
		table.insert(arr,string.sub(str,pos,st-1))
		pos = sp + 1
	end
	table.insert(arr,string.sub(str,pos))
	return arr
end

function os.get_real()
	if 	_OPTIONS.platform == "ios-arm64" or
		_OPTIONS.platform == "ios-x86" or
		_OPTIONS.platform == "ios-x86_64" or
		_OPTIONS.platform == "ios-cross-arm7" or
		_OPTIONS.platform == "ios-cross-x86" then
		return "ios"
	end

	if _OPTIONS.platform == "android-arm7" then
		return "android"
	end

	if _OPTIONS.platform == "mingw32" then
		return _OPTIONS.platform
	end

	if _OPTIONS.platform == "emscripten" then
		return _OPTIONS.platform
	end

	return os.get()
end

function os.is_real( os_name )
	return os.get_real() == os_name
end

if os.is_real("haiku") and not os.is64bit() then
	premake.gcc.cc = "gcc-x86"
	premake.gcc.cxx = "g++-x86"
	premake.gcc.ar = "ar-x86"
end

function get_dll_extension()
	if os.is_real("macosx") then
		return "dylib"
	elseif os.is_real("windows") or os.is_real("mingw32") then
		return "dll"
	else
		return "so"
	end
end

function get_host()
	if os.getenv("MSYSTEM") ~= "" and not is_vs() then
		return "msys"
	end
	return os.get()
end

function os_ishost(host)
	return get_host() == host
end

function postsymlinklib(src_path, dst_path, lib)
	if os.is_real("emscripten") then
		return
	end
	configuration { "release", "windows" }
		if os_ishost("windows") then
			postbuildcommands { "mklink \"" .. dst_path .. lib .. ".dll\"" .. " \"" .. src_path .. lib .. ".dll\" || ver>nul" }
		else
			postbuildcommands { "ln -sf \"" .. src_path .. lib .. "." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }
		end
	configuration { "debug", "windows" }
		if os_ishost("windows") then
			postbuildcommands { "mklink \"" .. dst_path .. lib .. "-debug.dll\"" .. " \"" .. src_path .. lib .. "-debug.dll\" || ver>nul" }
		else
			postbuildcommands { "ln -sf \"" .. src_path .. lib .. "-debug." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }
		end
	configuration { "release", "not windows" }
		postbuildcommands { "ln -sf \"" .. src_path .. "lib" .. lib .. "." .. get_dll_extension() .. "\" \"" .. dst_path .. "\"" }
	configuration { "debug", "not windows" }
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

function args_contains( element )
	return table.contains( _ARGS, element )
end

function multiple_insert( parent_table, insert_table )
	for _, value in pairs( insert_table ) do
		table.insert( parent_table, value )
	end
end

function get_ios_arch()
	local archs = explode( "-", _OPTIONS.platform )
	return archs[ table_length( archs ) ]
end

function os_findlib( name )
	if os.is_real("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		local path = "/Library/Frameworks/" .. name .. ".framework"

		if os.isdir( path ) then
			return path
		end
	end

	return os.findlib( name )
end

function get_backend_link_name( name )
	if os.is_real("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
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
	if os.is_real("macosx") then
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
remote_sdl2_version = "SDL2-2.32.8"

function build_arch_configuration()
	if os.is_real("mingw32") or os.is_real("mingw64") then
		buildoptions { "-D__USE_MINGW_ANSI_STDIO=1" }
	end
end

function build_base_configuration( package_name )
	includedirs { "src/thirdparty/zlib" }

	if not os.is("windows") and not os.is_real("emscripten") then
		buildoptions{ "-fPIC" }
	end

	if is_vs() then
		includedirs { "src/thirdparty/libzip/vs" }
		buildoptions { "/utf-8" }
	end

	set_ios_config()
	set_apple_config()
	build_arch_configuration()

	configuration "debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		if not is_vs() then
			buildoptions{ "-Wall", "-std=gnu99" }
		end
		targetname ( package_name .. "-debug" )
		if os.is_real("emscripten") then
			buildoptions{ "-g3" }
		end

	configuration "release"
		defines { "NDEBUG" }
		flags { "OptimizeSpeed" }
		if _OPTIONS["with-debug-symbols"] then
			flags { "Symbols" }
		end
		if not is_vs() then
			buildoptions{ "-Wall", "-std=gnu99" }
		end
		if os.is_real("emscripten") then
			buildoptions{ "-O3" }
		end
		targetname ( package_name )

	configuration "emscripten"
		buildoptions { "-s USE_SDL=2" }
		buildoptions { "-s USE_PTHREADS=1" }
end

function build_base_cpp_configuration( package_name )
	if not os.is("windows") and not os.is_real("emscripten") then
		buildoptions{ "-fPIC" }
	end

	if not is_vs() then
		buildoptions{ "-std=c++20" }
	else
		buildoptions{ "/std:c++20", "/utf-8" }
	end

	set_ios_config()
	set_apple_config()
	build_arch_configuration()

	if _OPTIONS["with-static-eepp"] then
		defines { "EE_STATIC" }
	end

	configuration "debug"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		flags { "Symbols" }
		if not is_vs() then
			buildoptions{ "-Wall" }
		end
		if os.is_real("emscripten") then
			buildoptions{ "-g3" }
		end
		targetname ( package_name .. "-debug" )

	configuration "release"
		defines { "NDEBUG" }
		flags { "OptimizeSpeed" }
		if _OPTIONS["with-debug-symbols"] then
			flags { "Symbols" }
		end
		if not is_vs() then
			buildoptions{ "-Wall" }
		end
		if os.is_real("emscripten") then
			buildoptions{ "-O3" }
		end
		targetname ( package_name )

	configuration "emscripten"
		buildoptions { "-s USE_SDL=2" }
		buildoptions { "-s USE_PTHREADS=1" }
end

function add_cross_config_links()
	if not is_vs() then
		if os.is_real("mingw32") or os.is_real("mingw64") or os.is_real("windows") or os.is_real("ios") then -- if is crosscompiling from *nix
			linkoptions { "-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic" }
		end

		if os.is_real("mingw32") or os.is_real("mingw64") then
			linkoptions { "-Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic" }
		end
	end
end

function fix_shared_lib_linking_path( package_name, libname )
	if ( "4.4-beta5" == _PREMAKE_VERSION or "HEAD" == _PREMAKE_VERSION ) and not _OPTIONS["with-static-eepp"] and package_name == "eepp" then
		if os.is("macosx") then
			linkoptions { "-install_name " .. libname .. ".dylib" }
		elseif os.is("linux") or os.is("freebsd") then
			linkoptions { "-Wl,-soname=\"" .. libname .. "\"" }
		elseif os.is("haiku") then
			linkoptions { "-Wl,-soname=\"" .. libname .. ".so" .. "\"" }
		end
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

function popen( executable_path )
	local handle = io.popen(executable_path)
	local result = handle:read("*a")
	handle:close()
	return result
end

function build_link_configuration( package_name, use_ee_icon )
	includedirs { "include" }

	local extension = "";

	if package_name == "eepp" then
		defines { "EE_EXPORTS" }
	elseif package_name == "eepp-static" then
		defines { "EE_STATIC" }
	end

	if not is_vs() then
		buildoptions{ "-std=c++20" }
	else
		buildoptions{ "/std:c++20", "/utf-8", "/bigobj" }
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
				if os.is64bit() then
					linkoptions { "../../bin/assets/icon/ee.x64.res" }
				else
					linkoptions { "../../bin/assets/icon/ee.res" }
				end
			end
		end

		if os.is_real("emscripten") then
			extension = ".html"

			if (	package_name ~= "eeew" and
					package_name ~= "eees" and
					package_name ~= "eehttp-request" and
					package_name ~= "eephysics" and
					package_name ~= "eevbo-fbo-batch"
			) then
				if package_name == "ecode" then
					linkoptions { "--preload-file " .. package_name .. "/assets/" }
				else
					linkoptions { "--preload-file assets/" }
				end
			end
		end

		if _OPTIONS.platform == "ios-cross-arm7" then
			extension = ".ios"
		end

		if _OPTIONS.platform == "ios-cross-x86" then
			extension = ".x86.ios"
		end

		if _OPTIONS.platform == "ios-cross-x86_64" then
			extension = ".x86_64.ios"
		end

		if os.is_real("linux") or os.is_real("haiku") or os.is_real("bsd") or os.is_real("macosx") then
			if _ACTION == "gmake" then
				linkoptions { "-Wl,-rpath,'$$ORIGIN'" }
			elseif _ACTION == "codeblocks" then
				linkoptions { "-Wl,-R\\\\$$$ORIGIN" }
			end
		end
	end

	if _OPTIONS["with-mold-linker"] then
		if _OPTIONS.platform == "clang" or _OPTIONS.platform == "clang-analyzer" then
			linkoptions { "-fuse-ld=mold" }
		else
			gccversion = popen( "gcc -dumpfullversion" )
			gccversionnumber = version_to_number( gccversion )
			if gccversionnumber >= 12110 then
				linkoptions { "-fuse-ld=mold" }
			else
				linkoptions { "-B/usr/bin/mold" }
			end
		end
	end

	if _OPTIONS["with-text-shaper"] then
		defines { "EE_TEXT_SHAPER_ENABLED" }
	end

	if _OPTIONS["with-static-cpp"] then
		linkoptions { "-static-libgcc -static-libstdc++" }
	end

	set_ios_config()
	set_apple_config()
	build_arch_configuration()

	configuration "debug"
		defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
		flags { "Symbols" }

		if not is_vs() and not os.is_real("emscripten") then
			buildoptions{ "-Wall -Wno-long-long" }
		end

		if os.is_real("emscripten") then
			linkoptions{ "--profiling --profiling-funcs -s DEMANGLE_SUPPORT=1 -s NO_DISABLE_EXCEPTION_CATCHING -sALLOW_MEMORY_GROWTH=1" }
			buildoptions { "-s USE_PTHREADS=1" }
			linkoptions { "-s USE_PTHREADS=1 -sPTHREAD_POOL_SIZE=8" }
		end

		fix_shared_lib_linking_path( package_name, "libeepp-debug" )

		if not os.is_real("emscripten") then
			targetname ( package_name .. "-debug" .. extension )
		else
			targetname ( package_name .. extension )
		end

	configuration "release"
		defines { "NDEBUG" }
		flags { "OptimizeSpeed" }
		if _OPTIONS["with-debug-symbols"] then
			flags { "Symbols" }
		end

		if not is_vs() and not os.is_real("emscripten") then
			buildoptions { "-fno-strict-aliasing" }
		end

		fix_shared_lib_linking_path( package_name, "libeepp" )

		targetname ( package_name .. extension )

	configuration "windows"
		add_cross_config_links()

	configuration "emscripten"
		linkoptions { "-s TOTAL_MEMORY=536870912 -s ALLOW_MEMORY_GROWTH=1 -s USE_SDL=2" }
		buildoptions { "-s USE_SDL=2" }
		buildoptions { "-s USE_PTHREADS=1" }
		linkoptions { "-s USE_PTHREADS=1 -sPTHREAD_POOL_SIZE=8" }

		if _OPTIONS["with-gles1"] and ( not _OPTIONS["with-gles2"] or _OPTIONS["force-gles1"] ) then
			linkoptions{ "-s LEGACY_GL_EMULATION=1" }
		end

		if _OPTIONS["with-gles2"] and not _OPTIONS["force-gles1"] then
			linkoptions{ "-s FULL_ES2=1" }
		end
end

function generate_os_links()
	if os.is_real("linux") then
		multiple_insert( os_links, { "rt", "pthread", "GL" } )

		if _OPTIONS["without-mojoal"] then
			table.insert( os_links, "openal" )
		end

		if _OPTIONS["with-static-eepp"] then
			table.insert( os_links, "dl" )
		end
	elseif os.is_real("windows") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32", "uuid" } )
	elseif os.is_real("mingw32") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32", "uuid" } )
	elseif os.is_real("mingw64") then
		multiple_insert( os_links, { "opengl32", "glu32", "gdi32", "ws2_32", "winmm", "ole32", "uuid" } )
	elseif os.is_real("macosx") then
		multiple_insert( os_links, { "OpenGL.framework", "CoreFoundation.framework" } )
	elseif os.is_real("freebsd") then
		multiple_insert( os_links, { "rt", "pthread", "GL" } )
	elseif os.is_real("haiku") then
		multiple_insert( os_links, { "GL", "network" } )
	elseif os.is_real("ios") then
		multiple_insert( os_links, { "OpenGLES.framework", "AudioToolbox.framework", "CoreAudio.framework", "Foundation.framework", "CoreFoundation.framework", "UIKit.framework", "QuartzCore.framework", "CoreGraphics.framework", "CoreMotion.framework", "AVFoundation.framework", "GameController.framework" } )
	end

	if _OPTIONS["without-mojoal"] then
		if os.is_real("linux") or os.is_real("freebsd") or os.is_real("haiku") or os.is_real("emscripten") then
			multiple_insert( os_links, { "openal" } )
		elseif os.is_real("windows") or os.is_real("mingw32") or os.is_real("mingw64") then
			if os_ishost("msys") then
				multiple_insert( os_links, { "openal" } )
			else
				multiple_insert( os_links, { "OpenAL32" } )
			end
		elseif os.is_real("macosx") or os.is_real("ios") then
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
		if not os.is_real("macosx") then
			links { "tsan" }
		end
	end

	if _OPTIONS["address-sanitizer"] then
		buildoptions { "-fsanitize=address" }
		linkoptions { "-fsanitize=address" }
		if not os.is_real("macosx") then
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
		links { "freetype-static", "libpng-static" }
	end

	if _OPTIONS["with-text-shaper"] then
		links { "harfbuzz-static" }
		defines { "EE_TEXT_SHAPER_ENABLED" }
	end

	links { "SOIL2-static",
			"libzip-static",
			"jpeg-compressor-static",
			"zlib-static",
			"imageresampler-static",
			"pugixml-static",
			"vorbis-static",
			"pcre2-8-static",
			"oniguruma-static",
			"libwebp-static",
	}

	if not _OPTIONS["without-mojoal"] then
		links { "mojoal-static"}
	end

	if not _OPTIONS["with-openssl"] and not os.is_real("emscripten") then
		links { "mbedtls-static" }
	end

	if not os.is_real("haiku") and not os.is_real("ios") and not os.is_real("android") and not os.is_real("emscripten") then
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
	print("Using SDL2 backend");
	files { "src/eepp/window/backend/SDL2/*.cpp" }
	defines { "EE_BACKEND_SDL_ACTIVE", "EE_SDL_VERSION_2" }

	if not can_add_static_backend("SDL2") then
		if not os.is_real("emscripten") then
			table.insert( link_list, get_backend_link_name( "SDL2" ) )
		end
	else
		insert_static_backend( "SDL2" )
	end
end

function set_apple_config()
	if is_xcode() or _OPTIONS["use-frameworks"] then
		linkoptions { "-F /Library/Frameworks" }
		buildoptions { "-F /Library/Frameworks" }
		includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
	end
	if os.is("macosx") then
		defines { "EE_SDL2_FROM_ROOTPATH" }
		if not is_xcode() and not _OPTIONS["use-frameworks"] then
			local sdl2flags = popen("sdl2-config --cflags"):gsub("\n", "")
			buildoptions { sdl2flags }
		end
	end
end

function set_ios_config()
	if _OPTIONS.platform == "ios-arm64" or _OPTIONS.platform == "ios-x86" or _OPTIONS.platform == "ios-x86_64" then
		local err = false

		if nil == os.getenv("TOOLCHAINPATH") then
			print("You must set TOOLCHAINPATH environment variable.")
			print("\tExample: /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/")
			err = true
		end

		if nil == os.getenv("SYSROOTPATH") then
			print("You must set SYSROOTPATH environment variable.")
			print("\tExample: /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS12.1.sdk")
			err = true
		end

		if nil == os.getenv("IOSVERSION") then
			print("You must set IOSVERSION environment variable.")
			print("\tExample: 12.1")
			err = true
		end

		if err then
			os.exit(1)
		end

		local sysroot_path = os.getenv("SYSROOTPATH")
		local framework_path = sysroot_path .. "/System/Library/Frameworks"
		local framework_libs_path = framework_path .. "/usr/lib"
		local sysroot_ver = " -miphoneos-version-min=9.0 -isysroot " .. sysroot_path

		buildoptions { sysroot_ver .. " -I" .. sysroot_path .. "/usr/include" }
		linkoptions { sysroot_ver }
		libdirs { framework_libs_path }
		linkoptions { " -F" .. framework_path .. " -L" .. framework_libs_path .. " -isysroot " .. sysroot_path }
		includedirs { "src/thirdparty/" .. remote_sdl2_version .. "/include" }
	end

	if _OPTIONS.platform == "ios-cross-arm7" or _OPTIONS.platform == "ios-cross-x86" then
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

	if os.is_real("mingw32") or os.is_real("mingw64") or os.is_real("emscripten") then
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
			add_sdl2( true )
		end
	end
end

function check_ssl_support()
	if not os.is_real("emscripten") then
		if _OPTIONS["with-openssl"] then
			if os.is("windows") then
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
end

function set_macos_and_ios_config()
	if os.is_real("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		libdirs { "/System/Library/Frameworks", "/Library/Frameworks" }
	elseif os.is_real("macosx") then
		libdirs { "/opt/homebrew/lib" }
	end

	libdirs { "src/thirdparty" }

	if _OPTIONS["use-frameworks"] then
		defines { "EE_USE_FRAMEWORKS" }
	end
end

function eepp_module_maps_add()
	links { "eepp-maps-static" }
	defines { "EE_MAPS_STATIC" }
	includedirs { "src/modules/maps/include/", "src/modules/maps/src/" }
end

function eepp_module_physics_add()
	links { "eepp-physics-static", "chipmunk-static" }
	defines { "EE_PHYSICS_STATIC" }
	includedirs { "src/modules/physics/include/", "src/modules/physics/src/" }
end

function build_eepp( build_name )
	includedirs {
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
	}

	defines { "PCRE2_STATIC", "PCRE2_CODE_UNIT_WIDTH=8", "ONIG_STATIC" }

	if not _OPTIONS["without-mojoal"] then
		defines { "AL_LIBTYPE_STATIC", "EE_MOJOAL" }
		includedirs { "src/thirdparty/mojoAL" }
	end

	set_macos_and_ios_config()
	set_ios_config()
	set_apple_config()

	add_static_links()

	if is_vs() then
		includedirs { "src/thirdparty/libzip/vs" }
	end

	if not is_vs() then
		buildoptions{ "-std=c++20" }
	else
		buildoptions{ "/std:c++20", "/utf-8", "/bigobj" }
	end

	if os.is_real("mingw32") or os.is_real("mingw64") or os.is_real("windows") then
		files { "src/eepp/system/platform/win/*.cpp" }
		files { "src/eepp/network/platform/win/*.cpp" }
	else
		files { "src/eepp/system/platform/posix/*.cpp" }
		files { "src/eepp/network/platform/unix/*.cpp" }
	end

	if os.is_real("windows") then
		links { "bcrypt" }
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
			"src/eepp/ui/abstract/*.cpp",
			"src/eepp/ui/models/*.cpp",
			"src/eepp/ui/css/*.cpp",
			"src/eepp/ui/doc/*.cpp",
			"src/eepp/ui/doc/languages/*.cpp",
			"src/eepp/ui/tools/*.cpp"
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
	if os.is_real("ios") then
		targetdir(dir .. get_ios_arch() .. "/" )
	else
		targetdir(dir)
	end
end

solution "eepp"
	targetdir("./bin/")
	configurations { "debug", "release" }

	if os.is_real("ios") then
		location("./make/" .. _OPTIONS.platform .. "/" )
		objdir("obj/" .. os.get_real() .. "/" .. get_ios_arch() .. "/" )
	else
		location("./make/" .. os.get_real() .. "/")
		objdir("obj/" .. os.get_real() .. "/")
	end

	generate_os_links()
	parse_args()

	if os.is_real("macosx") then
		defines { "GL_SILENCE_DEPRECATION" }
	end

	project "SOIL2-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/SOIL2/src/SOIL2/*.c" }
		includedirs { "src/thirdparty/SOIL2" }
		build_base_configuration( "SOIL2" )

	if not os.is_real("haiku") and not os.is_real("ios") and not os.is_real("android") and not os.is_real("emscripten") then
		project "glew-static"
			kind "StaticLib"
			language "C"
			defines { "GLEW_NO_GLU", "GLEW_STATIC" }
			set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
			files { "src/thirdparty/glew/*.c" }
			includedirs { "include/thirdparty/glew" }
			build_base_configuration( "glew" )
	end

	project "mbedtls-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		includedirs { "src/thirdparty/mbedtls/include/" }
		files { "src/thirdparty/mbedtls/library/*.c" }
		build_base_configuration( "mbedtls" )

	project "vorbis-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		includedirs { "src/thirdparty/libvorbis/lib/", "src/thirdparty/libogg/include", "src/thirdparty/libvorbis/include" }
		files { "src/thirdparty/libogg/**.c", "src/thirdparty/libvorbis/**.c" }
		build_base_configuration( "vorbis" )

	project "pugixml-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/pugixml/*.cpp" }
		build_base_cpp_configuration( "pugixml" )

	project "zlib-static"
		kind "StaticLib"
		language "C"
		defines { "_LARGEFILE64_SOURCE" }
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/zlib/*.c" }
		build_base_configuration( "zlib" )

	project "libzip-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/libzip/*.c" }
		includedirs { "src/thirdparty/zlib" }
		build_base_configuration( "libzip" )

	project "libpng-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/libpng/**.c" }
		includedirs { "src/thirdparty/libpng/include" }
		build_base_configuration( "libpng" )

	project "libwebp-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/libwebp/**.c" }
		includedirs { "src/thirdparty/libwebp" }
		build_base_configuration( "libwebp" )

	project "freetype-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		defines { "FT2_BUILD_LIBRARY" }
		files { "src/thirdparty/freetype2/src/**.c" }
		includedirs { "src/thirdparty/freetype2/include", "src/thirdparty/libpng" }
		build_base_configuration( "freetype" )

	project "pcre2-8-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
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
		if not os.is_real("emscripten") then
			files { 'src/thirdparty/pcre2/src/pcre2_jit_compile.c' }
		end
		includedirs { "src/thirdparty/pcre2/src" }
		build_base_configuration( "pcre2-8" )

	project "oniguruma-static"
		kind "StaticLib"
		language "C"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
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
		includedirs { "src/thirdparty/oniguruma" }
		build_base_configuration( "oniguruma" )

	if _OPTIONS["with-text-shaper"] then
		project "harfbuzz-static"
			kind "StaticLib"
			language "C++"
			set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
			defines { "HAVE_CONFIG_H" }
			files { "src/thirdparty/harfbuzz/**.cc" }
			includedirs { "src/thirdparty/freetype2/include", "src/thirdparty/harfbuzz" }
			build_base_cpp_configuration( "harfbuzz" )
			if is_vs() then
				buildoptions{ "/bigobj" }
			end
	end

	project "chipmunk-static"
		kind "StaticLib"

		if is_vs() then
			language "C++"
			buildoptions { "/TP" }
		else
			language "C"
		end

		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/chipmunk/*.c", "src/thirdparty/chipmunk/constraints/*.c" }
		includedirs { "src/modules/physics/include/eepp/thirdparty/chipmunk" }
		build_base_configuration( "chipmunk" )

	project "jpeg-compressor-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/jpeg-compressor/*.cpp" }
		build_base_cpp_configuration( "jpeg-compressor" )

	project "imageresampler-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		files { "src/thirdparty/imageresampler/*.cpp" }
		build_base_cpp_configuration( "imageresampler" )

	if not _OPTIONS["without-mojoal"] then
		project "mojoal-static"
			kind "StaticLib"
			language "C"
			defines {"AL_LIBTYPE_STATIC", "EE_MOJOAL" }
			set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
			includedirs { "include/eepp/thirdparty/mojoAL" }
			files { "src/thirdparty/mojoAL/*.c" }
			build_base_configuration( "mojoal" )
	end

	project "efsw-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")
		includedirs { "src/thirdparty/efsw/include", "src/thirdparty/efsw/src" }
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end

		if os.is("windows") then
			osfiles = "src/thirdparty/efsw/src/efsw/platform/win/*.cpp"
		else
			osfiles = "src/thirdparty/efsw/src/efsw/platform/posix/*.cpp"
		end

		files { "src/thirdparty/efsw/src/efsw/*.cpp", osfiles }

		if os.is("windows") then
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp"
			}
		elseif os.is("linux") then
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherKqueue.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp"
			}
		elseif os.is("macosx") then
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp"
			}
		elseif os.is("freebsd") then
			excludes {
				"src/thirdparty/efsw/src/efsw/WatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/WatcherFSEvents.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherInotify.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherWin32.cpp",
				"src/thirdparty/efsw/src/efsw/FileWatcherFSEvents.cpp"
			}
		end

		build_base_cpp_configuration( "efsw" )

	project "eepp-maps-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		includedirs { "include", "src/modules/maps/include/", "src/modules/maps/src/" }
		files { "src/modules/maps/src/**.cpp" }
		defines { "EE_MAPS_STATIC" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end
		build_base_cpp_configuration( "eepp-maps-static" )

	project "eepp-maps"
		kind "SharedLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		includedirs { "include", "src/modules/maps/include/", "src/modules/maps/src/" }
		files { "src/modules/maps/src/**.cpp" }
		links { "eepp-shared" }
		defines { "EE_MAPS_EXPORTS" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end
		build_base_cpp_configuration( "eepp-maps" )
		postsymlinklib("../libs/" .. os.get_real() .. "/", "../../bin/", "eepp-maps" )

	project "eepp-physics-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		includedirs { "include", "src/modules/physics/include/","src/modules/physics/src/" }
		files { "src/modules/physics/src/**.cpp", "src/eepp/physics/constraints/*.cpp" }
		defines { "EE_PHYSICS_STATIC" }
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end
		build_base_cpp_configuration( "eepp-physics-static" )

	project "eepp-physics"
		kind "SharedLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		includedirs { "include", "src/modules/physics/include/","src/modules/physics/src/" }
		files { "src/modules/physics/src/**.cpp", "src/eepp/physics/constraints/*.cpp" }
		links { "chipmunk-static", "eepp-shared" }
		defines { "EE_PHYSICS_EXPORTS" }
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end
		build_base_cpp_configuration( "eepp-physics" )
		postsymlinklib("../libs/" .. os.get_real() .. "/", "../../bin/", "eepp-physics" )

	project "eterm-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		includedirs { "include", "src/modules/eterm/include/","src/modules/eterm/src/" }
		files { "src/modules/eterm/src/**.cpp" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		if os_ishost("msys") then
			defines { "WINVER=0x0602" }
		end
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end
		build_base_cpp_configuration( "eterm" )

	project "languages-syntax-highlighting-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		includedirs { "include", "src/modules/languages-syntax-highlighting/src" }
		files { "src/modules/languages-syntax-highlighting/src/**.cpp" }
		if _OPTIONS["with-static-eepp"] then
			defines { "EE_STATIC" }
		end
		if not is_vs() then
			buildoptions{ "-std=c++20" }
		else
			buildoptions{ "/std:c++20" }
		end
		build_base_cpp_configuration( "languages-syntax-highlighting" )

	-- Library
	if not _OPTIONS["disable-static-build"] then
	project "eepp-static"
		kind "StaticLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		build_eepp( "eepp-static" )
	end

	project "eepp-shared"
		kind "SharedLib"
		language "C++"
		set_targetdir("libs/" .. os.get_real() .. "/")
		build_eepp( "eepp" )
		postsymlinklib("../libs/" .. os.get_real() .. "/", "../../bin/", "eepp" )
		postsymlinklib("../../libs/" .. os.get_real() .. "/", "../../bin/unit_tests/", "eepp" )

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
		includedirs { "src/thirdparty" }
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
		includedirs { "src/thirdparty/efsw/include", "src/thirdparty" }
		links { "efsw-static", "pugixml-static" }
		if not os.is_real("windows") and not os.is_real("haiku") then
			links { "pthread" }
		end
		if os.is_real("macosx") then
			links { "CoreFoundation.framework", "CoreServices.framework" }
		end
		files { "src/tools/uieditor/*.cpp" }
		build_link_configuration( "eepp-UIEditor", true )

	if os.is("macosx") then
	project "ecode-macos-helper-static"
		kind "StaticLib"
		language "C++"
		files { "src/tools/ecode/macos/*.m" }
		set_targetdir("libs/" .. os.get_real() .. "/thirdparty/")

		configuration "debug"
			defines { "DEBUG", "EE_DEBUG", "EE_MEMORY_MANAGER" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			buildoptions{ "-g3" }
			targetname ( "ecode-macos-helper-static-debug" )

		configuration "release"
			defines { "NDEBUG" }
			flags { "OptimizeSpeed" }
			if _OPTIONS["with-debug-symbols"] then
				flags { "Symbols" }
			end
			buildoptions{ "-O3" }
			targetname ( "ecode-macos-helper-static" )
	end

	project "ecode"
		set_kind()
		language "C++"
		files { "src/tools/ecode/**.cpp" }
		includedirs { "src/thirdparty/efsw/include", "src/thirdparty", "src/modules/eterm/include/", "src/modules/languages-syntax-highlighting/src" }
		links { "efsw-static", "eterm-static", "languages-syntax-highlighting-static" }
		if not os.is("windows") and not os.is("haiku") then
			links { "pthread" }
		end
		if os.is("macosx") then
			links { "CoreFoundation.framework", "CoreServices.framework", "Cocoa.framework" }
			links { "ecode-macos-helper-static" }
		end
		if os.is_real("linux") then
			links { "util" }

			if os_findlib("dw") then
				links { "dw" }
				defines { "ECODE_HAS_DW" }
			end
		end
		if os.is_real("windows") or os.is_real("mingw32") or os.is_real("mingw64") then
			links { "dbghelp", "psapi" }
		end
		if os.is("haiku") then
			links { "bsd", "network" }
		end
		if os.is("windows") and not is_vs() then
			if os.is64bit() then
				linkoptions { "../../bin/assets/icon/ecode.x64.res" }
			else
				linkoptions { "../../bin/assets/icon/ecode.res" }
			end
			buildoptions{ "-Wa,-mbig-obj" }
		end
		build_link_configuration( "ecode", false )
		configuration { "release", "windows" }
			if not is_vs() then
				buildoptions { "-g1" }
			end
		configuration { "release" }
			if os.is_real("linux") or os.is_real("macosx") or os.is_real("bsd") or os.is_real("haiku") then
				buildoptions { "-g1", "-fvisibility=default" }
			end
			if os.is_real("linux") or os.is_real("bsd") then
				linkoptions { "-rdynamic" }
			end

	project "eterm"
		set_kind()
		language "C++"
		files { "src/tools/eterm/**.cpp" }
		links { "eterm-static" }
		includedirs { "src/modules/eterm/include/", "src/thirdparty" }
		if os.is_real("linux") then
			links { "util" }
		end
		if os.is("haiku") then
			links { "bsd" }
		end
		if os.is("windows") and not is_vs() then
			if os.is64bit() then
				linkoptions { "../../bin/assets/icon/eterm.x64.res" }
			else
				linkoptions { "../../bin/assets/icon/eterm.res" }
			end
		end
		build_link_configuration( "eterm", false )

	project "eepp-texturepacker"
		kind "ConsoleApp"
		language "C++"
		includedirs { "src/thirdparty" }
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

	project "eepp-unit_tests"
		kind "ConsoleApp"
		targetdir("./bin/unit_tests")
		language "C++"
		files { "src/tests/unit_tests/*.cpp" }
		build_link_configuration( "eepp-unit_tests", true )

if os.isfile("external_projects.lua") then
	dofile("external_projects.lua")
end

if os.isfile("external_projects_premake4.lua") then
	dofile("external_projects_premake4.lua")
end
