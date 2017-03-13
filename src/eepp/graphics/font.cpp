#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Graphics {

Font::Font( const Uint32& Type, const std::string& Name ) :
	mType( Type ),
	mTexId(0),
	mHeight(0),
	mSize(0),
	mLineSkip(0),
	mAscent(0),
	mDescent(0)
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

Uint32 Font::getFontSize() const {
	return mSize;
}

Uint32 Font::getFontHeight() const {
	return mHeight;
}

Int32 Font::getLineSkip() const {
	return mLineSkip;
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

const GlyphData& Font::getGlyph(const Uint32 & index) {
	eeASSERT( index < mGlyphs.size() );
	return mGlyphs[ index ];
}

const TextureCoords& Font::getTextureCoords(const Uint32 & index) {
	eeASSERT( index < mTexCoords.size() );
	return mTexCoords[ index ];
}

Uint32 Font::getGlyphCount() const {
	return mGlyphs.size();
}

static bool isStopSelChar( Uint32 c ) {
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
		if ( isStopSelChar( Text[i] ) ) {
			EndCur = i;
			break;
		}
	}

	if ( 0 == CurPos ) {
		InitCur = 0;
	}

	for ( Int32 i = CurPos; i >= 0; i-- ) {
		if ( isStopSelChar( Text[i] ) ) {
			InitCur = i + 1;
			break;
		}
	}

	if ( InitCur == EndCur ) {
		InitCur = EndCur = -1;
	}
}

void Font::shrinkText( std::string& Str, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	Float tCurWidth = 0.f;
	Float tWordWidth = 0.f;
	Float tMaxWidth = (Float) MaxWidth;
	char * tChar = &Str[0];
	char * tLastSpace = NULL;
	Uint32  tGlyphSize = (Uint32)mGlyphs.size();

	while ( *tChar ) {
		if ( (Uint32)( *tChar ) < tGlyphSize ) {
			GlyphData * pChar = &mGlyphs[ ( *tChar ) ];
			Float fCharWidth	= (Float)pChar->Advance;

			if ( ( *tChar ) == '\t' )
				fCharWidth += pChar->Advance * 3;

			tWordWidth		+= fCharWidth;

			if ( ' ' == *tChar || '\0' == *( tChar + 1 ) ) {
				if ( tCurWidth + tWordWidth < tMaxWidth ) {
					tCurWidth		+= tWordWidth;
					tLastSpace		= tChar;

					tChar++;
				} else {
					if ( NULL != tLastSpace ) {
						*tLastSpace		= '\n';
						tChar	= tLastSpace + 1;
					} else {
						*tChar	= '\n';
					}

					if ( '\0' == *( tChar + 1 ) )
						tChar++;

					tLastSpace		= NULL;
					tCurWidth		= 0.f;
				}

				tWordWidth = 0.f;
			} else if ( '\n' == *tChar ) {
				tWordWidth 		= 0.f;
				tCurWidth 		= 0.f;
				tLastSpace		= NULL;
				tChar++;
			} else {
				tChar++;
			}
		} else {
			*tChar		= ' ';
		}
	}
}

void Font::shrinkText( String& Str, const Uint32& MaxWidth ) {
	if ( !Str.size() )
		return;

	Float tCurWidth = 0.f;
	Float tWordWidth = 0.f;
	Float tMaxWidth = (Float) MaxWidth;
	String::StringBaseType * tChar = &Str[0];
	String::StringBaseType * tLastSpace = NULL;

	while ( *tChar ) {
		if ( (String::StringBaseType)( *tChar ) < mGlyphs.size() ) {
			GlyphData * pChar = &mGlyphs[ ( *tChar ) ];
			Float fCharWidth	= (Float)pChar->Advance;

			if ( ( *tChar ) == '\t' )
				fCharWidth += pChar->Advance * 3;

			// Add the new char width to the current word width
			tWordWidth		+= fCharWidth;

			if ( ' ' == *tChar || '\0' == *( tChar + 1 ) ) {

				// If current width plus word width is minor to the max width, continue adding
				if ( tCurWidth + tWordWidth < tMaxWidth ) {
					tCurWidth		+= tWordWidth;
					tLastSpace		= tChar;

					tChar++;
				} else {
					// If it was an space before, replace that space for an new line
					// Start counting from the new line first character
					if ( NULL != tLastSpace ) {
						*tLastSpace		= '\n';
						tChar	= tLastSpace + 1;
					} else {	// The word is larger than the current possible width
						*tChar	= '\n';
					}

					if ( '\0' == *( tChar + 1 ) )
						tChar++;

					// Set the last spaces as null, because is a new line
					tLastSpace		= NULL;

					// New line, new current width
					tCurWidth		= 0.f;
				}

				// New word, so we reset the current word width
				tWordWidth = 0.f;
			} else if ( '\n' == *tChar ) {
				tWordWidth 		= 0.f;
				tCurWidth 		= 0.f;
				tLastSpace		= NULL;
				tChar++;
			} else {
				tChar++;
			}
		} else {	// Replace any unknown char as spaces.
			*tChar		= ' ';
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
