#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/glextensions.hpp>

namespace EE { namespace Graphics {

TextCache::TextCache() :
	mFont(NULL),
	mCachedWidth(0.f),
	mNumLines(1),
	mLargestLineCharCount(0),
	mFontColor(255,255,255,255),
	mFontShadowColor(0,0,0,255),
	mFlags(0),
	mVertexNumCached(0),
	mCachedCoords(false)
{
}

TextCache::TextCache( Graphics::Font * font, const String& text, ColorA FontColor, ColorA FontShadowColor ) :
	mText( text ),
	mFont( font ),
	mCachedWidth(0.f),
	mNumLines(1),
	mLargestLineCharCount(0),
	mFlags(0),
	mVertexNumCached(0),
	mCachedCoords(false)
{
	cacheWidth();
	updateCoords();
	setColor( FontColor );
	setShadowColor( FontShadowColor );
}

TextCache::~TextCache() {
}

void TextCache::create( Graphics::Font * font, const String& text, ColorA FontColor, ColorA FontShadowColor ) {
	mFont = font;
	mText = text;
	updateCoords();
	setColor( FontColor );
	setShadowColor( FontShadowColor );
	cacheWidth();
}

Graphics::Font * TextCache::getFont() const {
	return mFont;
}

void TextCache::setFont( Graphics::Font * font ) {
	mFont = font;
	cacheWidth();
}

String& TextCache::getText() {
	return mText;
}

void TextCache::updateCoords() {
	Uint32 size = (Uint32)mText.size() * GLi->quadVertexs();
	
	mRenderCoords.resize( size );
	mColors.resize( size, mFontColor );
}

void TextCache::setText( const String& text ) {
	bool needUpdate = false;

	if ( mText.size() != text.size() )
		needUpdate = true;

	mText = text;

	if ( needUpdate )
		updateCoords();

	cacheWidth();
}

const ColorA& TextCache::getColor() const {
	return mFontColor;
}

void TextCache::setAlpha( const Uint8& alpha ) {
	std::size_t s = mColors.size();
	for ( Uint32 i = 0; i < s; i++ ) {
		mColors[ i ].Alpha = alpha;
	}
}

void TextCache::setColor( const ColorA& color ) {
	if ( mFontColor != color ) {
		mFontColor = color;

		mColors.assign( mText.size() * GLi->quadVertexs(), mFontColor );
	}
}

void TextCache::setColor( const ColorA& color, Uint32 from, Uint32 to ) {
	std::vector<ColorA> colors( GLi->quadVertexs(), color );
	std::size_t s = mText.size();

	if ( to >= s ) {
		to = s - 1;
	}

	if ( from <= to && from < s && to <= s ) {
		size_t rto	= to + 1;
		Int32 rpos	= from;
		Int32 lpos	= 0;
		Uint32 i;
		Uint32 qsize = sizeof(ColorA) * GLi->quadVertexs();
		String::StringBaseType curChar;

		// New lines and tabs are not rendered, and not counted as a color
		// We need to skip those characters as nonexistent chars
		for ( i = 0; i < from; i++ ) {
			curChar = mText.at(i);
			if ( '\n' == curChar || '\t' == curChar || '\v' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;
				}
			}
		}

		for ( Uint32 i = from; i < rto; i++ ) {
			curChar = mText.at(i);

			lpos	= rpos;
			rpos++;

			// Same here
			if ( '\n' == curChar || '\t' == curChar || '\v' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;
				}
			}

			memcpy( &(mColors[ lpos * GLi->quadVertexs() ]), &colors[0], qsize );
		}
	}
}

const ColorA& TextCache::getShadowColor() const {
	return mFontShadowColor;
}

void TextCache::setShadowColor(const ColorA& color) {
	mFontShadowColor = color;
}

std::vector<eeVertexCoords>& TextCache::getVertextCoords() {
	return mRenderCoords;
}

std::vector<ColorA>& TextCache::getColors() {
	return mColors;
}

void TextCache::cacheWidth() {
	if ( NULL != mFont && mText.size() ) {
		mFont->cacheWidth( mText, mLinesWidth, mCachedWidth, mNumLines, mLargestLineCharCount );
	}else {
		mCachedWidth = 0;
	}

	mCachedCoords = false;
}

void TextCache::internalDraw( const Float& X, const Float& Y, const Vector2f & Scale, const Float & Angle, EE_BLEND_MODE Effect ) {
	GlobalBatchRenderer::instance()->draw();
	TextureFactory::instance()->bind( mFont->getTexId() );
	BlendMode::setMode( Effect );

	if ( mFlags & FONT_DRAW_SHADOW ) {
		Uint32 f = mFlags;

		mFlags &= ~FONT_DRAW_SHADOW;

		ColorA Col = getColor();

		if ( Col.a() != 255 ) {
			ColorA ShadowColor = getShadowColor();

			ShadowColor.Alpha = (Uint8)( (Float)ShadowColor.Alpha * ( (Float)Col.a() / (Float)255 ) );

			setColor( ShadowColor );
		} else {
			setColor( getShadowColor() );
		}

		Float pd = PixelDensity::getPixelDensity();

		GLi->translatef( 1 * pd , 1 * pd, 0.f );

		internalDraw( X, Y, Scale, Angle, Effect );

		GLi->translatef( -1 * pd , -1 * pd, 0.f );

		mFlags = f;

		setColor( Col );
	}

	Float cX = (Float) ( (Int32)X );
	Float cY = (Float) ( (Int32)Y );
	unsigned int numvert = 0;

	if ( Angle != 0.0f || Scale != 1.0f ) {
		GLi->pushMatrix();

		Vector2f Center( cX + getTextWidth() * 0.5f, cY + getTextHeight() * 0.5f );
		GLi->translatef( Center.x , Center.y, 0.f );
		GLi->rotatef( Angle, 0.0f, 0.0f, 1.0f );
		GLi->scalef( Scale.x, Scale.y, 1.0f );
		GLi->translatef( -Center.x + X, -Center.y + Y, 0.f );
	}

	if ( !cachedCoords() ) {
		cacheVerts( X, Y );
	}

	numvert = cachedVerts();

	Uint32 alloc	= numvert * sizeof(eeVertexCoords);
	Uint32 allocC	= numvert * GLi->quadVertexs();

	GLi->colorPointer	( 4, GL_UNSIGNED_BYTE	, 0						, reinterpret_cast<char*>( &mColors[0] )							, allocC	);
	GLi->texCoordPointer( 2, GL_FP				, sizeof(eeVertexCoords), reinterpret_cast<char*>( &mRenderCoords[0] )						, alloc		);
	GLi->vertexPointer	( 2, GL_FP				, sizeof(eeVertexCoords), reinterpret_cast<char*>( &mRenderCoords[0] ) + sizeof(Float) * 2	, alloc		);

	if ( GLi->quadsSupported() ) {
		GLi->drawArrays( GL_QUADS, 0, numvert );
	} else {
		GLi->drawArrays( GL_TRIANGLES, 0, numvert );
	}

	if ( Angle != 0.0f || Scale != 1.0f ) {
		GLi->popMatrix();
	}
}

void TextCache::cacheVerts( const Int32& X, const Int32& Y ) {
	if ( cachedCoords() )
		return;

	Float nX = 0;
	Float nY = 0;
	Uint32 Char = 0;
	unsigned int Line = 0;

	if ( !( mFlags & FONT_DRAW_VERTICAL ) ) {
		switch ( fontHAlignGet( mFlags ) ) {
			case FONT_DRAW_CENTER:
				nX = (Float)( (Int32)( ( getTextWidth() - getLinesWidth()[ Line ] ) * 0.5f ) );
				Line++;
				break;
			case FONT_DRAW_RIGHT:
				nX = getTextWidth() - getLinesWidth()[ Line ];
				Line++;
				break;
		}
	}

	Uint32 tGlyphSize = mFont->getGlyphCount();
	Float cX = (Float) ( (Int32)X );
	Float cY = (Float) ( (Int32)Y );
	unsigned int numvert = 0;

	for ( unsigned int i = 0; i < getText().size(); i++ ) {
		Char = getText().at(i);

		if ( Char < 0 && Char > -128 )
			Char = 256 + Char;

		if ( Char >= 0 && Char < tGlyphSize ) {
			eeTexCoords C = mFont->getTexCoords( Char );
			eeGlyph Glyph = mFont->getGlyph( Char );

			switch( Char ) {
				case '\v':
				{
					if ( mFlags & FONT_DRAW_VERTICAL )
						nY += mFont->getFontHeight();
					else
						nX += Glyph.Advance;
					break;
				}
				case '\t':
				{
					if ( mFlags & FONT_DRAW_VERTICAL )
						nY += mFont->getFontHeight() * 4;
					else
						nX += Glyph.Advance * 4;
					break;
				}
				case '\n':
				{
					if ( mFlags & FONT_DRAW_VERTICAL ) {
						nX += (mFont->getFontHeight());
						nY = 0;
					} else {
						if ( i + 1 < getText().size() ) {
							switch ( fontHAlignGet( mFlags ) ) {
								case FONT_DRAW_CENTER:
									nX = (Float)( (Int32)( ( getTextWidth() - getLinesWidth()[ Line ] ) * 0.5f ) );
									break;
								case FONT_DRAW_RIGHT:
									nX = getTextWidth() - getLinesWidth()[ Line ];
									break;
								default:
									nX = 0;
							}
						}

						nY += (mFont->getFontHeight());
						Line++;
					}

					break;
				}
				default:
				{
					if ( GLi->quadsSupported() ) {
						for ( Uint8 z = 0; z < 8; z+=2 ) {
							mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[z];
							mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ z + 1 ];
							mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[z] + nX;
							mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ z + 1 ] + nY;
							numvert++;
						}
					} else {
						mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[2];
						mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ 2 + 1 ];
						mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[2] + nX;
						mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ 2 + 1 ] + nY;
						numvert++;

						mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[0];
						mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ 0 + 1 ];
						mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[0] + nX;
						mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ 0 + 1 ] + nY;
						numvert++;

						mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[6];
						mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ 6 + 1 ];
						mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[6] + nX;
						mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ 6 + 1 ] + nY;
						numvert++;

						mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[2];
						mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ 2 + 1 ];
						mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[2] + nX;
						mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ 2 + 1 ] + nY;
						numvert++;

						mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[4];
						mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ 4 + 1 ];
						mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[4] + nX;
						mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ 4 + 1 ] + nY;
						numvert++;

						mRenderCoords[ numvert ].TexCoords[0]	= C.TexCoords[6];
						mRenderCoords[ numvert ].TexCoords[1]	= C.TexCoords[ 6 + 1 ];
						mRenderCoords[ numvert ].Vertex[0]		= cX + C.Vertex[6] + nX;
						mRenderCoords[ numvert ].Vertex[1]		= cY + C.Vertex[ 6 + 1 ] + nY;
						numvert++;
					}

					if ( mFlags & FONT_DRAW_VERTICAL )
						nY += mFont->getFontHeight();
					else
						nX += Glyph.Advance;
				}
			}
		}
	}

	cachedCoords( true );
	cachedVerts( numvert );
}

Float TextCache::getTextWidth() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? (Float)mFont->getFontHeight() * (Float)mNumLines : mCachedWidth;
}

Float TextCache::getTextHeight() {
	return ( mFlags & FONT_DRAW_VERTICAL ) ? mLargestLineCharCount * (Float)mFont->getFontHeight() : (Float)mFont->getFontHeight() * (Float)mNumLines;
}

const int& TextCache::getNumLines() const {
	return mNumLines;
}

const std::vector<Float>& TextCache::getLinesWidth() {
	return mLinesWidth;
}

void TextCache::draw( const Float& X, const Float& Y, const Vector2f& Scale, const Float& Angle, EE_BLEND_MODE Effect ) {
	if ( NULL != mFont ) {
		GlobalBatchRenderer::instance()->draw();

		if ( Angle != 0.0f || Scale != 1.0f ) {
			internalDraw( X, Y, Scale, Angle, Effect );
		} else {
			GLi->translatef( X, Y, 0.f );
	
			internalDraw( 0, 0, Scale, Angle, Effect );
	
			GLi->translatef( -X, -Y, 0.f );
		}
	}
}

const bool& TextCache::cachedCoords() const {
	return mCachedCoords;
}

void TextCache::cachedCoords( const bool& cached ) {
	mCachedCoords = cached;
}

const unsigned int& TextCache::cachedVerts() const {
	return mVertexNumCached;
}

void TextCache::cachedVerts( const unsigned int& num ) {
	mVertexNumCached = num;
}

void TextCache::setFlags( const Uint32& flags ) {
	if ( mFlags != flags ) {
		mFlags = flags;
		mCachedCoords = false;

		if ( ( mFlags & FONT_DRAW_VERTICAL ) != ( flags & FONT_DRAW_VERTICAL ) ) {
			cacheWidth();
		}
	}
}

const Uint32& TextCache::getFlags() const {
	return mFlags;
}

}}
