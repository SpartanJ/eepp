#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <algorithm>
#include <cmath>

namespace EE { namespace Graphics {

std::string Text::styleFlagToString( const Uint32& flags ) {
	std::string str;

	if ( flags & Bold )
		str += "bold";

	if ( flags & Italic ) {
		str += ( str.empty() ? "" : "|" );
		str += "italic";
	}

	if ( flags & Underlined ) {
		str += ( str.empty() ? "" : "|" );
		str += "underline";
	}

	if ( flags & StrikeThrough ) {
		str += ( str.empty() ? "" : "|" );
		str += "strikethrough";
	}

	if ( flags & Shadow ) {
		str += ( str.empty() ? "" : "|" );
		str += "shadow";
	}

	return str;
}

Uint32 Text::stringToStyleFlag( const std::string& str ) {
	std::string valStr = String::trim( str );
	String::toLowerInPlace( valStr );
	std::vector<std::string> strings = String::split( valStr, '|' );
	Uint32 flags = Text::Regular;

	if ( strings.size() ) {
		for ( std::size_t i = 0; i < strings.size(); i++ ) {
			std::string cur = strings[i];
			String::toLowerInPlace( cur );

			if ( "underlined" == cur || "underline" == cur )
				flags |= Text::Underlined;
			else if ( "bold" == cur )
				flags |= Text::Bold;
			else if ( "italic" == cur )
				flags |= Text::Italic;
			else if ( "strikethrough" == cur )
				flags |= Text::StrikeThrough;
			else if ( "shadowed" == cur || "shadow" == cur )
				flags |= Text::Shadow;
		}
	}

	return flags;
}

Text * Text::New() {
	return eeNew( Text, () );
}

Text * Text::New( const String& string, Font * font, unsigned int characterSize ) {
	return eeNew( Text, ( string, font, characterSize ) );
}

Text * Text::New( Font * font, unsigned int characterSize ) {
	return  eeNew( Text, ( font, characterSize ) );
}

Text::Text() :
	mString(),
	mFont(NULL),
	mCharacterSize(12),
	mRealCharacterSize(PixelDensity::dpToPxI(mCharacterSize)),
	mStyle(Regular),
	mFillColor(255, 255, 255, 255),
	mOutlineColor(0, 0, 0, 255),
	mOutlineThickness (0),
	mGeometryNeedUpdate(false),
	mCachedWidthNeedUpdate(false),
	mColorsNeedUpdate(false),
	mCachedWidth(0),
	mNumLines(0),
	mLargestLineCharCount(0),
	mFontShadowColor( Color( 0, 0, 0, 255 ) ),
	mAlign(0),
	mFontHeight(0)
{
}

Text::Text(const String& string, Font * font, unsigned int characterSize) :
	mString(string),
	mFont(font),
	mCharacterSize(characterSize),
	mRealCharacterSize(PixelDensity::dpToPxI(mCharacterSize)),
	mStyle(Regular),
	mFillColor(255, 255, 255, 255),
	mOutlineColor(0, 0, 0, 255),
	mOutlineThickness(0),
	mGeometryNeedUpdate(true),
	mCachedWidthNeedUpdate(true),
	mColorsNeedUpdate(true),
	mCachedWidth(0),
	mNumLines(0),
	mLargestLineCharCount(0),
	mFontShadowColor( Color( 0, 0, 0, 255 ) ),
	mAlign(0),
	mFontHeight( mFont->getFontHeight( mRealCharacterSize ) )
{
}

Text::Text(Font * font, unsigned int characterSize) :
	mFont(font),
	mCharacterSize(characterSize),
	mRealCharacterSize(PixelDensity::dpToPxI(mCharacterSize)),
	mStyle(Regular),
	mFillColor(255, 255, 255, 255),
	mOutlineColor(0, 0, 0, 255),
	mOutlineThickness(0),
	mGeometryNeedUpdate(true),
	mCachedWidthNeedUpdate(true),
	mColorsNeedUpdate(true),
	mCachedWidth(0),
	mNumLines(0),
	mLargestLineCharCount(0),
	mFontShadowColor( Color( 0, 0, 0, 255 ) ),
	mAlign(0),
	mFontHeight( mFont->getFontHeight( mRealCharacterSize ) )
{
}

void Text::create(Font * font, const String & text, Color FontColor, Color FontShadowColor, Uint32 characterSize ) {
	mFont = font;
	mString = text;
	mCharacterSize = characterSize;
	mRealCharacterSize = PixelDensity::dpToPxI(mCharacterSize);
	setFillColor( FontColor );
	setShadowColor( FontShadowColor );
	mGeometryNeedUpdate = true;
	mCachedWidthNeedUpdate = true;
	mColorsNeedUpdate = true;
	ensureColorUpdate();
	ensureGeometryUpdate();
}

void Text::setString(const String& string) {
	if (mString != string) {
		mString = string;
		mColorsNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setFont(Font * font) {
	if ( NULL != font && mFont != font) {
		mFont = font;

		mRealCharacterSize = PixelDensity::dpToPxI( mCharacterSize );
		mFontHeight = mFont->getFontHeight( mRealCharacterSize );

		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setCharacterSize(unsigned int size) {
	if ( NULL != mFont && mCharacterSize != size) {
		mCharacterSize = size;

		mRealCharacterSize = PixelDensity::dpToPxI( mCharacterSize );
		mFontHeight = mFont->getFontHeight( mRealCharacterSize );

		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setStyle(Uint32 style) {
	if (mStyle != style) {
		mStyle = style;
		mColorsNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setColor(const Color & color) {
	setFillColor(color);
}

void Text::setFillColor(const Color& color) {
	if (color != mFillColor) {
		mFillColor = color;
		mColorsNeedUpdate = true;
	}
}

void Text::setOutlineColor(const Color& color) {
	if (color != mOutlineColor) {
		mOutlineColor = color;
		mColorsNeedUpdate = true;
	}
}

void Text::setOutlineThickness(Float thickness) {
	if (thickness != mOutlineThickness) {
		mOutlineThickness = thickness;
		mColorsNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

String& Text::getString() {
	return mString;
}

Font* Text::getFont() const {
	return mFont;
}

unsigned int Text::getCharacterSize() const {
	return mCharacterSize;
}

unsigned int Text::getCharacterSizePx() const {
	return mRealCharacterSize;
}

const Uint32 &Text::getFontHeight() const {
	return mFontHeight;
}

Uint32 Text::getStyle() const {
	return mStyle;
}

void Text::setAlpha( const Uint8& alpha ) {
	std::size_t s = mColors.size();
	for ( Uint32 i = 0; i < s; i++ ) {
		mColors[ i ].a = alpha;
	}
	mFillColor.a = alpha;
}

const Color& Text::getFillColor() const {
	return mFillColor;
}

const Color &Text::getColor() const {
	return getFillColor();
}

const Color& Text::getOutlineColor() const {
	return mOutlineColor;
}

Float Text::getOutlineThickness() const {
	return mOutlineThickness;
}

Vector2f Text::findCharacterPos(std::size_t index) const {
	// Make sure that we have a valid font
	if (!mFont)
		return Vector2f();

	// Adjust the index if it's out of range
	if (index > mString.size())
		index = mString.size();

	// Precompute the variables needed by the algorithm
	bool  bold   = (mStyle & Bold) != 0;
	Float hspace = static_cast<Float>(mFont->getGlyph(L' ', mRealCharacterSize, bold).advance);
	Float vspace = static_cast<Float>(mFont->getLineSpacing(mRealCharacterSize));

	// Compute the position
	Vector2f position;
	Uint32 prevChar = 0;
	for (std::size_t i = 0; i < index; ++i) {
		Uint32 curChar = mString[i];

		// Apply the kerning offset
		position.x += static_cast<Float>(mFont->getKerning(prevChar, curChar, mRealCharacterSize));
		prevChar = curChar;

		// Handle special characters
		switch (curChar)
		{
			case ' ':  position.x += hspace;				 continue;
			case '\t': position.x += hspace * 4;			 continue;
			case '\n': position.y += vspace; position.x = 0; continue;
			case '\r': continue;
		}

		// For regular characters, add the advance offset of the glyph
		position.x += static_cast<Float>(mFont->getGlyph(curChar, mRealCharacterSize, bold).advance);
	}

	return position;
}

Int32 Text::findCharacterFromPos( const Vector2i& pos ) {
	if ( NULL == mFont )
		return 0;

	Float vspace = mFont->getLineSpacing(mRealCharacterSize);
	Float Width = 0, lWidth = 0, Height = vspace, lHeight = 0;
	Uint32 CharID;
	Uint32 prevChar = 0;
	std::size_t tSize = mString.size();
	bool  bold = (mStyle & Bold) != 0;

	for (std::size_t i = 0; i < tSize; ++i) {
		CharID = mString[i];
		Glyph glyph = mFont->getGlyph( CharID, mRealCharacterSize, bold, mOutlineThickness );

		lWidth = Width;

		if ( CharID != '\r' ) {
			Width += mFont->getKerning(prevChar, CharID, mRealCharacterSize);
			prevChar = CharID;
			Width += glyph.advance;
		}

		if ( CharID == '\t' ) {
			Width += glyph.advance * 3;
		}

		if ( CharID == '\n' ) {
			lWidth = 0;
			Width = 0;
		}

		if ( pos.x <= Width && pos.x >= lWidth && pos.y <= Height && pos.y >= lHeight ) {
			if ( i + 1 < tSize ) {
				Int32 curDist	= eeabs( pos.x - lWidth );
				Int32 nextDist	= eeabs( pos.x - ( lWidth + glyph.advance ) );

				if ( nextDist < curDist ) {
					return  i + 1;
				}
			}

			return i;
		}

		if ( CharID == '\n' ) {
			lHeight = Height;
			Height += vspace;

			if ( pos.x > Width && pos.y <= lHeight ) {
				return i;
			}
		}
	}

	if ( pos.x >= Width ) {
		return tSize;
	}

	return -1;
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

void Text::findWordFromCharacterIndex( const Int32& characterIndex, Int32& InitCur, Int32& EndCur ) {
	InitCur	= 0;
	EndCur	= mString.size();

	for ( std::size_t i = characterIndex; i < mString.size(); i++ ) {
		if ( isStopSelChar( mString[i] ) ) {
			EndCur = i;
			break;
		}
	}

	if ( 0 == characterIndex ) {
		InitCur = 0;
	}

	for ( Int32 i = characterIndex; i >= 0; i-- ) {
		if ( isStopSelChar( mString[i] ) ) {
			InitCur = i + 1;
			break;
		}
	}

	if ( InitCur == EndCur ) {
		InitCur = EndCur = -1;
	}
}

void Text::getWidthInfo( std::vector<Float>& LinesWidth, Float& CachedWidth, int& NumLines , int& LargestLineCharCount ) {
	if ( NULL == mFont )
		return;

	LinesWidth.clear();

	Float Width = 0, MaxWidth = 0;
	Uint32 CharID;
	Int32 Lines = 1;
	Int32 CharCount = 0;
	Uint32 prevChar = 0;
	LargestLineCharCount = 0;
	bool bold = (mStyle & Bold) != 0;

	for (std::size_t i = 0; i < mString.size(); ++i) {
		CharID = static_cast<Int32>( mString.at(i) );
		Glyph glyph = mFont->getGlyph( CharID, mRealCharacterSize, bold, mOutlineThickness );

		if ( CharID != '\r' ) {
			Width += mFont->getKerning(prevChar, CharID, mRealCharacterSize);
			prevChar = CharID;
			Width += glyph.advance;
		}

		CharCount++;

		if ( CharID == '\t' )
			Width += glyph.advance * 3;

		if ( CharID == '\n' ) {
			Lines++;

			LinesWidth.push_back( Width - glyph.advance );

			Width = 0;

			CharCount = 0;
		} else {
			if ( CharCount > LargestLineCharCount )
				LargestLineCharCount = CharCount;
		}

		if ( Width > MaxWidth )
			MaxWidth = Width;
	}

	if ( mString.size() && mString.at( mString.size() - 1 ) != '\n' ) {
		LinesWidth.push_back( Width );
	}

	CachedWidth = MaxWidth;
	NumLines = Lines;
}

void Text::shrinkText( const Uint32& MaxWidth ) {
	if ( !mString.size() || NULL == mFont )
		return;

	Float tCurWidth = 0.f;
	Float tWordWidth = 0.f;
	Float tMaxWidth = (Float) MaxWidth;
	String::StringBaseType * tChar = &mString[0];
	String::StringBaseType * tLastSpace = NULL;
	Uint32 prevChar = 0;
	bool bold = (mStyle & Bold) != 0;

	while ( *tChar ) {
		Glyph pChar = mFont->getGlyph( *tChar, mRealCharacterSize, bold, mOutlineThickness );

		Float fCharWidth	= (Float)pChar.advance;

		if ( ( *tChar ) == '\t' )
			fCharWidth += pChar.advance * 3;
		else if ( ( *tChar ) == '\r' )
			fCharWidth = 0;

		// Add the new char width to the current word width
		tWordWidth		+= fCharWidth;

		if ( *tChar != '\r' ) {
			tWordWidth += mFont->getKerning(prevChar, *tChar, mRealCharacterSize);
			prevChar = *tChar;
		}

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
	}

	mCachedWidthNeedUpdate = true;
	mGeometryNeedUpdate = true;
	mColorsNeedUpdate = true;
}

void Text::invalidateColors() {
	mColorsNeedUpdate = true;
}

Rectf Text::getLocalBounds() {
	ensureGeometryUpdate();

	return mBounds;
}

Float Text::getTextWidth() {
	cacheWidth();

	return mCachedWidth;
}

Float Text::getTextHeight() {
	cacheWidth();

	return NULL != mFont ? mFont->getLineSpacing(mRealCharacterSize) * ( 0 == mNumLines ? 1 : mNumLines ) : 0;
}

void Text::draw(const Float & X, const Float & Y, const Vector2f & Scale, const Float & Rotation, BlendMode Effect, const OriginPoint& rotationCenter, const OriginPoint& scaleCenter) {
	if ( NULL != mFont ) {
		ensureColorUpdate();
		ensureGeometryUpdate();

		unsigned int numvert = mVertices.size();

		if ( 0 == numvert )
			return;

		GlobalBatchRenderer::instance()->draw();
		TextureFactory::instance()->bind( mFont->getTexture(mRealCharacterSize), Texture::CoordinateType::Pixels );
		BlendMode::setMode( Effect );

		if ( mStyle & Shadow ) {
			mStyle &= ~Shadow;

			Color Col = getFillColor();

			if ( Col.a != 255 ) {
				Color ShadowColor = getShadowColor();
				ShadowColor.a = (Uint8)( (Float)ShadowColor.a * ( (Float)Col.a / (Float)255 ) );

				setFillColor( ShadowColor );
			} else {
				setFillColor( getShadowColor() );
			}

			mColors.assign( mColors.size(), getFillColor() );

			Float pd = PixelDensity::dpToPx(1);

			draw( X + pd, Y + pd, Scale, Rotation, Effect );

			mStyle |= Shadow;

			setFillColor( Col );
			mColors.assign( mColors.size(), getFillColor() );
		}

		if ( Rotation != 0.0f || Scale != 1.0f ) {
			Float cX = (Float) ( (Int32)X );
			Float cY = (Float) ( (Int32)Y );
			Vector2f Center( cX + mCachedWidth * 0.5f, cY + getTextHeight() * 0.5f );

			GLi->pushMatrix();

			Vector2f center = Center;
			if ( OriginPoint::OriginTopLeft == scaleCenter.OriginType ) center = Vector2f( cX, cY );
			else if ( OriginPoint::OriginCustom == scaleCenter.OriginType ) center = Vector2f( scaleCenter.x, scaleCenter.y );

			GLi->translatef( center.x , center.y, 0.f );
			GLi->scalef( Scale.x, Scale.y, 1.0f );
			GLi->translatef( -center.x, -center.y, 0.f );

			center = Center;
			if ( OriginPoint::OriginTopLeft == rotationCenter.OriginType ) center = Vector2f( cX, cY );
			else if ( OriginPoint::OriginCustom == rotationCenter.OriginType ) center = Vector2f( rotationCenter.x, rotationCenter.y );

			GLi->translatef( center.x , center.y, 0.f );
			GLi->rotatef( Rotation, 0.0f, 0.0f, 1.0f );
			GLi->translatef( -center.x + cX, -center.y + cY, 0.f );
		} else {
			GLi->translatef( X, Y, 0 );
		}

		Uint32 alloc	= numvert * sizeof(VertexCoords);
		Uint32 allocC	= numvert * GLi->quadVertexs();

		if ( 0 != mOutlineThickness ) {
			GLi->colorPointer	( 4, GL_UNSIGNED_BYTE	, 0						, reinterpret_cast<char*>( &mOutlineColors[0] )					, allocC	);
			GLi->texCoordPointer( 2, GL_FP				, sizeof(VertexCoords), reinterpret_cast<char*>( &mOutlineVertices[0] )						, alloc		);
			GLi->vertexPointer	( 2, GL_FP				, sizeof(VertexCoords), reinterpret_cast<char*>( &mOutlineVertices[0] ) + sizeof(Float) * 2	, alloc		);

			if ( GLi->quadsSupported() ) {
				GLi->drawArrays( GL_QUADS, 0, numvert );
			} else {
				GLi->drawArrays( GL_TRIANGLES, 0, numvert );
			}
		}

		GLi->colorPointer	( 4, GL_UNSIGNED_BYTE	, 0						, reinterpret_cast<char*>( &mColors[0] )						, allocC	);
		GLi->texCoordPointer( 2, GL_FP				, sizeof(VertexCoords), reinterpret_cast<char*>( &mVertices[0] )						, alloc		);
		GLi->vertexPointer	( 2, GL_FP				, sizeof(VertexCoords), reinterpret_cast<char*>( &mVertices[0] ) + sizeof(Float) * 2	, alloc		);

		if ( GLi->quadsSupported() ) {
			GLi->drawArrays( GL_QUADS, 0, numvert );
		} else {
			GLi->drawArrays( GL_TRIANGLES, 0, numvert );
		}

		if ( Rotation != 0.0f || Scale != 1.0f ) {
			GLi->popMatrix();
		} else {
			GLi->translatef( -X, -Y, 0 );
		}
	}
}

void Text::ensureGeometryUpdate() {
	cacheWidth();

	// Do nothing, if geometry has not changed
	if (!mGeometryNeedUpdate)
		return;

	// Mark geometry as updated
	mGeometryNeedUpdate = false;

	// Clear the previous geometry
	mVertices.clear();
	mOutlineVertices.clear();
	mBounds = Rectf();

	// No font or text: nothing to draw
	if (!mFont || mString.empty())
		return;

	// Compute values related to the text style
	bool  bold			   = (mStyle & Bold) != 0;
	bool  underlined		 = (mStyle & Underlined) != 0;
	bool  strikeThrough	  = (mStyle & StrikeThrough) != 0;
	Float italic			 = (mStyle & Italic) ? 0.208f : 0.f; // 12 degrees
	Float underlineOffset	= mFont->getUnderlinePosition(mRealCharacterSize);
	Float underlineThickness = mFont->getUnderlineThickness(mRealCharacterSize);

	// Compute the location of the strike through dynamically
	// We use the center point of the lowercase 'x' glyph as the reference
	// We reuse the underline thickness as the thickness of the strike through as well
	Rectf xBounds = mFont->getGlyph(L'x', mRealCharacterSize, bold).bounds;
	Float strikeThroughOffset = xBounds.Top + xBounds.Bottom / 2.f;

	// Precompute the variables needed by the algorithm
	Float hspace = static_cast<Float>(mFont->getGlyph(L' ', mRealCharacterSize, bold).advance);
	Float vspace = static_cast<Float>(mFont->getLineSpacing(mRealCharacterSize));
	Float x	  = 0.f;
	Float y	  = static_cast<Float>(mRealCharacterSize);

	// Create one quad for each character
	Float minX = static_cast<Float>(mRealCharacterSize);
	Float minY = static_cast<Float>(mRealCharacterSize);
	Float maxX = 0.f;
	Float maxY = 0.f;
	Uint32 prevChar = 0;

	Float centerDiffX = 0;
	unsigned int Line = 0;

	switch ( Font::getHorizontalAlign( mAlign ) ) {
		case TEXT_ALIGN_CENTER:
			centerDiffX = (Float)( (Int32)( ( mCachedWidth - mLinesWidth[ Line ] ) * 0.5f ) );
			Line++;
			break;
		case TEXT_ALIGN_RIGHT:
			centerDiffX = mCachedWidth - mLinesWidth[ Line ];
			Line++;
			break;
	}

	for (std::size_t i = 0; i < mString.size(); ++i) {
		Uint32 curChar = mString[i];

		// Apply the kerning offset
		x += mFont->getKerning(prevChar, curChar, mRealCharacterSize);
		prevChar = curChar;

		// If we're using the underlined style and there's a new line, draw a line
		if (underlined && (curChar == L'\n')) {
			addLine(mVertices, x, y, underlineOffset, underlineThickness, 0, centerDiffX);

			if (mOutlineThickness != 0)
				addLine(mOutlineVertices, x, y, underlineOffset, underlineThickness, mOutlineThickness, centerDiffX);
		}

		// If we're using the strike through style and there's a new line, draw a line across all characters
		if (strikeThrough && (curChar == L'\n')) {
			addLine(mVertices, x, y, strikeThroughOffset, underlineThickness, 0, centerDiffX);

			if (mOutlineThickness != 0)
				addLine(mOutlineVertices, x, y, strikeThroughOffset, underlineThickness, mOutlineThickness, centerDiffX);
		}

		if ( curChar == L'\n' ) {
			switch ( Font::getHorizontalAlign( mAlign ) ) {
				case TEXT_ALIGN_CENTER:
					centerDiffX = (Float)( (Int32)( ( mCachedWidth - mLinesWidth[ Line ] ) * 0.5f ) );
					break;
				case TEXT_ALIGN_RIGHT:
					centerDiffX = mCachedWidth - mLinesWidth[ Line ];
					break;
			}

			Line++;
		}

		// Handle special characters
		if ((curChar == ' ') || (curChar == '\t') || (curChar == '\n') || (curChar == '\r')) {
			// Update the current bounds (min coordinates)
			minX = std::min(minX, x);
			minY = std::min(minY, y);

			switch (curChar) {
				case ' ':  x += hspace;		break;
				case '\t': x += hspace * 4;	break;
				case '\n': y += vspace; x = 0; break;
				case '\r': break;
			}

			// Update the current bounds (max coordinates)
			maxX = std::max(maxX, x);
			maxY = std::max(maxY, y);

			// Next glyph, no need to create a quad for whitespace
			continue;
		}


		// Apply the outline
		if (mOutlineThickness != 0) {
			const Glyph& glyph = mFont->getGlyph(curChar, mRealCharacterSize, bold, mOutlineThickness);

			Float left   = glyph.bounds.Left;
			Float top	= glyph.bounds.Top;
			Float right  = glyph.bounds.Left + glyph.bounds.Right;
			Float bottom = glyph.bounds.Top  + glyph.bounds.Bottom;

			// Add the outline glyph to the vertices
			addGlyphQuad(mOutlineVertices, Vector2f(x, y), glyph, italic, mOutlineThickness, centerDiffX);

			// Update the current bounds with the outlined glyph bounds
			minX = std::min(minX, x + left   - italic * bottom - mOutlineThickness);
			maxX = std::max(maxX, x + right  - italic * top	- mOutlineThickness);
			minY = std::min(minY, y + top	- mOutlineThickness);
			maxY = std::max(maxY, y + bottom - mOutlineThickness);
		}

		// Extract the current glyph's description
		const Glyph& glyph = mFont->getGlyph(curChar, mRealCharacterSize, bold);

		// Add the glyph to the vertices
		addGlyphQuad(mVertices, Vector2f(x, y), glyph, italic, 0, centerDiffX);

		// Update the current bounds with the non outlined glyph bounds
		if (mOutlineThickness == 0) {
			Float left   = glyph.bounds.Left;
			Float top	= glyph.bounds.Top;
			Float right  = glyph.bounds.Left + glyph.bounds.Right;
			Float bottom = glyph.bounds.Top  + glyph.bounds.Bottom;

			minX = std::min(minX, x + left  - italic * bottom);
			maxX = std::max(maxX, x + right - italic * top);
			minY = std::min(minY, y + top);
			maxY = std::max(maxY, y + bottom);
		}

		// Advance to the next character
		x += glyph.advance;
	}

	// If we're using the underlined style, add the last line
	if (underlined && (x > 0)) {
		addLine(mVertices, x, y, underlineOffset, underlineThickness, 0, centerDiffX);

		if (mOutlineThickness != 0)
			addLine(mOutlineVertices, x, y, underlineOffset, underlineThickness, mOutlineThickness, centerDiffX);
	}

	// If we're using the strike through style, add the last line across all characters
	if (strikeThrough && (x > 0)) {
		addLine(mVertices, x, y, strikeThroughOffset, underlineThickness, 0, centerDiffX);

		if (mOutlineThickness != 0)
			addLine(mOutlineVertices, x, y, strikeThroughOffset, underlineThickness, mOutlineThickness, centerDiffX);
	}

	// Update the bounding rectangle
	mBounds.Left = minX;
	mBounds.Top = minY;
	mBounds.Right = maxX - minX;
	mBounds.Bottom = maxY - minY;
}

void Text::ensureColorUpdate() {
	if ( mColorsNeedUpdate ) {
		Uint32 tv = getTotalVertices();

		if ( mColors.size() < tv )
			mColors.resize( tv, mFillColor );

		mColors.assign( tv, mFillColor );

		if ( 0 != mOutlineThickness ) {
			if ( mOutlineColors.size() < tv )
				mOutlineColors.resize( tv, mOutlineColor );

			mOutlineColors.assign( tv, mOutlineColor );
		}

		mColorsNeedUpdate = false;
	}
}

const Color& Text::getShadowColor() const {
	return mFontShadowColor;
}

void Text::setShadowColor(const Color& color) {
	mFontShadowColor = color;
}

const int& Text::getNumLines() {
	cacheWidth();

	return mNumLines;
}

const std::vector<Float>& Text::getLinesWidth() {
	cacheWidth();

	return mLinesWidth;
}

void Text::setAlign( const Uint32& align ) {
	if ( mAlign != align ) {
		mAlign = align;
		mGeometryNeedUpdate = true;
	}
}

const Uint32& Text::getAlign() const {
	return mAlign;
}

void Text::cacheWidth() {
	if ( !mCachedWidthNeedUpdate )
		return;

	if ( NULL != mFont && mString.size() ) {
		getWidthInfo( mLinesWidth, mCachedWidth, mNumLines, mLargestLineCharCount );
		mCachedWidthNeedUpdate = false;
	} else {
		mCachedWidth = 0;
	}
}

void Text::setStyleConfig( const FontStyleConfig& styleConfig ) {
	setFont( styleConfig.Font );
	setCharacterSize( styleConfig.CharacterSize );
	setFillColor( styleConfig.FontColor );
	setStyle( styleConfig.Style );
	setOutlineThickness( styleConfig.OutlineThickness );
	setOutlineColor( styleConfig.OutlineColor );
	setShadowColor( styleConfig.ShadowColor );
}

void Text::setFillColor( const Color& color, Uint32 from, Uint32 to ) {
	if ( mString.empty() )
		return;

	ensureColorUpdate();

	bool  underlined = (mStyle & Underlined) != 0;
	bool  strikeThrough = (mStyle & StrikeThrough) != 0;
	std::size_t s = mString.size();

	if ( to >= s ) {
		to = s - 1;
	}

	if ( from <= to && from < s && to <= s ) {
		size_t rto	= to + 1;
		Int32 rpos	= from;
		Int32 lpos	= 0;
		Uint32 i;
		String::StringBaseType curChar;

		// Spaces, new lines and tabs are not rendered, and not counted as a color
		// We need to skip those characters as nonexistent chars
		for ( i = 0; i < from; i++ ) {
			curChar = mString[i];

			if ( ' ' == curChar || '\n' == curChar || '\t' == curChar || '\r' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;

					if ( '\n' == curChar) {
						if ( underlined )
							rpos++;

						if ( strikeThrough )
							rpos++;
					}
				}
			}
		}

		for ( i = from; i < rto; i++ ) {
			curChar = mString[i];

			lpos	= rpos;
			rpos++;

			// Same here
			if ( ' ' == curChar || '\n' == curChar || '\t' == curChar || '\r' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;

					if ( '\n' == curChar) {
						if ( underlined || strikeThrough ) {
							for ( int v = 0; v < GLi->quadVertexs(); v++ )
								mColors[ rpos * GLi->quadVertexs() + v ] = color;
						}

						if ( underlined )
							rpos++;

						if ( strikeThrough )
							rpos++;
					}
				}
			} else {
				for ( int v = 0; v < GLi->quadVertexs(); v++ )
					mColors[ lpos * GLi->quadVertexs() + v ] = color;
			}
		}

		if ( rto == s ) {
			if ( underlined ) {
				lpos++;
				Uint32 pos = lpos * GLi->quadVertexs();

				if ( pos < mColors.size() ) {
					for ( int v = 0; v < GLi->quadVertexs(); v++ )
						mColors[ lpos * GLi->quadVertexs() + v ] = color;
				}
			}

			if ( strikeThrough ) {
				lpos++;
				Uint32 pos = lpos * GLi->quadVertexs();

				if ( pos < mColors.size() ) {
					for ( int v = 0; v < GLi->quadVertexs(); v++ )
						mColors[ lpos * GLi->quadVertexs() + v ] = color;
				}
			}
		}
	}
}

// Add an underline or strikethrough line to the vertex array
void Text::addLine(std::vector<VertexCoords>& vertices, Float lineLength, Float lineTop, Float offset, Float thickness, Float outlineThickness, Int32 centerDiffX) {
	Float top = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
	Float bottom = top + std::floor(thickness + 0.5f);
	Float u1 = 0;
	Float v1 = 0;
	Float u2 = 1;
	Float v2 = 1;
	VertexCoords vc;

	if ( GLi->quadsSupported() ) {
		vc.texCoords.x	= u1;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + -outlineThickness;
		vc.position.y	= top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u1;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + -outlineThickness;
		vc.position.y	= bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + lineLength + outlineThickness;
		vc.position.y	= bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + lineLength + outlineThickness;
		vc.position.y	= top - outlineThickness;
		vertices.push_back( vc );
	} else {
		vc.texCoords.x	= u1;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + -outlineThickness;
		vc.position.y	= bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u1;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + -outlineThickness;
		vc.position.y	= top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + lineLength + outlineThickness;
		vc.position.y	= top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u1;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + -outlineThickness;
		vc.position.y	= bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + lineLength + outlineThickness;
		vc.position.y	= bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + lineLength + outlineThickness;
		vc.position.y	= top - outlineThickness;
		vertices.push_back( vc );
	}
}

// Add a glyph quad to the vertex array
void Text::addGlyphQuad(std::vector<VertexCoords>& vertices, Vector2f position, const EE::Graphics::Glyph& glyph, Float italic, Float outlineThickness, Int32 centerDiffX) {
	Float left		= glyph.bounds.Left;
	Float top		= glyph.bounds.Top;
	Float right		= glyph.bounds.Left + glyph.bounds.Right;
	Float bottom	= glyph.bounds.Top  + glyph.bounds.Bottom;

	Float u1 = static_cast<Float>(glyph.textureRect.Left);
	Float v1 = static_cast<Float>(glyph.textureRect.Top);
	Float u2 = static_cast<Float>(glyph.textureRect.Left + glyph.textureRect.Right);
	Float v2 = static_cast<Float>(glyph.textureRect.Top  + glyph.textureRect.Bottom);
	VertexCoords vc;

	if ( GLi->quadsSupported() ) {
		vc.texCoords.x	= u1;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + position.x + left  - italic * top	- outlineThickness;
		vc.position.y	= position.y + top	- outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u1;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + position.x + left  - italic * bottom - outlineThickness;
		vc.position.y	= position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + position.x + right - italic * bottom - outlineThickness;
		vc.position.y	= position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + position.x + right - italic * top - outlineThickness;
		vc.position.y	= position.y + top - outlineThickness;
		vertices.push_back( vc );
	} else {
		vc.texCoords.x	= u1;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + position.x + left  - italic * bottom - outlineThickness;
		vc.position.y	= position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u1;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + position.x + left  - italic * top	- outlineThickness;
		vc.position.y	= position.y + top	- outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + position.x + right - italic * top - outlineThickness;
		vc.position.y	= position.y + top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u1;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + position.x + left  - italic * bottom - outlineThickness;
		vc.position.y	= position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v2;
		vc.position.x	= centerDiffX + position.x + right - italic * bottom - outlineThickness;
		vc.position.y	= position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x	= u2;
		vc.texCoords.y	= v1;
		vc.position.x	= centerDiffX + position.x + right - italic * top - outlineThickness;
		vc.position.y	= position.y + top - outlineThickness;
		vertices.push_back( vc );
	}
}

Uint32 Text::getTotalVertices() {
	bool  underlined = (mStyle & Underlined) != 0;
	bool  strikeThrough = (mStyle & StrikeThrough) != 0;
	size_t sl = mString.size();
	size_t sv = sl * GLi->quadVertexs();

	Uint32 * c = &mString[0];
	Uint32 skiped = 0;
	bool lineHasChars = false;

	while ( '\0' != *c ) {
		lineHasChars = true;

		if ( ' ' == *c || '\n' == *c || '\t' == *c || '\r' == *c ) {
			lineHasChars = false;
			skiped++;

			if ( '\n' == *c ) {
				if ( underlined )
					skiped--;

				if ( strikeThrough )
					skiped--;
			}
		}

		c++;
	}

	if ( lineHasChars ) {
		if ( underlined )
			skiped--;

		if ( strikeThrough )
			skiped--;
	}

	sv -= skiped * GLi->quadVertexs();

	return sv;
}

}}
