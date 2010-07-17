#ifndef HAIKUTTF_HKFONT_HPP
#define HAIKUTTF_HKFONT_HPP

#include "hkbase.hpp"
#include "hkglyph.hpp"

namespace HaikuTTF {

class hkFontManager;

class hkFont {
	public:
		hkFont( hkFontManager * FontManager, unsigned int CacheSize = 256 );

		~hkFont();

		void CacheFlush();

		HK_GETSET( FT_Face, Face );

		HK_GETSET( int, Height );
		HK_GETSET( int, Ascent );
		HK_GETSET( int, Descent );
		HK_GETSET( int, LineSkip );
		HK_GETSET( int, FaceStyle );

		HK_GETSET( int, Kerning );

		HK_GETSET( int, GlyphOverhang );
		HK_GETSET( float, GlyphItalics );

		HK_GETSET( int, UnderlineOffset );
		HK_GETSET( int, UnderlineHeight );

		HK_GETSET( hkGlyph*, Current );

		HK_GETSET( hkGlyph, Scratch );

		HK_GETSET( int, FontSizeFamily );

		void 				Outline( int outline );
		int					Outline() const 		{ return mOutline; }

		void 				Hinting( int hinting );
		int					Hinting() const 		{ return mHinting; }

		void 				Style( int style );
		int 				Style() 				{return mStyle; }

		hkFontManager * 	Manager() const 		{ return mFm; }

		FT_Error 			GlyphFind( uint16_t ch, int want );

		FT_Error 			GlyphLoad( uint16_t ch, hkGlyph * cached, int want );

		unsigned char * 	GlyphRender( uint16_t ch, uint32_t fg );

		int 				GlyphMetrics( uint16_t ch, int* minx, int* maxx, int* miny, int* maxy, int* advance );
	protected:
		hkFontManager * mFm;

		hkGlyph * mCache;

		unsigned int mCacheSize;

		int mStyle;
		int mOutline;
		int mHinting;

		int UnderlineTopRow();
		int StrikeThroughTopRow();
		void DrawLine( const unsigned char * textbuf, const int row, const uint32_t color, FT_Bitmap * bitmap );
		void InitLineMectrics( const unsigned char * textbuf, const int row, uint8_t **pdst, int *pheight, FT_Bitmap * bitmap );
};

}

#endif
