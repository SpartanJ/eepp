#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <cmath>

namespace EE { namespace Graphics {
	// Add an underline or strikethrough line to the vertex array
	void Text::addLine(std::vector<VertexCoords>& vertices, std::vector<ColorA>& colors, Float lineLength, Float lineTop, const EE::System::ColorA& color, Float offset, Float thickness, Float outlineThickness, Sizei textureSize, Int32 centerDiffX) {
		Float top = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
		Float bottom = top + std::floor(thickness + 0.5f);
		Float u1 = 0;
		Float v1 = 0;
		Float u2 = 1 / (Float)textureSize.getWidth();
		Float v2 = 1 / (Float)textureSize.getHeight();
		VertexCoords vc;

		if ( GLi->quadsSupported() ) {
			vc.texCoords.x	= u1;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + -outlineThickness;
			vc.position.y	= top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + -outlineThickness;
			vc.position.y	= bottom + outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + lineLength + outlineThickness;
			vc.position.y	= bottom + outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + lineLength + outlineThickness;
			vc.position.y	= top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );
		} else {
			vc.texCoords.x	= u1;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + -outlineThickness;
			vc.position.y	= bottom + outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + -outlineThickness;
			vc.position.y	= top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + lineLength + outlineThickness;
			vc.position.y	= top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + -outlineThickness;
			vc.position.y	= bottom + outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + lineLength + outlineThickness;
			vc.position.y	= bottom + outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + lineLength + outlineThickness;
			vc.position.y	= top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );
		}
	}

	// Add a glyph quad to the vertex array
	void Text::addGlyphQuad(std::vector<VertexCoords>& vertices, std::vector<ColorA>& colors, Vector2f position, const EE::System::ColorA& color, const EE::Graphics::Glyph& glyph, Float italic, Float outlineThickness, Sizei textureSize, Int32 centerDiffX) {
		Float left		= glyph.bounds.Left;
		Float top		= glyph.bounds.Top;
		Float right		= glyph.bounds.Left + glyph.bounds.Right;
		Float bottom	= glyph.bounds.Top  + glyph.bounds.Bottom;

		Float u1 = static_cast<Float>(glyph.textureRect.Left) / (Float)textureSize.getWidth();
		Float v1 = static_cast<Float>(glyph.textureRect.Top) / (Float)textureSize.getHeight();
		Float u2 = static_cast<Float>(glyph.textureRect.Left + glyph.textureRect.Right) / (Float)textureSize.getWidth();
		Float v2 = static_cast<Float>(glyph.textureRect.Top  + glyph.textureRect.Bottom) / (Float)textureSize.getHeight();
		VertexCoords vc;

		if ( GLi->quadsSupported() ) {

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + position.x + left  - italic * top	- outlineThickness;
			vc.position.y	= position.y + top	- outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + position.x + left  - italic * bottom - outlineThickness;
			vc.position.y	= position.y + bottom - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + position.x + right - italic * bottom - outlineThickness;
			vc.position.y	= position.y + bottom - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + position.x + right - italic * top - outlineThickness;
			vc.position.y	= position.y + top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );
		} else {
			vc.texCoords.x	= u1;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + position.x + left  - italic * bottom - outlineThickness;
			vc.position.y	= position.y + bottom - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + position.x + left  - italic * top	- outlineThickness;
			vc.position.y	= position.y + top	- outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + position.x + right - italic * top - outlineThickness;
			vc.position.y	= position.y + top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u1;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + position.x + left  - italic * bottom - outlineThickness;
			vc.position.y	= position.y + bottom - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v2;
			vc.position.x	= centerDiffX + position.x + right - italic * bottom - outlineThickness;
			vc.position.y	= position.y + bottom - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );

			vc.texCoords.x	= u2;
			vc.texCoords.y	= v1;
			vc.position.x	= centerDiffX + position.x + right - italic * top - outlineThickness;
			vc.position.y	= position.y + top - outlineThickness;
			colors.push_back( color );
			vertices.push_back( vc );
		}
	}
}}

namespace EE { namespace Graphics {

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
	mCachedWidth(0),
	mNumLines(0),
	mLargestLineCharCount(0),
	mFontShadowColor( ColorA( 0, 0, 0, 255 ) ),
	mFlags(0),
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
	mCachedWidth(0),
	mNumLines(0),
	mLargestLineCharCount(0),
	mFontShadowColor( ColorA( 0, 0, 0, 255 ) ),
	mFlags(0),
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
	mCachedWidth(0),
	mNumLines(0),
	mLargestLineCharCount(0),
	mFontShadowColor( ColorA( 0, 0, 0, 255 ) ),
	mFlags(0),
	mFontHeight( mFont->getFontHeight( mRealCharacterSize ) )
{
}

void Text::create(Font * font, const String & text, ColorA FontColor, ColorA FontShadowColor, Uint32 characterSize ) {
	mFont = font;
	mString = text;
	mCharacterSize = characterSize;
	mRealCharacterSize = PixelDensity::dpToPxI(mCharacterSize);
	setColor( FontColor );
	setShadowColor( FontShadowColor );
	mGeometryNeedUpdate = true;
	mCachedWidthNeedUpdate = true;
	ensureGeometryUpdate();
}

void Text::setString(const String& string) {
	if (mString != string) {
		mString = string;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setFont(Font * font) {
	if (mFont != font) {
		mFont = font;

		mRealCharacterSize = PixelDensity::dpToPxI( mCharacterSize );
		mFontHeight = mFont->getFontHeight( mRealCharacterSize );

		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setCharacterSize(unsigned int size) {
	if (mCharacterSize != size) {
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
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setColor(const ColorA & color) {
	setFillColor(color);
}

void Text::setFillColor(const ColorA& color) {
	if (color != mFillColor) {
		mFillColor = color;

		// Change vertex colors directly, no need to update whole geometry
		// (if geometry is updated anyway, we can skip this step)
		if (!mGeometryNeedUpdate) {
			mColors.assign( mVertices.size(), mFillColor );
		}
	}
}

void Text::setOutlineColor(const ColorA& color) {
	if (color != mOutlineColor) {
		mOutlineColor = color;

		// Change vertex colors directly, no need to update whole geometry
		// (if geometry is updated anyway, we can skip this step)
		if (!mGeometryNeedUpdate) {
			mOutlineColors.assign( mOutlineVertices.size(), mOutlineColor );
		}
	}
}

void Text::setOutlineThickness(Float thickness) {
	if (thickness != mOutlineThickness) {
		mOutlineThickness = thickness;
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
}

const ColorA& Text::getFillColor() const {
	return mFillColor;
}

const ColorA &Text::getColor() const {
	return getFillColor();
}

const ColorA& Text::getOutlineColor() const {
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
		}

		// For regular characters, add the advance offset of the glyph
		position.x += static_cast<Float>(mFont->getGlyph(curChar, mRealCharacterSize, bold).advance);
	}

	return position;
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

	return mFont->getLineSpacing(mRealCharacterSize) * ( 0 == mNumLines ? 1 : mNumLines );
}

void Text::draw(const Float & X, const Float & Y, const Vector2f & Scale, const Float & Angle, EE_BLEND_MODE Effect) {
	if ( NULL != mFont ) {
		GlobalBatchRenderer::instance()->draw();
		TextureFactory::instance()->bind( mFont->getTexture(mRealCharacterSize) );
		BlendMode::setMode( Effect );

		if ( mStyle & Shadow ) {
			Uint32 f = mStyle;

			mStyle &= ~Shadow;

			ColorA Col = getFillColor();

			if ( Col.a != 255 ) {
				ColorA ShadowColor = getShadowColor();

				ShadowColor.a = (Uint8)( (Float)ShadowColor.a * ( (Float)Col.a / (Float)255 ) );

				setFillColor( ShadowColor );
			} else {
				setFillColor( getShadowColor() );
			}

			Float pd = PixelDensity::dpToPx(1);

			draw( X + pd, Y + pd, Scale, Angle, Effect );

			mStyle = f;

			setFillColor( Col );
		}

		unsigned int numvert = 0;

		if ( Angle != 0.0f || Scale != 1.0f ) {
			Float cX = (Float) ( (Int32)X );
			Float cY = (Float) ( (Int32)Y );

			GLi->pushMatrix();

			Vector2f Center( cX + mCachedWidth * 0.5f, cY + getTextHeight() * 0.5f );
			GLi->translatef( Center.x , Center.y, 0.f );
			GLi->rotatef( Angle, 0.0f, 0.0f, 1.0f );
			GLi->scalef( Scale.x, Scale.y, 1.0f );
			GLi->translatef( -Center.x + X, -Center.y + Y, 0.f );
		} else {
			GLi->translatef( X, Y, 0 );
		}

		ensureGeometryUpdate();

		numvert = mVertices.size();

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

		if ( Angle != 0.0f || Scale != 1.0f ) {
			GLi->popMatrix();
		} else {
			GLi->translatef( -X, -Y, 0 );
		}
	}
}

void Text::ensureGeometryUpdate() {
	Sizei textureSize = mFont->getTexture(mRealCharacterSize)->getPixelSize();

	if ( textureSize != mTextureSize )
		mGeometryNeedUpdate = true;

	// Do nothing, if geometry has not changed
	if (!mGeometryNeedUpdate)
		return;

	mTextureSize = textureSize;

	// Mark geometry as updated
	mGeometryNeedUpdate = false;

	// Clear the previous geometry
	mVertices.clear();
	mColors.clear();;
	mOutlineVertices.clear();
	mOutlineColors.clear();
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

	cacheWidth();

	switch ( fontHAlignGet( mFlags ) ) {
		case FONT_DRAW_CENTER:
			centerDiffX = (Float)( (Int32)( ( mCachedWidth - mLinesWidth[ Line ] ) * 0.5f ) );
			Line++;
			break;
		case FONT_DRAW_RIGHT:
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
			addLine(mVertices, mColors, x, y, mFillColor, underlineOffset, underlineThickness, 0, textureSize, centerDiffX);

			if (mOutlineThickness != 0)
				addLine(mOutlineVertices, mOutlineColors, x, y, mOutlineColor, underlineOffset, underlineThickness, mOutlineThickness, textureSize, centerDiffX);
		}

		// If we're using the strike through style and there's a new line, draw a line across all characters
		if (strikeThrough && (curChar == L'\n')) {
			addLine(mVertices, mColors, x, y, mFillColor, strikeThroughOffset, underlineThickness, 0, textureSize, centerDiffX);

			if (mOutlineThickness != 0)
				addLine(mOutlineVertices, mOutlineColors, x, y, mOutlineColor, strikeThroughOffset, underlineThickness, mOutlineThickness, textureSize, centerDiffX);
		}

		if ( curChar == L'\n' ) {
			switch ( fontHAlignGet( mFlags ) ) {
				case FONT_DRAW_CENTER:
					centerDiffX = (Float)( (Int32)( ( mCachedWidth - mLinesWidth[ Line ] ) * 0.5f ) );
					break;
				case FONT_DRAW_RIGHT:
					centerDiffX = mCachedWidth - mLinesWidth[ Line ];
					break;
			}

			Line++;
		}

		// Handle special characters
		if ((curChar == ' ') || (curChar == '\t') || (curChar == '\n')) {
			// Update the current bounds (min coordinates)
			minX = std::min(minX, x);
			minY = std::min(minY, y);

			switch (curChar) {
				case ' ':  x += hspace;		break;
				case '\t': x += hspace * 4;	break;
				case '\n': y += vspace; x = 0; break;
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
			addGlyphQuad(mOutlineVertices, mOutlineColors, Vector2f(x, y), mOutlineColor, glyph, italic, mOutlineThickness, textureSize, centerDiffX);

			// Update the current bounds with the outlined glyph bounds
			minX = std::min(minX, x + left   - italic * bottom - mOutlineThickness);
			maxX = std::max(maxX, x + right  - italic * top	- mOutlineThickness);
			minY = std::min(minY, y + top	- mOutlineThickness);
			maxY = std::max(maxY, y + bottom - mOutlineThickness);
		}

		// Extract the current glyph's description
		const Glyph& glyph = mFont->getGlyph(curChar, mRealCharacterSize, bold);

		// Add the glyph to the vertices
		addGlyphQuad(mVertices, mColors, Vector2f(x, y), mFillColor, glyph, italic, 0, textureSize, centerDiffX);

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
		addLine(mVertices, mColors, x, y, mFillColor, underlineOffset, underlineThickness, 0, textureSize, centerDiffX);

		if (mOutlineThickness != 0)
			addLine(mOutlineVertices, mOutlineColors, x, y, mOutlineColor, underlineOffset, underlineThickness, mOutlineThickness, textureSize, centerDiffX);
	}

	// If we're using the strike through style, add the last line across all characters
	if (strikeThrough && (x > 0)) {
		addLine(mVertices, mColors, x, y, mFillColor, strikeThroughOffset, underlineThickness, 0, textureSize, centerDiffX);

		if (mOutlineThickness != 0)
			addLine(mOutlineVertices, mOutlineColors, x, y, mOutlineColor, strikeThroughOffset, underlineThickness, mOutlineThickness, textureSize, centerDiffX);
	}

	// Update the bounding rectangle
	mBounds.Left = minX;
	mBounds.Top = minY;
	mBounds.Right = maxX - minX;
	mBounds.Bottom = maxY - minY;
}

const ColorA& Text::getShadowColor() const {
	return mFontShadowColor;
}

void Text::setShadowColor(const ColorA& color) {
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

void Text::setFlags( const Uint32& flags ) {
	if ( mFlags != flags ) {
		mFlags = flags;
		mGeometryNeedUpdate = true;
	}
}

const Uint32& Text::getFlags() const {
	return mFlags;
}

void Text::cacheWidth() {
	if ( !mCachedWidthNeedUpdate )
		return;

	if ( NULL != mFont && mString.size() ) {
		mFont->cacheWidth( mString, mRealCharacterSize, (mStyle & Bold), mOutlineThickness, mLinesWidth, mCachedWidth, mNumLines, mLargestLineCharCount );
		mCachedWidthNeedUpdate = false;
	} else {
		mCachedWidth = 0;
	}
}

void Text::setStyleConfig( const FontStyleConfig& styleConfig ) {
	setFont( styleConfig.Font );
	setCharacterSize( styleConfig.CharacterSize );
	setFillColor( styleConfig.Color );
	setStyle( styleConfig.Style );
	setOutlineThickness( styleConfig.OutlineThickness );
	setOutlineColor( styleConfig.OutlineColor );
}

}}
