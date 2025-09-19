#ifndef EE_CONFIG_HPP
#define EE_CONFIG_HPP

#define EE_PLATFORM_WIN 1
#define EE_PLATFORM_LINUX 2
#define EE_PLATFORM_MACOS 3
#define EE_PLATFORM_BSD 4
#define EE_PLATFORM_SOLARIS 5
#define EE_PLATFORM_HAIKU 6
#define EE_PLATFORM_ANDROID 7
#define EE_PLATFORM_IOS 8
#define EE_PLATFORM_EMSCRIPTEN 9

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( _WIN64 )
#define EE_PLATFORM EE_PLATFORM_WIN

#if ( defined( _MSCVER ) || defined( _MSC_VER ) )
#define EE_COMPILER_MSVC
#pragma warning( disable : 4267 )
#pragma warning( disable : 4251 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4311 )
#pragma warning( disable : 4312 )
#pragma warning( disable : 4068 )
#endif
#elif defined( __APPLE_CC__ ) || defined( __APPLE__ )

#include <TargetConditionals.h>

#if defined( __IPHONE__ ) || ( defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE ) || \
	( defined( TARGET_IPHONE_SIMULATOR ) && TARGET_IPHONE_SIMULATOR )
#define EE_PLATFORM EE_PLATFORM_IOS
#else
#define EE_PLATFORM EE_PLATFORM_MACOS
#endif
#elif defined( __emscripten__ ) || defined( EMSCRIPTEN )
#define EE_PLATFORM EE_PLATFORM_EMSCRIPTEN
#elif defined( __ANDROID__ ) || defined( ANDROID )
#define EE_PLATFORM EE_PLATFORM_ANDROID
#elif defined( linux ) || defined( __linux__ )
#define EE_PLATFORM EE_PLATFORM_LINUX
#elif defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ ) || \
	defined( __DragonFly__ )
#define EE_PLATFORM EE_PLATFORM_BSD
#elif defined( __SVR4 ) || defined( __sun )
#define EE_PLATFORM EE_PLATFORM_SOLARIS
#elif defined( __HAIKU__ ) || defined( __BEOS__ )
#define EE_PLATFORM EE_PLATFORM_HAIKU
#endif

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS || \
	EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )
#define EE_GLES2
#endif

#if !defined( EE_PLATFORM_TOUCH ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#define EE_PLATFORM_TOUCH
#endif
#endif

#if defined( linux ) || defined( __linux__ ) || defined( __FreeBSD__ ) ||          \
	defined( __OpenBSD__ ) || defined( __NetBSD__ ) || defined( __DragonFly__ ) || \
	defined( __SVR4 ) || defined( __sun )

#if !defined( EE_GLES1 ) && !defined( EE_GLES2 ) && defined( __has_include ) && \
	__has_include( <X11/Xlib.h> )
#define EE_X11_PLATFORM
#endif

#endif

#if ( defined( linux ) || defined( __linux__ ) ) && \
	( defined( __alpha ) || defined( __alpha__ ) || defined( __x86_64__ ) || defined( _M_X64 ) )
#define EE_LINUX_64
#endif

#if ( defined( __sparc__ ) || defined( __sparc ) ) && \
	( defined( __arch64__ ) || defined( __sparcv9 ) || defined( __sparc_v9__ ) )
#define EE_SPARC_64
#endif

//! Since EE just use basic POSIX stuff, declare as POSIX some OS that are mostly POSIX-compliant
#if defined( linux ) || defined( __linux__ ) || defined( __FreeBSD__ ) ||                       \
	defined( __OpenBSD__ ) || defined( __NetBSD__ ) || defined( __DragonFly__ ) ||              \
	defined( __SVR4 ) || defined( __sun ) || defined( __APPLE_CC__ ) || defined( __APPLE__ ) || \
	defined( __HAIKU__ ) || defined( __BEOS__ ) || defined( __emscripten__ ) ||                 \
	defined( EMSCRIPTEN )
#define EE_PLATFORM_POSIX
#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD ||     \
	EE_PLATFORM == EE_PLATFORM_SOLARIS || EE_PLATFORM == EE_PLATFORM_HAIKU || \
	EE_PLATFORM == EE_PLATFORM_ANDROID
#define EE_HAVE_CLOCK_GETTIME
#endif

#if defined( __GNUC__ )
#define EE_COMPILER_GCC
#endif

#if defined( arm ) || defined( __arm__ ) || defined( __aarch64__ ) || defined( _M_ARM64 ) || \
	defined( __ARM_ARCH_ISA_A64 ) || defined( __ARM_ARCH_7S__ ) || defined( __ARM_ARCH_7A__ )
#define EE_ARM
#endif

/// Activate at least one backend for the compilation
#if !defined( EE_BACKEND_SDL_ACTIVE )
#define EE_BACKEND_SDL_ACTIVE
#endif

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS || \
	( EE_PLATFORM == EE_PLATFORM_WIN && defined( EE_COMPILER_MSVC ) )
#define EE_OVERRIDES_MAIN
#if defined( EE_BACKEND_SDL_ACTIVE )
#if EE_PLATFORM == EE_PLATFORM_WIN && defined( EE_COMPILER_MSVC )
#define main SDL_main
#else
#define main EE_SDL_main
#endif
#endif
#endif

#if defined( __cplusplus ) && defined( EE_OVERRIDES_MAIN )
#define EE_MAIN_FUNC extern "C"
#else
#define EE_MAIN_FUNC
#endif

#if EE_PLATFORM != EE_PLATFORM_ANDROID
#define EE_SUPPORT_EXCEPTIONS
#endif

#ifndef EE_DEBUG
#ifndef NDEBUG
#define EE_DEBUG
#endif
#endif

#define EE_LITTLE_ENDIAN 1
#define EE_BIG_ENDIAN 2

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN || EE_PLATFORM == EE_PLATFORM_ANDROID
#define EE_ENDIAN EE_LITTLE_ENDIAN
#endif

#ifndef EE_ENDIAN

#if defined( __386__ ) || defined( i386 ) || defined( __i386__ ) || defined( __X86 ) ||            \
	defined( _M_IX86 ) || defined( _M_X64 ) || defined( __x86_64__ ) || defined( alpha ) ||        \
	defined( __alpha ) || defined( __alpha__ ) || defined( _M_ALPHA ) || defined( ARM ) ||         \
	defined( _ARM ) || defined( __arm__ ) || defined( WIN32 ) || defined( _WIN32 ) ||              \
	defined( __WIN32__ ) || defined( _WIN32_WCE ) || defined( __NT__ ) || defined( __MIPSEL__ ) || \
	defined( EE_ARM )
#define EE_ENDIAN EE_LITTLE_ENDIAN
#else
#define EE_ENDIAN EE_BIG_ENDIAN
#endif

#endif

#ifndef EE_STATIC
#if EE_PLATFORM == EE_PLATFORM_WIN
// Windows platforms
#ifdef EE_EXPORTS
// From DLL side, we must export
#define EE_API __declspec( dllexport )
#else
// From client application side, we must import
#define EE_API __declspec( dllimport )
#endif
#else
#if ( __GNUC__ >= 4 )
#define EE_API __attribute__( ( visibility( "default" ) ) )
#endif
#endif
#endif

#ifndef EE_API
#define EE_API
#endif

#if ( defined( EE_GLES2 ) || defined( EE_GLES1 ) ) && !defined( EE_GLES )
#define EE_GLES
#endif

#if defined( EE_GLES2 ) && defined( EE_GLES1 )
#define EE_GLES_BOTH
#endif

#if ( !defined( EE_GLES1 ) && ( defined( EE_GLES2 ) || !defined( EE_PLATFORM_TOUCH ) ) ) || \
	defined( EE_GLES_BOTH )
#define EE_SHADERS_SUPPORTED
#endif

#if ( EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	  defined( EE_X11_PLATFORM ) ) &&                                        \
	!defined( EE_GLES )
#define EE_GLEW_AVAILABLE
#endif

#define eeCOMMA ,
#define eeARRAY_SIZE( __array ) ( sizeof( __array ) / sizeof( __array[0] ) )
#define eeSAFE_DELETE( p ) \
	{                      \
		if ( p ) {         \
			eeDelete( p ); \
			( p ) = NULL;  \
		}                  \
	}
#define eeSAFE_FREE( p )        \
	{                           \
		if ( p ) {              \
			eeFree( (void*)p ); \
			( p ) = NULL;       \
		}                       \
	}
#define eeSAFE_DELETE_ARRAY( p ) \
	{                            \
		if ( p ) {               \
			eeDeleteArray( p );  \
			( p ) = NULL;        \
		}                        \
	}
#define eeINDEX_NOT_FOUND 0xFFFFFFFF

#ifndef likely
#if defined( __GNUC__ ) || defined( __clang__ )
#define likely( x ) __builtin_expect( !!( x ), 1 )
#else
#define likely( x ) ( x )
#endif
#endif

#ifndef unlikely
#if defined( __GNUC__ ) || defined( __clang__ )
#define unlikely( x ) __builtin_expect( !!( x ), 0 )
#else
#define unlikely( x ) ( x )
#endif
#endif

namespace EE {
#ifdef EE_USE_DOUBLES
typedef double Float;
#define eesqrt sqrt
#define eesin sin
#define eecos cos
#define eetan tan
#define eeacos acos
#define eeatan2 atan2
#define eemod fmod
#define eeexp exp
#define eepow pow
#define eefloor floor
#define eeceil ceil
#define eeabs abs
#else
typedef float Float; //! The internal floating point used on EE++. \n This can help to improve
					 //! compatibility with some platforms. \n And helps for an easy change from
					 //! single precision to double precision.
#define eesqrt sqrtf
#define eesin sinf
#define eecos cosf
#define eetan tanf
#define eeacos acosf
#define eeatan2 atan2f
#define eemod fmodf
#define eeexp expf
#define eepow powf
#define eefloor floorf
#define eeceil ceilf
#define eeabs fabs
#endif

#ifdef EE_COMPILER_MSVC
#define eevsnprintf( str, size, format, args ) _vsnprintf_s( str, size, size, format, args )
#else
#define eevsnprintf( str, size, format, args ) vsnprintf( str, size, format, args )
#endif

template <typename T> T eemax( T a, T b ) {
	return ( a > b ) ? a : b;
}

template <typename T> T eemin( T a, T b ) {
	return ( a < b ) ? a : b;
}

template <typename T> T eeabs( T n ) {
	return ( n < 0 ) ? -n : n;
}

template <typename T> T eeclamp( T val, T min, T max ) {
	return ( val < min ) ? min : ( ( val > max ) ? max : val );
}

template <typename T> void eeclamp( T* val, T min, T max ) {
	( *val < min ) ? * val = min : ( ( *val > max ) ? * val = max : *val );
}

typedef signed char Int8;
typedef unsigned char Uint8;
typedef signed short Int16;
typedef unsigned short Uint16;
typedef signed int Int32;
typedef unsigned int Uint32;

// 64 bits integer types
#if defined( _MSC_VER )
typedef signed __int64 Int64;
typedef unsigned __int64 Uint64;
#else
typedef signed long long Int64;
typedef unsigned long long Uint64;
#endif

#if defined( EE_LINUX_64 ) || defined( EE_SPARC_64 ) || defined( __osf__ ) ||                  \
	( defined( _WIN64 ) && !defined( _XBOX ) ) || defined( __64BIT__ ) || defined( __LP64 ) || \
	defined( __LP64__ ) || defined( _LP64 ) || defined( _ADDR64 ) || defined( _CRAYC )
typedef Int64 IntPtr;
typedef Uint64 UintPtr;
#else
typedef Int32 IntPtr;
typedef Uint32 UintPtr;
#endif

#define EE_PI ( 3.14159265358979323846 )
#define EE_PI2 ( 6.28318530717958647692 )
#define EE_PI_180 ( (Float)EE_PI / 180.f )
#define EE_PI_360 ( (Float)EE_PI / 360.f )
#define EE_180_PI ( 180.f / (Float)EE_PI )
#define EE_360_PI ( 360.f / (Float)EE_PI )
#define EE_1B ( 1 )
#define EE_1KB ( 1024 )
#define EE_1MB ( 1048576 )
#define EE_1GB ( 1073741824 )
#define EE_1TB ( 1099511627776 )
} // namespace EE

#endif
