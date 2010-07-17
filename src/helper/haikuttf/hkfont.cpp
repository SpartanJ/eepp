#include "hkfont.hpp"
#include "hkfontmanager.hpp"

namespace HaikuTTF {

hkFont::hkFont( hkFontManager * FontManager, unsigned int CacheSize ) :
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
	mFm(NULL),
	mCache(NULL),
	mStyle(0),
	mOutline(0),
	mHinting(0)
{
	mFm = FontManager;

	mCacheSize = CacheSize;
	mCache = new hkGlyph[ mCacheSize ];
}

hkFont::~hkFont() {
	hkSAFE_DELETE_ARRAY( mCache );
}

void hkFont::Outline( int outline ) {
	mOutline = outline;
	CacheFlush();
}

void hkFont::Hinting( int hinting ) {
	if (hinting == TTF_HINTING_LIGHT)
		mHinting = FT_LOAD_TARGET_LIGHT;
	else if (hinting == TTF_HINTING_MONO)
		mHinting= FT_LOAD_TARGET_MONO;
	else if (hinting == TTF_HINTING_NONE)
		mHinting = FT_LOAD_NO_HINTING;
	else
		mHinting = 0;

	CacheFlush();
}

void hkFont::Style( int style ) {
	int prev_style = mFaceStyle;
	mStyle = style | mFaceStyle;

	if ( ( mStyle | TTF_STYLE_NO_GLYPH_CHANGE ) != ( prev_style | TTF_STYLE_NO_GLYPH_CHANGE ) )
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

FT_Error hkFont::GlyphFind( uint16_t ch, int want ) {
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

FT_Error hkFont::GlyphLoad( uint16_t ch, hkGlyph * cached, int want ) {
	FT_Face face;
	FT_Error error;
	FT_GlyphSlot glyph;
	FT_Glyph_Metrics* metrics;
	FT_Outline* outline;

	if ( NULL == mFace )
		return FT_Err_Invalid_Handle;

	face = mFace;

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

		if( TTF_HANDLE_STYLE_BOLD(this) )
			cached->MaxX( cached->MaxX() + mGlyphOverhang );

		if( TTF_HANDLE_STYLE_ITALIC(this) )
			cached->MaxX( cached->MaxX() + (int)ceil( mGlyphItalics ) );

		cached->Stored( cached->Stored() | CACHED_METRICS );
	}

	if ( ( ( want & CACHED_BITMAP ) && !( cached->Stored() & CACHED_BITMAP ) ) || ( ( want & CACHED_PIXMAP ) && !( cached->Stored() & CACHED_PIXMAP) ) ) {
		int mono = ( want & CACHED_BITMAP );
		int i;
		FT_Bitmap* src;
		FT_Bitmap* dst;
		FT_Glyph bitmap_glyph = NULL;

		if( TTF_HANDLE_STYLE_ITALIC(this) ) {
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

		if ( mono ) {
			dst = cached->Bitmap();
		} else {
			dst = cached->Pixmap();
		}

		memcpy( dst, src, sizeof( *dst ) );

		if ( src->pixel_mode == FT_PIXEL_MODE_MONO ) {
			dst->pitch *= 8;
		} else if ( src->pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
			dst->pitch *= 4;
		} else if ( src->pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
			dst->pitch *= 2;
		}

		if( TTF_HANDLE_STYLE_BOLD(this) ) {
			int bump = mGlyphOverhang;
			dst->pitch += bump;
			dst->width += bump;
		}

		if( TTF_HANDLE_STYLE_ITALIC(this) ) {
			int bump = (int)ceil( mGlyphItalics );
			dst->pitch += bump;
			dst->width += bump;
		}

		if (dst->rows != 0) {
			dst->buffer = (unsigned char *)malloc( dst->pitch * dst->rows );

			if( !dst->buffer ) {
				return FT_Err_Out_Of_Memory;
			}
			memset( dst->buffer, 0, dst->pitch * dst->rows );

			for( i = 0; i < src->rows; i++ ) {
				int soffset = i * src->pitch;
				int doffset = i * dst->pitch;

				if ( mono ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					int j;
					if ( src->pixel_mode == FT_PIXEL_MODE_MONO ) {
						for ( j = 0; j < src->width; j += 8 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
							ch <<= 1;
							*dstp++ = (ch&0x80) >> 7;
						}
					}  else if ( src->pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
						for ( j = 0; j < src->width; j += 4 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
							ch <<= 2;
							*dstp++ = (((ch&0xA0) >> 6) >= 0x2) ? 1 : 0;
						}
					} else if ( src->pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
						for ( j = 0; j < src->width; j += 2 ) {
							unsigned char ch = *srcp++;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
							ch <<= 4;
							*dstp++ = (((ch&0xF0) >> 4) >= 0x8) ? 1 : 0;
						}
					} else {
						for ( j = 0; j < src->width; j++ ) {
							unsigned char ch = *srcp++;
							*dstp++ = (ch >= 0x80) ? 1 : 0;
						}
					}
				} else if ( src->pixel_mode == FT_PIXEL_MODE_MONO ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 8) {
						ch = *srcp++;
						for (k = 0; k < 8; ++k) {
							if ((ch&0x80) >> 7) {
								*dstp++ = NUM_GRAYS - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 1;
						}
					}
				} else if ( src->pixel_mode == FT_PIXEL_MODE_GRAY2 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 4 ) {
						ch = *srcp++;
						for ( k = 0; k < 4; ++k ) {
							if ((ch&0xA0) >> 6) {
								*dstp++ = NUM_GRAYS * ((ch&0xA0) >> 6) / 3 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 2;
						}
					}
				} else if ( src->pixel_mode == FT_PIXEL_MODE_GRAY4 ) {
					unsigned char *srcp = src->buffer + soffset;
					unsigned char *dstp = dst->buffer + doffset;
					unsigned char ch;
					int j, k;
					for ( j = 0; j < src->width; j += 2 ) {
						ch = *srcp++;
						for ( k = 0; k < 2; ++k ) {
							if ((ch&0xF0) >> 4) {
							    *dstp++ = NUM_GRAYS * ((ch&0xF0) >> 4) / 15 - 1;
							} else {
								*dstp++ = 0x00;
							}
							ch <<= 4;
						}
					}
				} else {
					memcpy(dst->buffer+doffset,
					       src->buffer+soffset, src->pitch);
				}
			}
		}

		if ( TTF_HANDLE_STYLE_BOLD(this) ) {
			int row;
			int col;
			int offset;
			int pixel;
			uint8_t* pixmap;

			for( row = dst->rows - 1; row >= 0; --row ) {
				pixmap = (uint8_t*) dst->buffer + row * dst->pitch;
				for( offset=1; offset <= mGlyphOverhang; ++offset ) {
					for( col = dst->width - 1; col > 0; --col ) {
						if( mono ) {
							pixmap[col] |= pixmap[col-1];
						} else {
							pixel = (pixmap[col] + pixmap[col-1]);
							if( pixel > NUM_GRAYS - 1 ) {
								pixel = NUM_GRAYS - 1;
							}
							pixmap[col] = (uint8_t) pixel;
						}
					}
				}
			}
		}

		if ( mono ) {
			cached->Stored( cached->Stored() | CACHED_BITMAP );
		} else {
			cached->Stored( cached->Stored() | CACHED_PIXMAP );
		}

		if( bitmap_glyph ) {
			FT_Done_Glyph( bitmap_glyph );
		}
	}

	cached->Cached( ch );

	return 0;
}

unsigned char * hkFont::GlyphRender( uint16_t ch, uint32_t fg ) {
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

	textbuf = new unsigned char[ bitmap->width * bitmap->rows * 4 ];

	if ( NULL == textbuf )
		return NULL;

	uint32_t * buffu32 = reinterpret_cast<uint32_t*> ( &textbuf[0] );

	memset( buffu32, fg, bitmap->width * bitmap->rows );

	const uint8_t* pixels = bitmap->buffer;

	for ( int y = 0; y < bitmap->rows; y++ ) {
		for ( int x = 0; x < bitmap->width; x++ ) {
			uint32_t index = (x + y * bitmap->width) * 4 + 3;

			textbuf[ index ] = pixels[ x ];
		}

		pixels += bitmap->pitch;
	}

	if( TTF_HANDLE_STYLE_UNDERLINE(this) ) {
		row = UnderlineTopRow();
		DrawLine( textbuf, row, fg, bitmap );
	}

	if( TTF_HANDLE_STYLE_STRIKETHROUGH(this) ) {
		row = StrikeThroughTopRow();
		DrawLine( textbuf, row, fg, bitmap );
	}

	return textbuf;
}

int hkFont::GlyphMetrics( uint16_t ch, int* minx, int* maxx, int* miny, int* maxy, int* advance ) {
	FT_Error error;

	error = GlyphFind( ch, CACHED_METRICS );

	if ( error )
		return -1;

	if ( NULL != minx )
		*minx = mCurrent->MinX();

	if ( NULL !=maxx ) {
		*maxx = mCurrent->MaxX();

		if( TTF_HANDLE_STYLE_BOLD(this) )
			*maxx += mGlyphOverhang;
	}

	if ( NULL !=miny )
		*miny = mCurrent->MinY();

	if ( NULL !=maxy )
		*maxy = mCurrent->MaxY();

	if ( advance ) {
		*advance = mCurrent->Advance();

		if( TTF_HANDLE_STYLE_BOLD(this) )
			*advance += mGlyphOverhang;
	}

	return 0;
}

void hkFont::InitLineMectrics( const unsigned char * textbuf, const int row, uint8_t **pdst, int *pheight, FT_Bitmap * bitmap ) {
	uint8_t *dst;
	int height;

	dst = (uint8_t *)textbuf;

	if( row > 0 )
		dst += row * bitmap->pitch;

	height = mUnderlineHeight;

	if( mOutline > 0 )
		height += mOutline * 2;

	*pdst = dst;
	*pheight = height;
}

void hkFont::DrawLine( const unsigned char * textbuf, const int row, const uint32_t color, FT_Bitmap * bitmap ) {
	int line;
	uint32_t * dst_check = (uint32_t*)textbuf + bitmap->pitch / 4 * bitmap->rows;
	uint8_t * dst8; /* destination, byte version */
	uint32_t * dst;
	int height;
	int col;

	uint32_t pixel = color | 0xFF000000;

	InitLineMectrics( textbuf, row, &dst8, &height, bitmap );
	dst = (uint32_t *) dst8;

	for ( line = height; line > 0 && dst < dst_check; --line ) {
		for ( col = 0; col < bitmap->width; ++col )
			dst[col] = pixel;

		dst += bitmap->pitch / 4;
	}
}

}
