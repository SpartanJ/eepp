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

		void				Face( FT_Face face );
		FT_Face				Face();

		void				Height( int height );
		int					Height();

		void				Ascent( int ascent );
		int					Ascent();

		void				Descent( int descent );
		int					Descent();

		void				LineSkip( int lineskip );
		int					LineSkip();

		void				FaceStyle( int facestyle );
		int					FaceStyle();

		void				Kerning( int kerning );
		int					Kerning();

		void				GlyphOverhang( int glyphoverhang );
		int					GlyphOverhang();

		void				GlyphItalics( float glyphitalics );
		float				GlyphItalics();

		void				UnderlineOffset( int underlineoffset );
		int					UnderlineOffset();

		void				UnderlineHeight( int underlineheight );
		int					UnderlineHeight();

		void				Current( hkGlyph * current );
		hkGlyph *			Current();

		void				Scratch( hkGlyph scratch );
		hkGlyph				Scratch();

		void				FontSizeFamily( int fontsizefamily );
		int					FontSizeFamily();

		void 				Outline( int outline );
		int					Outline() const;

		void 				Hinting( int hinting );
		int					Hinting() const;

		void 				Style( int style );
		int					Style();

		hkFontManager * 	Manager() const;

		FT_Error 			GlyphFind( u16 ch, int want );

		FT_Error 			GlyphLoad( u16 ch, hkGlyph * cached, int want );

		unsigned char * 	GlyphRender( u16 ch, u32 fg = 0x00000000 );

		int 				GlyphMetrics( u16 ch, int* minx, int* maxx, int* miny, int* maxy, int* advance );

		void 				CacheFlush();
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

		int 				UnderlineTopRow();

		int 				StrikeThroughTopRow();

		void 				DrawLine( const unsigned char * textbuf, const int row, const u32 color, FT_Bitmap * bitmap );

		void 				InitLineMectrics( const unsigned char * textbuf, const int row, u8 **pdst, int *pheight, FT_Bitmap * bitmap );
};

}

#endif
