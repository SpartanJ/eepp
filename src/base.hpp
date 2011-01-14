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

#include "helper/PlusCallback/callback.hpp"

#include <SDL/SDL.h>

#define Int8 Sint8
#define Int16 Sint16
#define Int32 Sint32
#define Int64 Sint64

#define EE_PLATFORM_WIN		1
#define EE_PLATFORM_LINUX	2
#define EE_PLATFORM_MACOSX	3

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( _WIN64 )
#   define EE_PLATFORM EE_PLATFORM_WIN

	#if (defined (_MSCVER) || defined (_MSC_VER))
#	define EE_COMPILER_MSVC
	#endif

#elif defined( __APPLE_CC__) || defined ( __APPLE__ )
#   define EE_PLATFORM EE_PLATFORM_MACOSX

#elif defined ( linux ) || defined( __linux__ )
#   define EE_PLATFORM EE_PLATFORM_LINUX
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

#if defined(__x86_64__) || \
	(defined(_WIN64) && !defined(_XBOX)) || \
	defined(__64BIT__) || defined(__LP64)  || defined(__LP64__) || defined(_LP64) || defined(_ADDR64) || defined(_CRAYC) || defined(__arch64__) || \
	defined(__sparcv9) || defined(__sparc_v9__)
#define EE_64BIT
#else
#define EE_32BIT
#endif


#define EE_LITTLE_ENDIAN 1
#define EE_BIG_ENDIAN 2

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
	|| defined(__X86)   || defined(_M_IX86)                       \
	|| defined(_M_X64)  || defined(__x86_64__)                    \
	|| defined(alpha)   || defined(__alpha) || defined(__alpha__) \
	|| defined(_M_ALPHA)                                          \
	|| defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
	|| defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
	|| defined(_WIN32_WCE) || defined(__NT__)                     \
	|| defined(__MIPSEL__)
	#define EE_ENDIAN  EE_LITTLE_ENDIAN
#else
	#define EE_ENDIAN  EE_BIG_ENDIAN
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_LINUX
#define EE_GLEW_AVAILABLE
#else
#warning Platform not supported.
#endif

#ifdef EE_PLATFORM
#define EE_SUPPORTED_PLATFORM
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
	#define EE_CALL _stdcall

	#ifdef EE_DYNAMIC
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
		#define EE_API
	#endif
#else
	// Other platforms don't need to define anything
	#define EE_API
	#define EE_CALL
#endif

#if ( __GNUC__ >= 4 ) && defined( EE_DYNAMIC ) && defined( EE_EXPORTS )
#define EE_API __attribute__ ((visibility("default")))
#else
#define EE_API
#endif

#ifdef EE_GLEW_AVAILABLE
#include "helper/glew/glew.h"
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#define eeARRAY_SIZE(__array)	( sizeof(__array) / sizeof(__array[0]) )
#define eeSAFE_DELETE(p)		{ if(p) { eeDelete (p);			(p)=NULL; } }
#define eeSAFE_FREE(p)			{ if(p) { eeFree ( (void*)p );	(p)=NULL; } }
#define eeSAFE_DELETE_ARRAY(p)  { if(p) { eeDeleteArray(p);		(p)=NULL; } }

#ifdef EE_64BIT
#define EE_USE_DOUBLES 1
#else
#define EE_USE_DOUBLE 0
#endif

namespace EE {
#if EE_USE_DOUBLES
	typedef double eeFloat;
	#define eesqrt sqrt
	#define eesin sin
	#define eecos cos
	#define eeacos acos
	#define eeatan2 atan2
	#define eemod fmod
	#define eeexp exp
	#define efow pow
	#define eefloor floor
	#define eeceil ceil
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
#endif

	typedef double			eeDouble; 	//! The internal double floating point. It's only used when the engine needs some very high precision floating point ( for example the timer )
	typedef unsigned int	eeUint;
	typedef signed int		eeInt;

	const eeFloat PI		= 3.141592654f;
	const eeFloat TwoPI		= 6.283185308f;
	const eeFloat PId180	= PI / 180.f;
	const eeFloat d180PI	= 180.f / PI;
}

#include "base/base.hpp"

#endif
