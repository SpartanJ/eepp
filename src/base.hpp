#ifndef EE_BASE_HPP
#define EE_BASE_HPP

#include <cstdlib>
#include <climits>
#include <cmath>
#include <cwchar>
#include <cstdarg>
#include <ctime>

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <queue>
#include <list>
#include <memory>
#include <algorithm>
#include <cctype>

#include <iostream>
#include <fstream>
#include <sstream>

#include <SDL/SDL.h>

// Templates needed by the library
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#define Int8 Sint8
#define Int16 Sint16
#define Int32 Sint32
#define Int64 Sint64

#define EE_PLATFORM_WIN32 1
#define EE_PLATFORM_LINUX 2
#define EE_PLATFORM_APPLE 3

#if defined( __WIN32__ ) || defined( _WIN32 )
#   define EE_PLATFORM EE_PLATFORM_WIN32

	#if (defined (_MSCVER) || defined (_MSC_VER))
#	define EE_COMPILER_MSVC
	#endif

#elif defined( __APPLE_CC__) || defined ( __APPLE__ )
#   define EE_PLATFORM EE_PLATFORM_APPLE

#else
#   define EE_PLATFORM EE_PLATFORM_LINUX
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN32
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

		#ifdef _MSC_VER
			#pragma warning(disable : 4251)
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

#include "helper/glew/glew.h"
#if EE_PLATFORM == EE_PLATFORM_APPLE
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace EE {
	#define EE_CLOCKS_PER_SEC 1000000l

	#define eeARRAY_SIZE(__array)	( sizeof(__array) / sizeof(__array[0]) )
	#define eeSAFE_DELETE(p)		{ if(p) { delete (p);		(p)=NULL; } }
	#define eeSAFE_FREE(p)			{ if(p) { free ( (void*)p );(p)=NULL; } }
	#define eeSAFE_DELETE_ARRAY(p)  { if(p) { delete[](p);		(p)=NULL; } }

	typedef float				eeFloat; 	//! The internal floating point used on EE++. \n This can help to improve compatibility with some platforms. \n And helps for an easy change from single precision to double precision.
	typedef double			eeDouble; 	//! The internal double floating point. It's only used when the engine needs some very high precision floating point ( for example the timer )
	typedef unsigned int		eeUint;
	typedef signed int		eeInt;

	const eeFloat PI			= 3.141592654f;
	const eeFloat TwoPI		= 6.283185308f;
	const eeFloat PId180		= PI / 180.f;
	const eeFloat d180PI		= 180.f / PI;

	using namespace std;
}

#endif
