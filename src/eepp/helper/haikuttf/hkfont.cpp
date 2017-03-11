#include "hkfont.hpp"
#include "hkfontmanager.hpp"
#include <cmath>

namespace HaikuTTF {

hkFont::hkFont( hkFontManager * FontManager, unsigned int CacheSize ) :
	mFm( FontManager ),
	mCache(NULL),
	mFace(NULL),
	mHeight(0),
	mAscent(0),
	mDescent(0),
	mLineSkip(0),
	mFaceStyle(0),
	mKerning(0),
	mGlyphOverhang(0),
	mGlyphItalics(0),
	mUnderlineOffset(0),
	mUnderlineHeight(0),
	mCurrent(NULL),
	mFontSizeFamily(0),
	mStyle(0),
	mOutline(0),
	mHinting(0)
{
	mCacheSize	= CacheSize;
	mCache		= hkNewArray( hkGlyph, mCacheSize );
}

hkFont::~hkFont() {
	cacheFlush();

	if ( NULL != mFace ) {
		FT_Done_Face( mFace );
		mFace = NULL;
	}

	hkSAFE_DELETE_ARRAY( mCache );
}

void hkFont::outline( int outline ) {
	mOutline = outline;
	cacheFlush();
}

void hkFont::hinting( int hinting ) {
	if ( hinting == HK_TTF_HINTING_LIGHT )
		mHinting = FT_LOAD_TARGET_LIGHT;
	else if ( hinting == HK_TTF_HINTING_MONO )
		mHinting= FT_LOAD_TARGET_MONO;
	else if ( hinting == HK_TTF_HINTING_NONE )
		mHinting = FT_LOAD_NO_HINTING;
	else
		mHinting = 0;

	cacheFlush();
}

void hkFont::style( int style ) {
	int prev_style = mFaceStyle;
	mStyle = style | mFaceStyle;

	if ( ( mStyle | HK_TTF_STYLE_NO_GLYPH_CHANGE ) != ( prev_style | HK_TTF_STYLE_NO_GLYPH_CHANGE ) )
		cacheFlush();
}

int hkFont::underlineTopRow() {
	return mAscent - mUnderlineOffset - 1;
}

int hkFont::strikeThroughTopRow() {
	return mHeight / 2;
}

void hkFont::cacheFlush() {
	for( int i = 0; i < (int)mCacheSize; ++i ) {
		if( mCache[i].cached() )
			mCache[i].flush();
	}

	if( mScratch.cached() )
		mScratch.flush();
}

FT_Error hkFont::findGlyph(u32 ch, int want ) {
	int retval = 0;

	if( ch < mCacheSize ) {
		mCurrent = &mCache[ch];
	} else {
		if ( mScratch.cached() != ch )
			mScratch.flush();

		mCurrent = &mScratch;
	}

	if ( ( mCurrent->stored() & want) != want )
		retval = loadGlyph( ch, mCurrent, want );

	return retval;
}

FT_Error hkFont::loadGlyph(u32 ch, hkGlyph * cached, int want ) {
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( NULL == mFace )
		return FT_Err_Invalid_Handle;

	face = mFace;

	mFm->mutexLock();

	if ( !cached->index() )
		cached->index( FT_Get_Char_Index( face, ch ) );

	error = FT_Load_Glyph( face, cached->index(), FT_LOAD_DEFAULT | mHinting );

	if( error )
		return error;

	glyph = face->glyph;
	metrics = &glyph->metrics;
	outline = &glyph->outline;

	if ( ( want & CACHED_METRICS ) && !( cached->stored() & CACHED_METRICS ) ) {
		if ( FT_IS_SCALABLE( face ) ) {
			cached->minX( FT_FLOOR( metrics->horiBearingX ) );
			cached->maxX( cached->minX() + FT_CEIL( metrics->width ) );
			cached->maxY( FT_FLOOR( metrics->horiBearingY ) );
			cached->minY( cached->maxY() - FT_CEIL( metrics->height ) );
			cached->offsetY( mAscent - cached->maxY() );
			cached->advance( FT_CEIL( metrics->horiAdvance ) );
		} else {
			cached->minX( FT_FLOOR( metrics->horiBearingX ) );
			cached->maxX( cached->minX() + FT_CEIL( metrics->horiAdvance ) );
			cached->maxY( FT_FLOOR( metrics->horiBearingY ) );
			cached->minY( cached->maxY() - FT_CEIL( face->available_sizes[ mFontSizeFamily ].height ) );
			cached->offsetY( 0 );
			cached->advance( FT_CEIL( metrics->horiAdvance ) );
		}

		if( HK_TTF_HANDLE_STYLE_BOLD(this) )
			cached->maxX( cached->maxX() + mGlyphOverhang );

		if( HK_TTF_HANDLE_STYLE_ITALIC(this) )
			cached->maxX( cached->maxX() + (int)ceil( mGlyphItalics ) );

		cached->stored( cached->stored() | CACHED_METRICS );
	}

	if ( ( want & CACHED_PIXMAP ) && !( cached->stored() & CACHED_PIXMAP) ) {
		int mono = ( want & CACHED_BITMAP );
		int i;
		FT_Bitmap* src;
		FT_Bitmap* dst;
		FT_Glyph bitmap_glyph = NULL;

		if( HK_TTF_HANDLE_STYLE_ITALIC(this) ) {
			FT_Matrix shear;

			shear.xx = 1 << 16;
			shear.xy = (int) ( mGlyphItalics * ( 1 << 16 ) ) / mHeight;
			shear.yx = 0;
			shear.yy = 1 << 16;

			FT_Outline_Transform( outline, &shear );
		}

		if( ( mOutline > 0 ) && glyph->format != FT_GLYPH_FORMAT_BITMAP ) {
			FT_Stroker stroker;
			FT_Get_Glyph( glyph, &bitmap_glyph );

			error = FT_Stroker_New( mFm->getLibrary(), &stroker );

			if( error )
				return error;

			FT_Stroker_Set( stroker, mOutline * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0 );
			FT_Glyph_Stroke( &bitmap_glyph, stroker, 1 );
			FT_Stroker_Done( stroker );

			error = FT_Glyph_To_Bitmap( &bitmap_glyph, mono ? ft_render_mode_mono : ft_render_mode_normal, 0, 1 );

			if( error ) {
				FT_Done_Glyph( bitmap_glyph );
				return error;
			}

			src = &((FT_BitmapGlyph)bitmap_glyph)->bitmap;
		} else {
			error = FT_Render_Glyph( glyph, mono ? ft_render_mode_mono : ft_render_mode_normal );

			if( error )
				return error;

			src = &glyph->bitmap;
		}

		dst = cached->pixmap();

		memcpy( dst, src, sizeof( *dst ) );

		if( HK_TTF_HANDLE_STYLE_BOLD(this) ) {
			int bump = mGlyphOverhang;
			dst->pitch += bump;
			dst->width += bump;
		}

		if( HK_TTF_HANDLE_STYLE_ITALIC(this) ) {
			int bump = (int)ceil( mGlyphItalics );
			dst->pitch += bump;
			dst->width += bump;
		}

		if (dst->rows != 0) {
			dst->buffer = (unsigned char*)hkMalloc( dst->pitch * dst->rows );

			if( !dst->buffer ) {
				return FT_Err_Out_Of_Memory;
			}
			memset( dst->buffer, 0, dst->pitch * dst->rows );

			for( i = 0; i < src->rows; i++ ) {
				int soffset = i * src->pitch;
				int doffset = i * dst->pitch;

				memcpy( dst->buffer+doffset, src->buffer+soffset, src->pitch );
			}
		}

		if ( HK_TTF_HANDLE_STYLE_BOLD(this) ) {
			int row;
			int col;
			int offset;
			int pixel;
			u8* pixmap;

			for( row = dst->rows - 1; row >= 0; --row ) {
				pixmap = (u8*) dst->buffer + row * dst->pitch;

				for( offset=1; offset <= mGlyphOverhang; ++offset ) {
					for( col = dst->width - 1; col > 0; --col ) {
						pixel = (pixmap[col] + pixmap[col-1]);

						if( pixel > NUM_GRAYS - 1 ) {
							pixel = NUM_GRAYS - 1;
						}

						pixmap[col] = (u8) pixel;
					}
				}
			}
		}

		cached->stored( cached->stored() | CACHED_PIXMAP );

		if( bitmap_glyph ) {
			FT_Done_Glyph( bitmap_glyph );
		}
	}

	mFm->mutexUnlock();

	cached->cached( ch );

	return 0;
}

unsigned char * hkFont::renderGlyph(u32 ch, u32 fg ) {
	unsigned char * textbuf = NULL;
	int row;
	FT_Error error;
	hkGlyph *glyph;
	FT_Bitmap * bitmap;

	error = findGlyph( ch, CACHED_METRICS | CACHED_PIXMAP );

	if ( error )
		return(NULL);

	glyph = mCurrent;
	bitmap = glyph->pixmap();

	textbuf = hkNewArray( unsigned char, bitmap->width * bitmap->rows * 4 );

	if ( NULL == textbuf )
		return NULL;

	u32 * buffu32 = reinterpret_cast<u32*> ( &textbuf[0] );

	memset( buffu32, fg, bitmap->width * bitmap->rows * 4 );

	const u8* pixels = bitmap->buffer;
	u32 index;

	for ( int y = 0; y < bitmap->rows; y++ ) {
		for ( int x = 0; x < bitmap->width; x++ ) {
			index = (x + y * bitmap->width) * 4 + 3;

			textbuf[ index ] = pixels[ x ];
		}

		pixels += bitmap->pitch;
	}

	if( HK_TTF_HANDLE_STYLE_UNDERLINE(this) ) {
		row = underlineTopRow();
		drawLine( textbuf, row, fg, bitmap );
	}

	if( HK_TTF_HANDLE_STYLE_STRIKETHROUGH(this) ) {
		row = strikeThroughTopRow();
		drawLine( textbuf, row, fg, bitmap );
	}

	return textbuf;
}

int hkFont::getGlyphMetrics(u32 ch, int* minx, int* maxx, int* miny, int* maxy, int* advance ) {
	FT_Error error;

	error = findGlyph( ch, CACHED_METRICS );

	if ( error )
		return -1;

	if ( NULL != minx )
		*minx = mCurrent->minX() - mOutline;

	if ( NULL !=maxx ) {
		*maxx = mCurrent->maxX() + mOutline;

		if( HK_TTF_HANDLE_STYLE_BOLD(this) )
			*maxx += mGlyphOverhang;
	}

	if ( NULL !=miny )
		*miny = mCurrent->minY() - mOutline;

	if ( NULL !=maxy )
		*maxy = mCurrent->maxY() + mOutline;

	if ( NULL != advance ) {
		*advance = mCurrent->advance() + mOutline;

		if( HK_TTF_HANDLE_STYLE_BOLD(this) )
			*advance += mGlyphOverhang;
	}

	return 0;
}

void hkFont::initLineMectrics( const unsigned char * textbuf, const int row, u8 **pdst, int *pheight, FT_Bitmap * bitmap ) {
	u8 *dst;
	int height;

	dst = (u8 *)textbuf;

	if( row > 0 )
		dst += row * bitmap->pitch;

	height = mUnderlineHeight;

	if( mOutline > 0 )
		height += mOutline * 2;

	*pdst = dst;
	*pheight = height;
}

void hkFont::drawLine( const unsigned char * textbuf, const int row, const u32 color, FT_Bitmap * bitmap ) {
	int line;
	u32 * dst_check = (u32*)textbuf + bitmap->pitch / 4 * bitmap->rows;
	u8 * dst8; /* destination, byte version */
	u32 * dst;
	int height;
	int col;

	u32 pixel = color | 0xFF000000;

	initLineMectrics( textbuf, row, &dst8, &height, bitmap );
	dst = (u32 *) dst8;

	for ( line = height; line > 0 && dst < dst_check; --line ) {
		for ( col = 0; col < bitmap->width; ++col )
			dst[col] = pixel;

		dst += bitmap->pitch / 4;
	}
}

void hkFont::face( FT_Face face ) {
	mFace = face;
}

FT_Face	hkFont::face() {
	return mFace;
}

void hkFont::height( int height ) {
	mHeight = height;
}

int	hkFont::height() {
	return mHeight + mOutline;
}

void hkFont::ascent( int ascent ) {
	mAscent = ascent;
}

int hkFont::ascent() {
	return mAscent;
}

void hkFont::descent( int descent ) {
	mDescent = descent;
}

int hkFont::descent() {
	return mDescent;
}

void hkFont::lineSkip( int lineskip ) {
	mLineSkip = lineskip;
}

int	hkFont::lineSkip() {
	return mLineSkip;
}

void hkFont::faceStyle( int facestyle ) {
	mFaceStyle = facestyle;
}

int hkFont::faceStyle()	{
	return mFaceStyle;
}

void hkFont::kerning( int kerning )	{
	mKerning = kerning;
}

int hkFont::kerning() {
	return mKerning;
}

void hkFont::glyphOverhang( int glyphoverhang )	{
	mGlyphOverhang = glyphoverhang;
}

int hkFont::glyphOverhang()	{
	return mGlyphOverhang;
}

void hkFont::glyphItalics( float glyphitalics ) {
	mGlyphItalics = glyphitalics;
}

float hkFont::glyphItalics() {
	return mGlyphItalics;
}

void hkFont::underlineOffset( int underlineoffset ) {
	mUnderlineOffset = underlineoffset;
}

int hkFont::underlineOffset() {
	return mUnderlineOffset;
}

void hkFont::underlineHeight( int underlineheight )	{
	mUnderlineHeight = underlineheight;
}

int hkFont::underlineHeight() {
	return mUnderlineHeight;
}

void hkFont::current( hkGlyph * current ) {
	mCurrent = current;
}

hkGlyph * hkFont::current()	{
	return mCurrent;
}

void hkFont::scratch( hkGlyph scratch ) {
	mScratch = scratch;
}

hkGlyph	hkFont::scratch() {
	return mScratch;
}

void hkFont::fontSizeFamily( int fontsizefamily ) {
	mFontSizeFamily = fontsizefamily;
}

int	hkFont::fontSizeFamily() {
	return mFontSizeFamily;
}

int	hkFont::outline() const {
	return mOutline;
}

int	hkFont::hinting() const {
	return mHinting;
}

int	hkFont::style() {
	return mStyle;
}

}
