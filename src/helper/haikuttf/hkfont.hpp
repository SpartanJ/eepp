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

		inline void		Face( FT_Face face )						{ mFace = face; 						}
		inline FT_Face		Face()										{ return mFace; 						}

		inline void		Height( int height )						{ mHeight = height; 					}
		inline int			Height()									{ return mHeight; 						}

		inline void		Ascent( int ascent )						{ mAscent = ascent; 					}
		inline int			Ascent()									{ return mAscent; 						}

		inline void		Descent( int descent )						{ mDescent = descent; 					}
		inline int			Descent()									{ return mDescent; 					}

		inline void		LineSkip( int lineskip )					{ mLineSkip = lineskip; 				}
		inline int			LineSkip()									{ return mLineSkip; 					}

		inline void		FaceStyle( int facestyle )					{ mFaceStyle = facestyle; 				}
		inline int			FaceStyle()									{ return mFaceStyle; 					}

		inline void		Kerning( int kerning )						{ mKerning = kerning; 					}
		inline int			Kerning()									{ return mKerning; 					}

		inline void		GlyphOverhang( int glyphoverhang )			{ mGlyphOverhang = glyphoverhang; 		}
		inline int			GlyphOverhang()								{ return mGlyphOverhang; 				}

		inline void		GlyphItalics( float glyphitalics )			{ mGlyphItalics = glyphitalics; 		}
		inline float		GlyphItalics()								{ return mGlyphItalics; 				}

		inline void		UnderlineOffset( int underlineoffset )		{ mUnderlineOffset = underlineoffset;	}
		inline int			UnderlineOffset()							{ return mUnderlineOffset; 			}

		inline void		UnderlineHeight( int underlineheight )		{ mUnderlineHeight = underlineheight;	}
		inline int			UnderlineHeight()							{ return mUnderlineHeight; 			}

		inline void		Current( hkGlyph * current )				{ mCurrent = current; 					}
		inline hkGlyph * 	Current()									{ return mCurrent; 					}

		inline void		Scratch( hkGlyph scratch )					{ mScratch = scratch; 					}
		inline hkGlyph		Scratch()									{ return mScratch; 					}

		inline void		FontSizeFamily( int fontsizefamily )		{ mFontSizeFamily = fontsizefamily; 	}
		inline int			FontSizeFamily()							{ return mFontSizeFamily; 				}

		void 				Outline( int outline );
		inline int			Outline() const 							{ return mOutline; 					}

		void 				Hinting( int hinting );
		inline int			Hinting() const 							{ return mHinting; 					}

		void 				Style( int style );
		inline int 		Style() 									{ return mStyle; 						}

		hkFontManager * 	Manager() const 							{ return mFm; 							}

		FT_Error 			GlyphFind( uint16_t ch, int want );

		FT_Error 			GlyphLoad( uint16_t ch, hkGlyph * cached, int want );

		unsigned char * 	GlyphRender( uint16_t ch, uint32_t fg );

		int 				GlyphMetrics( uint16_t ch, int* minx, int* maxx, int* miny, int* maxy, int* advance );

		void 				CacheFlush();
	protected:
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

		void 				DrawLine( const unsigned char * textbuf, const int row, const uint32_t color, FT_Bitmap * bitmap );

		void 				InitLineMectrics( const unsigned char * textbuf, const int row, uint8_t **pdst, int *pheight, FT_Bitmap * bitmap );
};

}

#endif
