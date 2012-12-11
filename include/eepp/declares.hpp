#ifndef EE_DECLARES_HPP
#define EE_DECLARES_HPP

#include <eepp/helper/sophist/sophist.h>
#include <cmath>

#define EE_PLATFORM_WIN		1
#define EE_PLATFORM_LINUX	2
#define EE_PLATFORM_MACOSX	3
#define EE_PLATFORM_BSD		4
#define EE_PLATFORM_SOLARIS	5
#define EE_PLATFORM_HAIKU	6
#define EE_PLATFORM_ANDROID	7
#define EE_PLATFORM_IOS		8

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( _WIN64 )
	#define EE_PLATFORM EE_PLATFORM_WIN

	#if ( defined( _MSCVER ) || defined( _MSC_VER ) )
		#define EE_COMPILER_MSVC
	#endif
#elif defined( __APPLE_CC__ ) || defined ( __APPLE__ )

	#include <TargetConditionals.h>

	#if defined( __IPHONE__ ) || ( defined( TARGET_OS_IPHONE ) && TARGET_OS_IPHONE ) || ( defined( TARGET_IPHONE_SIMULATOR ) && TARGET_IPHONE_SIMULATOR )
		#define EE_PLATFORM EE_PLATFORM_IOS
	#else
		#define EE_PLATFORM EE_PLATFORM_MACOSX
	#endif

#elif defined( __ANDROID__ ) || defined( ANDROID )
	#define EE_PLATFORM EE_PLATFORM_ANDROID
#elif defined ( linux ) || defined( __linux__ )
	#define EE_PLATFORM EE_PLATFORM_LINUX
#elif defined( __FreeBSD__ ) || defined(__OpenBSD__) || defined( __NetBSD__ ) || defined( __DragonFly__ )
	#define EE_PLATFORM EE_PLATFORM_BSD
#elif defined( __SVR4 ) || defined( __sun )
	#define EE_PLATFORM EE_PLATFORM_SOLARIS
#elif defined( __HAIKU__ ) || defined( __BEOS__ )
	#define EE_PLATFORM EE_PLATFORM_HAIKU
#endif

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS
	#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )
		#define EE_GLES2
	#endif

	#ifndef EE_PLATFORM_TOUCH
		#define EE_PLATFORM_TOUCH
	#endif
#endif

#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )
#define EE_PLATFORM_DESKTOP
#endif

#if defined ( linux ) || defined( __linux__ ) \
	|| defined( __FreeBSD__ ) || defined(__OpenBSD__) || defined( __NetBSD__ ) || defined( __DragonFly__ ) \
	|| defined( __SVR4 ) || defined( __sun )

#if !defined( EE_GLES1 ) && !defined( EE_GLES2 )

#define EE_X11_PLATFORM

#endif

#endif

//! Since EE just use basic POSIX stuff, declare as POSIX some OS that are mostly POSIX-compliant
#if defined ( linux ) || defined( __linux__ ) || defined( __FreeBSD__ ) || defined(__OpenBSD__) || defined( __NetBSD__ ) || defined( __DragonFly__ ) || defined( __SVR4 ) || defined( __sun ) || defined( __APPLE_CC__ ) || defined ( __APPLE__ ) || defined( __HAIKU__ ) || defined( __BEOS__ )
	#define EE_PLATFORM_POSIX
#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || EE_PLATFORM == EE_PLATFORM_SOLARIS || EE_PLATFORM == EE_PLATFORM_HAIKU
	#define EE_HAVE_CLOCK_GETTIME
#endif

#if defined(__GNUC__)
	#define EE_COMPILER_GCC
#endif

#if defined(arm) || defined(__arm__)
	#define	EE_ARM
#endif

/// Activate at least one backend for the compilation
#if !defined( EE_BACKEND_SDL_ACTIVE ) && !defined( EE_BACKEND_ALLEGRO_ACTIVE ) && !defined( EE_BACKEND_SFML_ACTIVE )
	#define EE_BACKEND_SDL_ACTIVE
#endif

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS || defined(  EE_COMPILER_MSVC )
	#if EE_PLATFORM == EE_PLATFORM_ANDROID
		#define EE_NO_WIDECHAR
	#endif

	#ifdef EE_BACKEND_SDL_ACTIVE
		#define main	SDL_main
	#endif

	#ifdef EE_BACKEND_ALLEGRO_ACTIVE
		#if EE_PLATFORM == EE_PLATFORM_IOS
			#define ALLEGRO_MAGIC_MAIN
			#define main _al_mangled_main
		#elif EE_PLATFORM == EE_PLATFORM_ANDROID
			#ifdef __cplusplus
			extern "C" int main(int argc, char ** argv);
			#else
			extern int main(int argc, char ** argv);
			#endif
		#endif
	#endif

	#ifndef EE_MAIN_FUNC
		#ifdef __cplusplus
			#define EE_MAIN_FUNC extern "C"
		#else
			#define EE_MAIN_FUNC
		#endif
	#endif
#else
	#ifndef EE_MAIN_FUNC
		#define EE_MAIN_FUNC
	#endif

	#define EE_SUPPORT_EXCEPTIONS
#endif

#ifndef EE_DEBUG
	#if defined( DEBUG ) || defined( _DEBUG ) || defined( __DEBUG ) || defined( __DEBUG__ )
		#define EE_DEBUG
	#endif
#endif

#if 1 == SOPHIST_pointer64
	#define EE_64BIT
#else
	#define EE_32BIT
#endif

#define EE_LITTLE_ENDIAN	1
#define EE_BIG_ENDIAN		2

#if SOPHIST_little_endian == SOPHIST_endian
	#define EE_ENDIAN EE_LITTLE_ENDIAN
#else
	#define EE_ENDIAN EE_BIG_ENDIAN
#endif

#ifdef EE_PLATFORM
	#define EE_SUPPORTED_PLATFORM
#else
	#error Platform not supported
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
	#ifdef EE_COMPILER_MSVC
		#pragma warning(disable : 4251)
		#pragma warning(disable : 4244)
		#pragma warning(disable : 4996)
		#pragma warning(disable : 4311)
		#pragma warning(disable : 4312)
		#pragma warning(disable : 4068)
	#endif

	#ifdef EE_DYNAMIC
		// Windows platforms
		#ifdef EE_EXPORTS
			// From DLL side, we must export
			#define EE_API __declspec(dllexport)
		#else
			// From client application side, we must import
			#define EE_API __declspec(dllimport)
		#endif
	#else
		// No specific directive needed for static build
		#ifndef EE_API
		#define EE_API
		#endif
	#endif
#else
	#if ( __GNUC__ >= 4 )
		#define EE_API __attribute__ ((visibility("default")))
	#endif

	// Other platforms don't need to define anything
	#ifndef EE_API
	#define EE_API
	#endif
#endif

#ifndef EE_USE_DOUBLES
	#ifdef EE_64BIT
		#define EE_USE_DOUBLES 1
	#else
		#define EE_USE_DOUBLES 0
	#endif
#endif

#if ( defined( EE_GLES2 ) || defined( EE_GLES1 ) ) && !defined( EE_GLES )
	#define EE_GLES
#endif

#if defined( EE_GLES2 ) && defined( EE_GLES1 )
	#define EE_GLES_BOTH
#endif

#if ( !defined( EE_GLES1 ) && ( defined( EE_GLES2 ) || !defined( EE_PLATFORM_TOUCH ) ) ) || defined( EE_GLES_BOTH )
	#define EE_SHADERS_SUPPORTED
#endif

#define eeCOMMA ,
#define eeARRAY_SIZE(__array)	( sizeof(__array) / sizeof(__array[0]) )
#define eeSAFE_DELETE(p)		{ if(p) { eeDelete (p);			(p)=NULL; } }
#define eeSAFE_FREE(p)			{ if(p) { eeFree ( (void*)p );	(p)=NULL; } }
#define eeSAFE_DELETE_ARRAY(p)  { if(p) { eeDeleteArray(p);		(p)=NULL; } }
#define eeINDEX_NOT_FOUND 0xFFFFFFFF

namespace EE {
#if 1 == EE_USE_DOUBLES
	#define EE_SIZE_OF_FLOAT 8
	typedef double eeFloat;
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
	#define EE_SIZE_OF_FLOAT 4
	typedef float eeFloat; //! The internal floating point used on EE++. \n This can help to improve compatibility with some platforms. \n And helps for an easy change from single precision to double precision.
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

	template<typename T>
	T eemax( T a, T b ) {
		return (a > b) ? a : b;
	}

	template<typename T>
	T eemin( T a, T b ) {
		return (a < b) ? a : b;
	}

	template<typename T>
	T eeabs( T n ) {
		return ( n < 0 ) ? -n : n;
	}

	template<typename T>
	T eeclamp( T val, T min, T max ) {
		return ( val < min ) ? min : ( ( val > max ) ? max : val );
	}

	template<typename T>
	void eeclamp( T* val, T min, T max ) {
		( *val < min ) ? *val = min : ( ( *val > max ) ? *val = max : *val );
	}

	typedef SOPHIST_int8	Int8;
	typedef SOPHIST_uint8	Uint8;
	typedef SOPHIST_int16	Int16;
	typedef SOPHIST_uint16	Uint16;
	typedef SOPHIST_int32	Int32;
	typedef SOPHIST_uint32	Uint32;
	typedef double			eeDouble; 	//! The internal double floating point. It's only used when the engine needs some very high precision floating point ( for example the timer )
	typedef unsigned int	eeUint;
	typedef signed int		eeInt;

	#if SOPHIST_has_64
	typedef SOPHIST_uint64	Uint64;
	typedef SOPHIST_int64	Int64;
	#else
	typedef SOPHIST_uint32	Uint64;	// Fallback to a 32 bit int
	typedef SOPHIST_int32	Int64;	// All the desktop platforms support 64bit ints, so this shouldn't happen.
	#endif

	#define EE_PI			3.14159265358979323846
	#define EE_PI2			6.28318530717958647692
	const eeFloat EE_PI_180	= (eeFloat)EE_PI / 180;
	const eeFloat EE_PI_360	= (eeFloat)EE_PI / 360;
	const eeFloat EE_180_PI	= 180 / (eeFloat)EE_PI;
	const eeFloat EE_360_PI	= 360 / (eeFloat)EE_PI;

	#define EE_1B		( 1 )
	#define EE_1KB		( 1024 )
	#define EE_1MB		( 1048576 )
	#define EE_1GB		( 1073741824 )
	#define EE_1TB		( 1099511627776 )
}

#endif
