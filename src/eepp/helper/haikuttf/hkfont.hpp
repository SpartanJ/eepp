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

		void				face( FT_Face face );
		FT_Face				face();

		void				height( int height );
		int					height();

		void				ascent( int ascent );
		int					ascent();

		void				descent( int descent );
		int					descent();

		void				lineSkip( int lineskip );
		int					lineSkip();

		void				faceStyle( int facestyle );
		int					faceStyle();

		void				kerning( int kerning );
		int					kerning();

		void				glyphOverhang( int glyphoverhang );
		int					glyphOverhang();

		void				glyphItalics( float glyphitalics );
		float				glyphItalics();

		void				underlineOffset( int underlineoffset );
		int					underlineOffset();

		void				underlineHeight( int underlineheight );
		int					underlineHeight();

		void				current( hkGlyph * current );
		hkGlyph *			current();

		void				scratch( hkGlyph scratch );
		hkGlyph				scratch();

		void				fontSizeFamily( int fontsizefamily );
		int					fontSizeFamily();

		void 				outline( int outline );
		int					outline() const;

		void 				hinting( int hinting );
		int					hinting() const;

		void 				style( int style );
		int					style();

		FT_Error 			findGlyph( u32 ch, int want );

		FT_Error 			loadGlyph( u32 ch, hkGlyph * cached, int want );

		unsigned char * 	renderGlyph( u32 ch, u32 fg = 0x00000000 );

		int 				getGlyphMetrics( u32 ch, int* minx, int* maxx, int* miny, int* maxy, int* advance );

		void 				cacheFlush();
	protected:
		friend class hkFontManager;

		hkFontManager * 	mFm;
		hkGlyph * 			mCache;
		unsigned int 		mCacheSize;

		FT_Face				mFace;
		int					mHeight;
		int					mAscent;
		int					mDescent;
		int					mLineSkip;
		int					mFaceStyle;
		int					mKerning;
		int					mGlyphOverhang;
		float				mGlyphItalics;
		int					mUnderlineOffset;
		int					mUnderlineHeight;
		hkGlyph * 			mCurrent;
		hkGlyph				mScratch;
		int 				mFontSizeFamily;
		int 				mStyle;
		int 				mOutline;
		int 				mHinting;

		int 				underlineTopRow();

		int 				strikeThroughTopRow();

		void 				drawLine( const unsigned char * textbuf, const int row, const u32 color, FT_Bitmap * bitmap );

		void 				initLineMectrics( const unsigned char * textbuf, const int row, u8 **pdst, int *pheight, FT_Bitmap * bitmap );
};

}

#endif
