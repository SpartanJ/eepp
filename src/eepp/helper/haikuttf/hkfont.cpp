#include "hkfont.hpp"
#include "hkfontmanager.hpp"

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
	CacheFlush();

	if ( NULL != Face() ) {
		FT_Done_Face( Face() );
		Face( NULL );
	}

	hkSAFE_DELETE_ARRAY( mCache );
}

void hkFont::Outline( int outline ) {
	mOutline = outline;
	CacheFlush();
}

void hkFont::Hinting( int hinting ) {
	if ( hinting == HK_TTF_HINTING_LIGHT )
		mHinting = FT_LOAD_TARGET_LIGHT;
	else if ( hinting == HK_TTF_HINTING_MONO )
		mHinting= FT_LOAD_TARGET_MONO;
	else if ( hinting == HK_TTF_HINTING_NONE )
		mHinting = FT_LOAD_NO_HINTING;
	else
		mHinting = 0;

	CacheFlush();
}

void hkFont::Style( int style ) {
	int prev_style = mFaceStyle;
	mStyle = style | mFaceStyle;

	if ( ( mStyle | HK_TTF_STYLE_NO_GLYPH_CHANGE ) != ( prev_style | HK_TTF_STYLE_NO_GLYPH_CHANGE ) )
		CacheFlush();
}

int hkFont::UnderlineTopRow() {
	return mAscent - mUnderlineOffset - 1;
}

int hkFont::StrikeThroughTopRow() {
	return mHeight / 2;
}

void hkFont::CacheFlush() {
	for( int i = 0; i < (int)mCacheSize; ++i ) {
		if( mCache[i].Cached() )
			mCache[i].Flush();
	}

	if( mScratch.Cached() )
		mScratch.Flush();
}

FT_Error hkFont::GlyphFind( u16 ch, int want ) {
	int retval = 0;

	if( ch < mCacheSize ) {
		mCurrent = &mCache[ch];
	} else {
		if ( mScratch.Cached() != ch )
			mScratch.Flush();

		mCurrent = &mScratch;
	}

	if ( ( mCurrent->Stored() & want) != want )
		retval = GlyphLoad( ch, mCurrent, want );

	return retval;
}

FT_Error hkFont::GlyphLoad( u16 ch, hkGlyph * cached, int want ) {
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( NULL == mFace )
		return FT_Err_Invalid_Handle;

	face = mFace;

	mFm->MutexLock();

	if ( !cached->Index() )
		cached->Index( FT_Get_Char_Index( face, ch ) );


	error = FT_Load_Glyph( face, cached->Index(), FT_LOAD_DEFAULT | mHinting );

	if( error )
		return error;

	glyph = face->glyph;
	metrics = &glyph->metrics;
	outline = &glyph->outline;

	if ( ( want & CACHED_METRICS ) && !( cached->Stored() & CACHED_METRICS ) ) {
		if ( FT_IS_SCALABLE( face ) ) {
			cached->MinX( FT_FLOOR( metrics->horiBearingX ) );
			cached->MaxX( cached->MinX() + FT_CEIL( metrics->width ) );
			cached->MaxY( FT_FLOOR( metrics->horiBearingY ) );
			cached->MinY( cached->MaxY() - FT_CEIL( metrics->height ) );
			cached->OffsetY( mAscent - cached->MaxY() );
			cached->Advance( FT_CEIL( metrics->horiAdvance ) );
		} else {
			cached->MinX( FT_FLOOR( metrics->horiBearingX ) );
			cached->MaxX( cached->MinX() + FT_CEIL( metrics->horiAdvance ) );
			cached->MaxY( FT_FLOOR( metrics->horiBearingY ) );
			cached->MinY( cached->MaxY() - FT_CEIL( face->available_sizes[ mFontSizeFamily ].height ) );
			cached->OffsetY( 0 );
			cached->Advance( FT_CEIL( metrics->horiAdvance ) );
		}

		if( HK_TTF_HANDLE_STYLE_BOLD(this) )
			cached->MaxX( cached->MaxX() + mGlyphOverhang );

		if( HK_TTF_HANDLE_STYLE_ITALIC(this) )
			cached->MaxX( cached->MaxX() + (int)ceil( mGlyphItalics ) );

		cached->Stored( cached->Stored() | CACHED_METRICS );
	}

	if ( ( want & CACHED_PIXMAP ) && !( cached->Stored() & CACHED_PIXMAP) ) {
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

			error = FT_Stroker_New( mFm->Library(), &stroker );

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

		dst = cached->Pixmap();

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

		cached->Stored( cached->Stored() | CACHED_PIXMAP );

		if( bitmap_glyph ) {
			FT_Done_Glyph( bitmap_glyph );
		}
	}

	mFm->MutexUnlock();

	cached->Cached( ch );

	return 0;
}

unsigned char * hkFont::GlyphRender( u16 ch, u32 fg ) {
	unsigned char * textbuf = NULL;
	int row;
	FT_Error error;
	hkGlyph *glyph;
	FT_Bitmap * bitmap;

	error = GlyphFind( ch, CACHED_METRICS | CACHED_PIXMAP );

	if ( error )
		return(NULL);

	glyph = mCurrent;
	bitmap = glyph->Pixmap();

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
		row = UnderlineTopRow();
		DrawLine( textbuf, row, fg, bitmap );
	}

	if( HK_TTF_HANDLE_STYLE_STRIKETHROUGH(this) ) {
		row = StrikeThroughTopRow();
		DrawLine( textbuf, row, fg, bitmap );
	}

	return textbuf;
}

int hkFont::GlyphMetrics( u16 ch, int* minx, int* maxx, int* miny, int* maxy, int* advance ) {
	FT_Error error;

	error = GlyphFind( ch, CACHED_METRICS );

	if ( error )
		return -1;

	if ( NULL != minx )
		*minx = mCurrent->MinX();

	if ( NULL !=maxx ) {
		*maxx = mCurrent->MaxX();

		if( HK_TTF_HANDLE_STYLE_BOLD(this) )
			*maxx += mGlyphOverhang;
	}

	if ( NULL !=miny )
		*miny = mCurrent->MinY();

	if ( NULL !=maxy )
		*maxy = mCurrent->MaxY();

	if ( NULL != advance ) {
		*advance = mCurrent->Advance();

		if( HK_TTF_HANDLE_STYLE_BOLD(this) )
			*advance += mGlyphOverhang;
	}

	return 0;
}

void hkFont::InitLineMectrics( const unsigned char * textbuf, const int row, u8 **pdst, int *pheight, FT_Bitmap * bitmap ) {
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

void hkFont::DrawLine( const unsigned char * textbuf, const int row, const u32 color, FT_Bitmap * bitmap ) {
	int line;
	u32 * dst_check = (u32*)textbuf + bitmap->pitch / 4 * bitmap->rows;
	u8 * dst8; /* destination, byte version */
	u32 * dst;
	int height;
	int col;

	u32 pixel = color | 0xFF000000;

	InitLineMectrics( textbuf, row, &dst8, &height, bitmap );
	dst = (u32 *) dst8;

	for ( line = height; line > 0 && dst < dst_check; --line ) {
		for ( col = 0; col < bitmap->width; ++col )
			dst[col] = pixel;

		dst += bitmap->pitch / 4;
	}
}

void hkFont::Face( FT_Face face ) {
	mFace = face;
}

FT_Face	hkFont::Face() {
	return mFace;
}

void hkFont::Height( int height ) {
	mHeight = height;
}

int	hkFont::Height() {
	return mHeight;
}

void hkFont::Ascent( int ascent ) {
	mAscent = ascent;
}

int hkFont::Ascent() {
	return mAscent;
}

void hkFont::Descent( int descent ) {
	mDescent = descent;
}

int hkFont::Descent() {
	return mDescent;
}

void hkFont::LineSkip( int lineskip ) {
	mLineSkip = lineskip;
}

int	hkFont::LineSkip() {
	return mLineSkip;
}

void hkFont::FaceStyle( int facestyle ) {
	mFaceStyle = facestyle;
}

int hkFont::FaceStyle()	{
	return mFaceStyle;
}

void hkFont::Kerning( int kerning )	{
	mKerning = kerning;
}

int hkFont::Kerning() {
	return mKerning;
}

void hkFont::GlyphOverhang( int glyphoverhang )	{
	mGlyphOverhang = glyphoverhang;
}

int hkFont::GlyphOverhang()	{
	return mGlyphOverhang;
}

void hkFont::GlyphItalics( float glyphitalics ) {
	mGlyphItalics = glyphitalics;
}

float hkFont::GlyphItalics() {
	return mGlyphItalics;
}

void hkFont::UnderlineOffset( int underlineoffset ) {
	mUnderlineOffset = underlineoffset;
}

int hkFont::UnderlineOffset() {
	return mUnderlineOffset;
}

void hkFont::UnderlineHeight( int underlineheight )	{
	mUnderlineHeight = underlineheight;
}

int hkFont::UnderlineHeight() {
	return mUnderlineHeight;
}

void hkFont::Current( hkGlyph * current ) {
	mCurrent = current;
}

hkGlyph * hkFont::Current()	{
	return mCurrent;
}

void hkFont::Scratch( hkGlyph scratch ) {
	mScratch = scratch;
}

hkGlyph	hkFont::Scratch() {
	return mScratch;
}

void hkFont::FontSizeFamily( int fontsizefamily ) {
	mFontSizeFamily = fontsizefamily;
}

int	hkFont::FontSizeFamily() {
	return mFontSizeFamily;
}

int	hkFont::Outline() const {
	return mOutline;
}

int	hkFont::Hinting() const {
	return mHinting;
}

int	hkFont::Style() {
	return mStyle;
}

hkFontManager * hkFont::Manager() const {
	return mFm;
}

}
