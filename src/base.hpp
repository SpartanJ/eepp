#ifndef EE_BASE_HPP
#define EE_BASE_HPP

#include <cstdlib>
#include <climits>
#include <cmath>
#include <cwchar>
#include <cstdarg>
#include <ctime>
#include <cctype>
#include <cassert>

#include <iostream>
#include <fstream>
#include <sstream>

#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <queue>
#include <list>
#include <set>
#include <stack>

#include "helper/PlusCallback/callback.hpp"
#include "helper/sophist/sophist.h"

#define EE_PLATFORM_WIN		1
#define EE_PLATFORM_LINUX	2
#define EE_PLATFORM_MACOSX	3

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( _WIN64 )
	#define EE_PLATFORM EE_PLATFORM_WIN

	#if ( defined( _MSCVER ) || defined( _MSC_VER ) )
		#define EE_COMPILER_MSVC
	#endif

#elif defined( __APPLE_CC__ ) || defined ( __APPLE__ )
	#define EE_PLATFORM EE_PLATFORM_MACOSX

#elif defined ( linux ) || defined( __linux__ )
	#define EE_PLATFORM EE_PLATFORM_LINUX
#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOSX
	#define EE_PLATFORM_UNIX
#endif

#if defined(__GNUC__)
	#define EE_COMPILER_GCC
#endif

#ifndef EE_DEBUG
	#if defined( DEBUG ) || defined( _DEBUG ) || defined( __DEBUG )
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
#endif

#if ( __GNUC__ >= 4 ) && defined( EE_DYNAMIC ) && defined( EE_EXPORTS )
	#define EE_API __attribute__ ((visibility("default")))
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
	#ifdef EE_DYNAMIC
		#define EE_CALL _stdcall

		// Windows platforms
		#ifdef EE_EXPORTS
			// From DLL side, we must export
			#define EE_API __declspec(dllexport)
		#else
			// From client application side, we must import
			#define EE_API __declspec(dllimport)
		#endif

		#ifdef EE_COMPILER_MSVC
			#pragma warning(disable : 4251)
			#pragma warning(disable : 4244)
			#pragma warning(disable : 4996)
		#endif
	#else
		// No specific directive needed for static build
		#ifndef EE_API
		#define EE_API
		#endif
		#define EE_CALL
	#endif
#else
	// Other platforms don't need to define anything
	#ifndef EE_API
	#define EE_API
	#endif
	#define EE_CALL
#endif

#ifndef EE_USE_DOUBLE
	#ifdef EE_64BIT
		#define EE_USE_DOUBLES 1
	#else
		#define EE_USE_DOUBLE 0
	#endif
#endif

/// Activate at least one backend for the compilation
#if !defined( EE_BACKEND_SDL_ACTIVE ) && !defined( EE_BACKEND_ALLEGRO_ACTIVE )
#define EE_BACKEND_SDL_ACTIVE
#endif

#define eeARRAY_SIZE(__array)	( sizeof(__array) / sizeof(__array[0]) )
#define eeSAFE_DELETE(p)		{ if(p) { eeDelete (p);			(p)=NULL; } }
#define eeSAFE_FREE(p)			{ if(p) { eeFree ( (void*)p );	(p)=NULL; } }
#define eeSAFE_DELETE_ARRAY(p)  { if(p) { eeDeleteArray(p);		(p)=NULL; } }

namespace EE {
#if 1 == EE_USE_DOUBLES
	typedef double eeFloat;
	#define eesqrt sqrt
	#define eesin sin
	#define eecos cos
	#define eeacos acos
	#define eeatan2 atan2
	#define eemod fmod
	#define eeexp exp
	#define eepow pow
	#define eefloor floor
	#define eeceil ceil
	#define eeabs abs
#else
	typedef float eeFloat; //! The internal floating point used on EE++. \n This can help to improve compatibility with some platforms. \n And helps for an easy change from single precision to double precision.
	#define eesqrt sqrtf
	#define eesin sinf
	#define eecos cosf
	#define eeacos acosf
	#define eeatan2 atan2f
	#define eemod fmodf
	#define eeexp expf
	#define eepow powf
	#define eefloor floorf
	#define eeceil ceilf
	#define eeabs fabs
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
		if ( val < min ) {
			return min;
		} else if ( val > max ) {
			return max;
		}

		return val;
	}

	template<typename T>
	void eeclamp( T* val, T min, T max ) {
		if ( *val < min ) {
			*val = min;
		} else if ( *val > max ) {
			*val = max;
		}
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

	const eeFloat PI		= 3.14159265358979323846;
	const eeFloat TwoPI		= 6.28318530717958647692;
	const eeFloat PId180	= PI / 180;
	const eeFloat d180PI	= 180 / PI;
}

#include "base/base.hpp"

#endif
