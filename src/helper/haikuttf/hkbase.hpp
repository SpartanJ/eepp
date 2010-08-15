#ifndef HAIKUTTF_HKBASE_HPP
#define HAIKUTTF_HKBASE_HPP

#include <cmath>

#include "sophist.h"
typedef SOPHIST_int8		s8;
typedef SOPHIST_uint8		u8;
typedef SOPHIST_int16		s16;
typedef SOPHIST_uint16		u16;
typedef SOPHIST_int32		s32;
typedef SOPHIST_uint32		u32;
#ifdef SOPHIST_has_64
typedef SOPHIST_int64		s64;
typedef SOPHIST_uint64		u64;
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

/* Handle a style only if the font does not already handle it */
#define TTF_HANDLE_STYLE_BOLD(font) 			( ( (font)->Style() & TTF_STYLE_BOLD ) && !( (font)->FaceStyle() & TTF_STYLE_BOLD ) )
#define TTF_HANDLE_STYLE_ITALIC(font) 			( ( (font)->Style() & TTF_STYLE_ITALIC ) && !( (font)->FaceStyle() & TTF_STYLE_ITALIC ) )
#define TTF_HANDLE_STYLE_UNDERLINE(font) 		( (font)->Style() & TTF_STYLE_UNDERLINE )
#define TTF_HANDLE_STYLE_STRIKETHROUGH(font) 	( (font)->Style() & TTF_STYLE_STRIKETHROUGH )

/* Font styles that does not impact glyph drawing */
#define TTF_STYLE_NO_GLYPH_CHANGE	(TTF_STYLE_UNDERLINE | TTF_STYLE_STRIKETHROUGH)

#define hkARRAY_SIZE(__array)	( sizeof(__array) / sizeof(__array[0]) )
#define hkSAFE_DELETE(p)		{ if(p) { delete (p);		(p)=NULL; } }
#define hkSAFE_FREE(p)			{ if(p) { free (p);			(p)=NULL; } }
#define hkSAFE_DELETE_ARRAY(p)  { if(p) { delete[](p);		(p)=NULL; } }

#define TTF_STYLE_NORMAL	0x00
#define TTF_STYLE_BOLD		0x01
#define TTF_STYLE_ITALIC	0x02
#define TTF_STYLE_UNDERLINE	0x04
#define TTF_STYLE_STRIKETHROUGH	0x08

#define TTF_HINTING_NORMAL    0
#define TTF_HINTING_LIGHT     1
#define TTF_HINTING_MONO      2
#define TTF_HINTING_NONE      3
#endif
