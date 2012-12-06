#ifndef HAIKUTTF_HKBASE_HPP
#define HAIKUTTF_HKBASE_HPP

#include <cmath>

#include "sophist.h"

namespace HaikuTTF {
typedef SOPHIST_int8		s8;
typedef SOPHIST_uint8		u8;
typedef SOPHIST_int16		s16;
typedef SOPHIST_uint16		u16;
typedef SOPHIST_int32		s32;
typedef SOPHIST_uint32		u32;
}

#define HK_PLATFORM_WIN 	(1)
#define HK_PLATFORM_LINUX 	(2)
#define HK_PLATFORM_MACOSX 	(3)
#define HK_PLATFORM_BSD		(4)
#define HK_PLATFORM_SOLARIS	(5)
#define HK_PLATFORM_HAIKU	(6)

#if defined( __WIN32__ ) || defined( _WIN32 ) || defined( _WIN64 )
	#define HK_PLATFORM HK_PLATFORM_WIN
#elif defined( __APPLE_CC__ ) || defined ( __APPLE__ )
	#define HK_PLATFORM HK_PLATFORM_MACOSX
#elif defined ( linux ) || defined( __linux__ )
	#define HK_PLATFORM HK_PLATFORM_LINUX
#elif defined( __FreeBSD__ ) || defined(__OpenBSD__) || defined( __NetBSD__ ) || defined( __DragonFly__ )
	#define HK_PLATFORM HK_PLATFORM_BSD
#elif defined( __SVR4 )
	#define HK_PLATFORM HK_PLATFORM_SOLARIS
#elif defined( __HAIKU__ ) || defined( __BEOS__ )
	#define HK_PLATFORM HK_PLATFORM_HAIKU
#endif

#if defined ( linux ) || defined( __linux__ ) || defined( __FreeBSD__ ) || defined(__OpenBSD__) || defined( __NetBSD__ ) || defined( __DragonFly__ ) || defined( __SVR4 ) || defined( __APPLE_CC__ ) || defined ( __APPLE__ ) || defined( __HAIKU__ ) || defined( __BEOS__ )
	#define HK_PLATFORM_POSIX
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

#define NUM_GRAYS       256

#define FT_FLOOR(X)		( ( X & -64 ) / 64 )
#define FT_CEIL(X)		( ( ( X + 63) & -64 ) / 64 )

#define CACHED_METRICS	0x10
#define CACHED_BITMAP	0x01
#define CACHED_PIXMAP	0x02

#define HK_TTF_STYLE_NORMAL			(0x00)
#define HK_TTF_STYLE_BOLD			(0x01)
#define HK_TTF_STYLE_ITALIC			(0x02)
#define HK_TTF_STYLE_UNDERLINE		(0x04)
#define HK_TTF_STYLE_STRIKETHROUGH	(0x08)

#define HK_TTF_HINTING_NORMAL    	(0)
#define HK_TTF_HINTING_LIGHT     	(1)
#define HK_TTF_HINTING_MONO      	(2)
#define HK_TTF_HINTING_NONE      	(3)

#define HK_TTF_HANDLE_STYLE_BOLD(font) 				( ( (font)->Style() & HK_TTF_STYLE_BOLD ) && !( (font)->FaceStyle() & HK_TTF_STYLE_BOLD ) )
#define HK_TTF_HANDLE_STYLE_ITALIC(font) 			( ( (font)->Style() & HK_TTF_STYLE_ITALIC ) && !( (font)->FaceStyle() & HK_TTF_STYLE_ITALIC ) )
#define HK_TTF_HANDLE_STYLE_UNDERLINE(font) 		( (font)->Style() & HK_TTF_STYLE_UNDERLINE )
#define HK_TTF_HANDLE_STYLE_STRIKETHROUGH(font) 	( (font)->Style() & HK_TTF_STYLE_STRIKETHROUGH )

#define HK_TTF_STYLE_NO_GLYPH_CHANGE				( HK_TTF_STYLE_UNDERLINE | HK_TTF_STYLE_STRIKETHROUGH )

#ifndef hkNew
#define hkNew( classType, constructor ) new classType constructor
#endif

#ifndef hkNewArray
#define hkNewArray( classType, amount ) new classType [ amount ]
#endif

#ifndef hkMalloc
#define hkMalloc( amount ) malloc( amount )
#endif

#ifndef hkDelete
#define hkDelete( data ) delete data;
#endif

#ifndef hkDeleteArray
#define hkDeleteArray( data ) delete [] data;
#endif

#ifndef hkFree
#define hkFree( data ) free(data);
#endif

#define hkARRAY_SIZE(__array)	( sizeof(__array) / sizeof(__array[0]) )
#define hkSAFE_DELETE(p)		{ if(p) { hkDelete (p);		(p)=NULL; } }
#define hkSAFE_FREE(p)			{ if(p) { hkFree (p);		(p)=NULL; } }
#define hkSAFE_DELETE_ARRAY(p)  { if(p) { hkDeleteArray(p);	(p)=NULL; } }

#endif
