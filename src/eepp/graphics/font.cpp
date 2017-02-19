#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>

namespace EE { namespace Graphics {

Font::Font( const Uint32& Type, const std::string& Name ) :
	mType( Type ),
	mTexId(0),
	mHeight(0),
	mSize(0),
	mLineSkip(0),
	mAscent(0),
	mDescent(0),
	mTextCache( this )
{
	this->setName( Name );
	FontManager::instance()->add( this );
}

Font::~Font() {
	mGlyphs.clear();

	if ( !FontManager::instance()->isDestroying() ) {
		FontManager::instance()->remove( this, false );
	}
}

void Font::setText( const String& Text ) {
	mTextCache.setText( Text );
}

const ColorA& Font::getColor() const {
	return mTextCache.getColor();
}

void Font::setColor(const ColorA& color) {
	mTextCache.setColor( color );
}

const ColorA& Font::getShadowColor() const {
	return mTextCache.getShadowColor();
}

void Font::setShadowColor(const ColorA& color) {
	mTextCache.setShadowColor( color );
}

int Font::getNumLines() {
	return mTextCache.getNumLines();
}

Float Font::getTextWidth( const String& Text ) {
	setText( Text );
	return mTextCache.getTextWidth();
}

Float Font::getTextWidth() {
	return mTextCache.getTextWidth();
}

Uint32 Font::getFontSize() const {
	return mSize;
}

Uint32 Font::getFontHeight() const {
	return mHeight;
}

Int32 Font::getLineSkip() const {
	return mLineSkip;
}

Int32 Font::getFontAscent() const {
	return mAscent;
}

Int32 Font::getFontDescent() const {
	return mDescent;
}

String Font::getText() {
	return mTextCache.getText();
}

Float Font::getTextHeight() {
	return (Float)getFontHeight() * (Float)getNumLines();
}

const std::vector<Float>& Font::getLinesWidth() {
	return mTextCache.getLinesWidth();
}

void Font::draw( const Float& X, const Float& Y, const Uint32& Flags, const Vector2f& Scale, const Float& Angle, const EE_BLEND_MODE& Effect) {
	draw( mTextCache, X, Y, Flags, Scale, Angle, Effect );
}

void Font::draw( const String& Text, const Float& X, const Float& Y, const Uint32& Flags, const Vector2f& Scale, const Float& Angle, const EE_BLEND_MODE& Effect ) {
	mTextCache.setText( Text );
	mTextCache.setFlags( Flags );
	mTextCache.draw( X, Y, Scale, Angle, Effect );
}

void Font::draw( TextCache& TextCache, const Float& X, const Float& Y, const Uint32& Flags, const Vector2f& Scale, const Float& Angle, const EE_BLEND_MODE& Effect ) {
	if ( !TextCache.getText().size() )
		return;

	GlobalBatchRenderer::instance()->draw();
	TextureFactory::instance()->bind( mTexId );
	BlendMode::setMode( Effect );

	if ( Flags & FONT_DRAW_SHADOW ) {
		Uint32 f = Flags;

		f &= ~FONT_DRAW_SHADOW;

		ColorA Col = TextCache.getColor();

		setText( TextCache.getText() );

		if ( Col.a() != 255 ) {
			ColorA ShadowColor = TextCache.getShadowColor();

			ShadowColor.Alpha = (Uint8)( (Float)ShadowColor.Alpha * ( (Float)Col.a() / (Float)255 ) );

			setColor( ShadowColor );
		} else {
			setColor( TextCache.getShadowColor() );
		}

		draw( X + 1, Y + 1, f, Scale, Angle, Effect );

		mTextCache.setFlags( Flags );

		setColor( Col );
	}

	Float cX = (Float) ( (Int32)X );
	Float cY = (Float) ( (Int32)Y );
	Float nX = 0;
	Float nY = 0;
	Int16 Char = 0;
	unsigned int Line = 0;
	unsigned int numvert = 0;

	if ( Angle != 0.0f || Scale != 1.0f ) {
		GLi->pushMatrix();

		Vector2f Center( cX + TextCache.getTextWidth() * 0.5f, cY + TextCache.getTextHeight() * 0.5f );
		GLi->translatef( Center.x , Center.y, 0.f );
		GLi->rotatef( Angle, 0.0f, 0.0f, 1.0f );
		GLi->scalef( Scale.x, Scale.y, 1.0f );
		GLi->translatef( -Center.x + X, -Center.y + Y, 0.f );
	}

	std::vector<eeVertexCoords>& RenderCoords = TextCache.getVertextCoords();
	std::vector<ColorA>& Colors = TextCache.getColors();

	if ( !TextCache.cachedCoords() ) {
		if ( !( Flags & FONT_DRAW_VERTICAL ) ) {
			switch ( fontHAlignGet( Flags ) ) {
				case FONT_DRAW_CENTER:
					nX = (Float)( (Int32)( ( TextCache.getTextWidth() - TextCache.getLinesWidth()[ Line ] ) * 0.5f ) );
					Line++;
					break;
				case FONT_DRAW_RIGHT:
					nX = TextCache.getTextWidth() - TextCache.getLinesWidth()[ Line ];
					Line++;
					break;
			}
		}

		Int32 tGlyphSize = (Int32)mGlyphs.size();

		for ( unsigned int i = 0; i < TextCache.getText().size(); i++ ) {
			Char = static_cast<Int32>( TextCache.getText().at(i) );

			if ( Char < 0 && Char > -128 )
				Char = 256 + Char;

			if ( Char >= 0 && Char < tGlyphSize ) {
				eeTexCoords* C = &mTexCoords[ Char ];

				switch( Char ) {
					case '\v':
					{
						if ( Flags & FONT_DRAW_VERTICAL )
							nY += getFontHeight();
						else
							nX += mGlyphs[ Char ].Advance;
						break;
					}
					case '\t':
					{
						if ( Flags & FONT_DRAW_VERTICAL )
							nY += getFontHeight() * 4;
						else
							nX += mGlyphs[ Char ].Advance * 4;
						break;
					}
					case '\n':
					{
						if ( Flags & FONT_DRAW_VERTICAL ) {
							nX += (getFontHeight() * Scale.y);
							nY = 0;
						} else {
							if ( i + 1 < TextCache.getText().size() ) {
								switch ( fontHAlignGet( Flags ) ) {
									case FONT_DRAW_CENTER:
										nX = (Float)( (Int32)( ( TextCache.getTextWidth() - TextCache.getLinesWidth()[ Line ] ) * 0.5f ) );
										break;
									case FONT_DRAW_RIGHT:
										nX = TextCache.getTextWidth() - TextCache.getLinesWidth()[ Line ];
										break;
									default:
										nX = 0;
								}
							}

							nY += (getFontHeight() * Scale.y);
							Line++;
						}

						break;
					}
					default:
					{
						if ( GLi->quadsSupported() ) {
							for ( Uint8 z = 0; z < 8; z+=2 ) {
								RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[z];
								RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ z + 1 ];
								RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[z] + nX;
								RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ z + 1 ] + nY;
								numvert++;
							}
						} else {
							RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[2];
							RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ 2 + 1 ];
							RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[2] + nX;
							RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ 2 + 1 ] + nY;
							numvert++;

							RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[0];
							RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ 0 + 1 ];
							RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[0] + nX;
							RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ 0 + 1 ] + nY;
							numvert++;

							RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[6];
							RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ 6 + 1 ];
							RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[6] + nX;
							RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ 6 + 1 ] + nY;
							numvert++;

							RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[2];
							RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ 2 + 1 ];
							RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[2] + nX;
							RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ 2 + 1 ] + nY;
							numvert++;

							RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[4];
							RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ 4 + 1 ];
							RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[4] + nX;
							RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ 4 + 1 ] + nY;
							numvert++;

							RenderCoords[ numvert ].TexCoords[0]	= C->TexCoords[6];
							RenderCoords[ numvert ].TexCoords[1]	= C->TexCoords[ 6 + 1 ];
							RenderCoords[ numvert ].Vertex[0]		= cX + C->Vertex[6] + nX;
							RenderCoords[ numvert ].Vertex[1]		= cY + C->Vertex[ 6 + 1 ] + nY;
							numvert++;
						}

						if ( Flags & FONT_DRAW_VERTICAL )
							nY += getFontHeight();
						else
							nX += mGlyphs[ Char ].Advance;
					}
				}
			}
		}

		TextCache.cachedCoords( true );
		TextCache.cachedVerts( numvert );
	} else {
		numvert = TextCache.cachedVerts();
	}

	Uint32 alloc	= numvert * sizeof(eeVertexCoords);
	Uint32 allocC	= numvert * GLi->quadVertexs();

	GLi->colorPointer	( 4, GL_UNSIGNED_BYTE	, 0						, reinterpret_cast<char*>( &Colors[0] )								, allocC	);
	GLi->texCoordPointer( 2, GL_FP				, sizeof(eeVertexCoords), reinterpret_cast<char*>( &RenderCoords[0] )						, alloc		);
	GLi->vertexPointer	( 2, GL_FP				, sizeof(eeVertexCoords), reinterpret_cast<char*>( &RenderCoords[0] ) + sizeof(Float) * 2	, alloc		);

	if ( GLi->quadsSupported() ) {
		GLi->drawArrays( GL_QUADS, 0, numvert );
	} else {
		GLi->drawArrays( GL_TRIANGLES, 0, numvert );
	}

	if ( Angle != 0.0f || Scale != 1.0f ) {
		GLi->popMatrix();
	}
}

void Font::cacheWidth( const String& Text, std::vector<Float>& LinesWidth, Float& CachedWidth, int& NumLines , int& LargestLineCharCount ) {
	LinesWidth.clear();

	Float Width = 0, MaxWidth = 0;
	Int32 CharID;
	Int32 Lines = 1;
	Int32 CharCount = 0;

	Int32 tGlyphSize = (Int32)mGlyphs.size();

	LargestLineCharCount = 0;

	for (std::size_t i = 0; i < Text.size(); ++i) {
		CharID = static_cast<Int32>( Text.at(i) );

		if ( CharID >= 0 && CharID < tGlyphSize ) {
			Width += mGlyphs[CharID].Advance;

			CharCount++;

			if ( CharID == '\t' )
				Width += mGlyphs[CharID].Advance * 3;

			if ( CharID == '\n' ) {
				Lines++;

				Float lWidth = ( CharID == '\t' ) ? mGlyphs[CharID].Advance * 4.f : mGlyphs[CharID].Advance;

				LinesWidth.push_back( Width - lWidth );

				Width = 0;

				CharCount = 0;
			} else {
				if ( CharCount > LargestLineCharCount )
					LargestLineCharCount = CharCount;
			}

			if ( Width > MaxWidth )
				MaxWidth = Width;
		}
	}

	if ( Text.size() && Text.at( Text.size() - 1 ) != '\n' ) {
		LinesWidth.push_back( Width );
	}

	CachedWidth = MaxWidth;
	NumLines = Lines;
}

Int32 Font::findClosestCursorPosFromPoint( const String& Text, const Vector2i& pos ) {
	Float Width = 0, lWidth = 0, Height = getFontHeight(), lHeight = 0;
	Int32 CharID;
	Int32 tGlyphSize = (Int32)mGlyphs.size();
	std::size_t tSize = Text.size();

	for (std::size_t i = 0; i < tSize; ++i) {
		CharID = static_cast<Int32>( Text.at(i) );

		if ( CharID >= 0 && CharID < tGlyphSize ) {
			lWidth = Width;

			Width += mGlyphs[CharID].Advance;

			if ( CharID == '\t' ) {
				Width += mGlyphs[CharID].Advance * 3;
			}

			if ( CharID == '\n' ) {
				lWidth = 0;
				Width = 0;
			}

			if ( pos.x <= Width && pos.x >= lWidth && pos.y <= Height && pos.y >= lHeight ) {
				if ( i + 1 < tSize ) {
					Int32 curDist	= eeabs( pos.x - lWidth );
					Int32 nextDist	= eeabs( pos.x - ( lWidth + mGlyphs[CharID].Advance ) );

					if ( nextDist < curDist ) {
						return  i + 1;
					}
				}

				return i;
			}

			if ( CharID == '\n' ) {
				lHeight = Height;
				Height += getFontHeight();

				if ( pos.x > Width && pos.y <= lHeight ) {
					return i;
				}
			}
		}
	}

	if ( pos.x >= Width ) {
		return tSize;
	}

	return -1;
}

Vector2i Font::getCursorPos( const String& Text, const Uint32& Pos ) {
	Float Width = 0, Height = getFontHeight();
	Int32 CharID;
	Int32 tGlyphSize = mGlyphs.size();
	std::size_t tSize = ( Pos < Text.size() ) ? Pos : Text.size();

	for (std::size_t i = 0; i < tSize; ++i) {
		CharID = static_cast<Int32>( Text.at(i) );

		if ( CharID >= 0 && CharID < tGlyphSize ) {
			Width += mGlyphs[CharID].Advance;

			if ( CharID == '\t' ) {
				Width += mGlyphs[CharID].Advance * 3;
			}

			if ( CharID == '\n' ) {
				Width = 0;
				Height += getFontHeight();
			}
		}
	}

	return Vector2i( Width, Height );
}

static bool IsStopSelChar( Uint32 c ) {
	return ( !String::isCharacter( c ) && !String::isNumber( c ) ) ||
			' ' == c ||
			'.' == c ||
			',' == c ||
			';' == c ||
			':' == c ||
			'\n' == c ||
			'"' == c ||
			'\'' == c;
}

void Font::selectSubStringFromCursor( const String& Text, const Int32& CurPos, Int32& InitCur, Int32& EndCur ) {
	InitCur	= 0;
	EndCur	= Text.size();

	for ( std::size_t i = CurPos; i < Text.size(); i++ ) {
		if ( IsStopSelChar( Text[i] ) ) {
			EndCur = i;
			break;
		}
	}

	if ( 0 == CurPos ) {
		InitCur = 0;
	}

	for ( Int32 i = CurPos; i >= 0; i-- ) {
		if ( IsStopSelChar( Text[i] ) ) {
			InitCur = i + 1;
			break;
		}
	}

	if ( InitCur == EndCur ) {
		InitCur = EndCur = -1;
	}
}

void Font::cacheWidth() {
	mTextCache.cacheWidth();
}

void Font::shrinkText( std::string& Str, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	Float		tCurWidth		= 0.f;
	Float 	tWordWidth		= 0.f;
	Float 	tMaxWidth		= (Float) MaxWidth;
	char *		tStringLoop		= &Str[0];
	char *		tLastSpace		= NULL;
	Uint32 		tGlyphSize 		= (Uint32)mGlyphs.size();

	while ( *tStringLoop ) {
		if ( (Uint32)( *tStringLoop ) < tGlyphSize ) {
			eeGlyph * pChar = &mGlyphs[ ( *tStringLoop ) ];
			Float fCharWidth	= (Float)pChar->Advance;

			if ( ( *tStringLoop ) == '\t' )
				fCharWidth += pChar->Advance * 3;

			tWordWidth		+= fCharWidth;

			if ( ' ' == *tStringLoop || '\0' == *( tStringLoop + 1 ) ) {
				if ( tCurWidth + tWordWidth < tMaxWidth ) {
					tCurWidth		+= tWordWidth;
					tLastSpace		= tStringLoop;

					tStringLoop++;
				} else {
					if ( NULL != tLastSpace ) {
						*tLastSpace		= '\n';
						tStringLoop	= tLastSpace + 1;
					} else {
						*tStringLoop	= '\n';
					}

					if ( '\0' == *( tStringLoop + 1 ) )
						tStringLoop++;

					tLastSpace		= NULL;
					tCurWidth		= 0.f;
				}

				tWordWidth = 0.f;
			} else if ( '\n' == *tStringLoop ) {
				tWordWidth 		= 0.f;
				tCurWidth 		= 0.f;
				tLastSpace		= NULL;
				tStringLoop++;
			} else {
				tStringLoop++;
			}
		} else {
			*tStringLoop		= ' ';
		}
	}
}

void Font::shrinkText( String& Str, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	Float		tCurWidth		= 0.f;
	Float 	tWordWidth		= 0.f;
	Float 	tMaxWidth		= (Float) MaxWidth;
	String::StringBaseType *	tStringLoop		= &Str[0];
	String::StringBaseType *	tLastSpace		= NULL;

	while ( *tStringLoop ) {
		if ( (String::StringBaseType)( *tStringLoop ) < mGlyphs.size() ) {
			eeGlyph * pChar = &mGlyphs[ ( *tStringLoop ) ];
			Float fCharWidth	= (Float)pChar->Advance;

			if ( ( *tStringLoop ) == '\t' )
				fCharWidth += pChar->Advance * 3;

			// Add the new char width to the current word width
			tWordWidth		+= fCharWidth;

			if ( ' ' == *tStringLoop || '\0' == *( tStringLoop + 1 ) ) {

				// If current width plus word width is minor to the max width, continue adding
				if ( tCurWidth + tWordWidth < tMaxWidth ) {
					tCurWidth		+= tWordWidth;
					tLastSpace		= tStringLoop;

					tStringLoop++;
				} else {
					// If it was an space before, replace that space for an new line
					// Start counting from the new line first character
					if ( NULL != tLastSpace ) {
						*tLastSpace		= '\n';
						tStringLoop	= tLastSpace + 1;
					} else {	// The word is larger than the current possible width
						*tStringLoop	= '\n';
					}

					if ( '\0' == *( tStringLoop + 1 ) )
						tStringLoop++;

					// Set the last spaces as null, because is a new line
					tLastSpace		= NULL;

					// New line, new current width
					tCurWidth		= 0.f;
				}

				// New word, so we reset the current word width
				tWordWidth = 0.f;
			} else if ( '\n' == *tStringLoop ) {
				tWordWidth 		= 0.f;
				tCurWidth 		= 0.f;
				tLastSpace		= NULL;
				tStringLoop++;
			} else {
				tStringLoop++;
			}
		} else {	// Replace any unknown char as spaces.
			*tStringLoop		= ' ';
		}
	}
}

const Uint32& Font::getTexId() const {
	return mTexId;
}

const Uint32& Font::getType() const {
	return mType;
}

const std::string& Font::getName() const {
	return mFontName;
}

void Font::setName( const std::string& name ) {
	mFontName = name;
	mFontHash = String::hash( mFontName );
}

const Uint32& Font::getId() {
	return mFontHash;
}

}}
