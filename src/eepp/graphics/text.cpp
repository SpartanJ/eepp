#include <algorithm>
#include <cmath>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <limits>

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

Float Text::getTextWidth( Font* font, const Uint32& fontSize, const String& string,
						  const Uint32& style, const Uint32& tabWidth,
						  const Float& outlineThickness ) {
	return getTextWidth<String>( font, fontSize, string, style, tabWidth, outlineThickness );
}

Float Text::getTextWidth( Font* font, const Uint32& fontSize, const String::View& string,
						  const Uint32& style, const Uint32& tabWidth,
						  const Float& outlineThickness ) {
	return getTextWidth<String::View>( font, fontSize, string, style, tabWidth, outlineThickness );
}

Sizef Text::draw( const String& string, const Vector2f& pos, Font* font, Float fontSize,
				  const Color& fontColor, Uint32 style, Float outlineThickness,
				  const Color& outlineColor, const Color& shadowColor, const Vector2f& shadowOffset,
				  const Uint32& tabWidth ) {
	return draw<String>( string, pos, font, fontSize, fontColor, style, outlineThickness,
						 outlineColor, shadowColor, shadowOffset, tabWidth );
}

Sizef Text::draw( const String& string, const Vector2f& pos, const FontStyleConfig& config,
				  const Uint32& tabWidth ) {
	return draw<String>( string, pos, config, tabWidth );
}

Sizef Text::draw( const String::View& string, const Vector2f& pos, Font* font, Float fontSize,
				  const Color& fontColor, Uint32 style, Float outlineThickness,
				  const Color& outlineColor, const Color& shadowColor, const Vector2f& shadowOffset,
				  const Uint32& tabWidth ) {
	return draw<String::View>( string, pos, font, fontSize, fontColor, style, outlineThickness,
							   outlineColor, shadowColor, shadowOffset, tabWidth );
}

Sizef Text::draw( const String::View& string, const Vector2f& pos, const FontStyleConfig& config,
				  const Uint32& tabWidth ) {
	return draw<String::View>( string, pos, config, tabWidth );
}

Text* Text::New() {
	return eeNew( Text, () );
}

Text* Text::New( const String& string, Font* font, unsigned int characterSize ) {
	return eeNew( Text, ( string, font, characterSize ) );
}

Text* Text::New( Font* font, unsigned int characterSize ) {
	return eeNew( Text, ( font, characterSize ) );
}

template <typename StringType>
Sizef Text::draw( const StringType& string, const Vector2f& pos, Font* font, Float fontSize,
				  const Color& fontColor, Uint32 style, Float outlineThickness,
				  const Color& outlineColor, const Color& shadowColor, const Vector2f& shadowOffset,
				  const Uint32& tabWidth ) {
	Vector2f cpos{ pos };
	String::StringBaseType ch;
	String::StringBaseType prevChar = 0;
	bool isBold = ( style & Text::Bold ) != 0;
	bool isItalic = ( style & Text::Italic ) != 0;
	Float kerning = 0;
	Float width = 0;
	Float height = font->getFontHeight( fontSize );
	Sizef size{ 0, height };
	size_t ssize = string.size();
	BatchRenderer* BR = GlobalBatchRenderer::instance();
	BR->setBlendMode( BlendMode::Alpha() );

	auto drawUnderline = [&]() {
		Float underlineOffset = font->getUnderlinePosition( fontSize );
		Float underlineThickness = font->getUnderlineThickness( fontSize );
		Float top =
			cpos.y + std::floor( fontSize + underlineOffset - ( underlineThickness / 2 ) + 0.5f );
		Float bottom = top + std::floor( underlineThickness + 0.5f );
		if ( style & Text::Shadow ) {
			BR->quadsSetTexCoord( 0, 0, 1, 1 );
			BR->quadsSetColor( shadowColor );
			BR->batchQuad( Rectf( pos.x + shadowOffset.x, top + shadowOffset.y,
								  pos.x + width + shadowOffset.x, bottom + +shadowOffset.y ) );
		}
		if ( outlineThickness ) {
			BR->quadsSetTexCoord( 0, 0, 1, 1 );
			BR->quadsSetColor( outlineColor );
			BR->batchQuad( Rectf( pos.x - outlineThickness, top - outlineThickness,
								  pos.x + width + outlineThickness, bottom + outlineThickness ) );
		}
		BR->quadsSetTexCoord( 0, 0, 1, 1 );
		BR->quadsSetColor( fontColor );
		BR->batchQuad( Rectf( pos.x, top, pos.x + width, bottom ) );
	};

	auto drawStrikeThrough = [&]() {
		Rectf xBounds = font->getGlyph( L'x', fontSize, isBold, isItalic ).bounds;
		Float strikeThroughOffset = xBounds.Top + xBounds.Bottom * 0.5f;
		Float underlineThickness = font->getUnderlineThickness( fontSize );
		Float top = std::floor( cpos.y + fontSize + strikeThroughOffset -
								( underlineThickness / 2 ) + 0.5f );
		Float bottom = top + std::floor( underlineThickness + 0.5f );
		if ( style & Text::Shadow ) {
			BR->quadsSetTexCoord( 0, 0, 1, 1 );
			BR->quadsSetColor( shadowColor );
			BR->batchQuad( Rectf( pos.x + shadowOffset.x, top + shadowOffset.y,
								  pos.x + width + shadowOffset.x, bottom + +shadowOffset.y ) );
		}
		if ( outlineThickness ) {
			BR->quadsSetTexCoord( 0, 0, 1, 1 );
			BR->quadsSetColor( outlineColor );
			BR->batchQuad( Rectf( pos.x - outlineThickness, top - outlineThickness,
								  pos.x + width + outlineThickness, bottom + outlineThickness ) );
		}
		BR->quadsSetTexCoord( 0, 0, 1, 1 );
		BR->quadsSetColor( fontColor );
		BR->batchQuad( Rectf( pos.x, top, pos.x + width, bottom ) );
	};

	for ( size_t i = 0; i < ssize; ++i ) {
		ch = string[i];

		switch ( ch ) {
			case '\r':
				continue;
			case '\t': {
				Float advance =
					font->getGlyph( ' ', fontSize, isBold, isItalic ).advance * tabWidth;
				width += advance;
				cpos.x += advance;
				prevChar = ch;
				continue;
			}
			case ' ': {
				Float advance = font->getGlyph( ' ', fontSize, isBold, isItalic ).advance;
				width += advance;
				cpos.x += advance;
				prevChar = ch;
				continue;
			}
			case '\n': {
				if ( style & Text::Underlined )
					drawUnderline();
				if ( style & Text::StrikeThrough )
					drawStrikeThrough();

				size.x = eemax( width, cpos.x );
				width = 0;
				cpos.y += height;
				if ( i != ssize - 1 )
					size.y += height;
				continue;
			}
		}

		if ( style & Text::Shadow ) {
			auto* gds = font->getGlyphDrawable( ch, fontSize, isBold, isItalic, outlineThickness );
			gds->setDrawMode( isItalic ? GlyphDrawable::DrawMode::TextItalic
									   : GlyphDrawable::DrawMode::Text );
			gds->setPosition( cpos + shadowOffset );
			gds->setColor( shadowColor );
			gds->draw();
		}

		if ( outlineThickness != 0.f ) {
			auto* gdo = font->getGlyphDrawable( ch, fontSize, isBold, isItalic, outlineThickness );
			gdo->setDrawMode( isItalic ? GlyphDrawable::DrawMode::TextItalic
									   : GlyphDrawable::DrawMode::Text );
			gdo->setPosition( cpos );
			gdo->setColor( outlineColor );
			gdo->draw();
		}

		auto* gd = font->getGlyphDrawable( ch, fontSize, isBold, isItalic );
		gd->setDrawMode( isItalic ? GlyphDrawable::DrawMode::TextItalic
								  : GlyphDrawable::DrawMode::Text );
		gd->setPosition( cpos );
		gd->setColor( fontColor );
		gd->draw();

		cpos.x += gd->getAdvance();
		width += gd->getAdvance();

		if ( !font->isMonospace() ) {
			kerning =
				font->getKerning( prevChar, ch, fontSize, isBold, isItalic, outlineThickness );
			cpos.x += kerning;
			width += kerning;
		}

		prevChar = ch;
	}

	if ( ( style & Text::Underlined ) && width != 0 )
		drawUnderline();

	if ( ( style & Text::StrikeThrough ) && width != 0 )
		drawStrikeThrough();

	size.x = eemax( width, size.x );
	return size;
}

template <typename StringType>
Sizef Text::draw( const StringType& string, const Vector2f& pos, const FontStyleConfig& config,
				  const Uint32& tabWidth ) {
	return draw<StringType>( string, pos, config.Font, config.CharacterSize, config.FontColor,
							 config.Style, config.OutlineThickness, config.OutlineColor,
							 config.ShadowColor, config.ShadowOffset, tabWidth );
}

Text::Text() {}

Text::Text( const String& string, Font* font, unsigned int characterSize ) :
	Text( font, characterSize ) {
	mString = string;
	invalidate();
}

Text::Text( Font* font, unsigned int characterSize ) {
	mFontStyleConfig.Font = font;
	mFontStyleConfig.CharacterSize = characterSize;
	mFontHeight = mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
	if ( !mFontStyleConfig.Font->isScalable() )
		mFontStyleConfig.CharacterSize = mFontHeight;
}

void Text::create( Font* font, const String& text, Color FontColor, Color FontShadowColor,
				   Uint32 characterSize ) {
	mFontStyleConfig.Font = font;
	mFontStyleConfig.CharacterSize = characterSize;

	mFontHeight = mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
	if ( !mFontStyleConfig.Font->isScalable() )
		mFontStyleConfig.CharacterSize = mFontHeight;
	mString = text;
	setFillColor( FontColor );
	setShadowColor( FontShadowColor );
	invalidate();
}

void Text::onNewString() {
	mColorsNeedUpdate = true;
	mGeometryNeedUpdate = true;
	mCachedWidthNeedUpdate = true;
	mContainsColorEmoji = false;
	if ( FontManager::instance()->getColorEmojiFont() != nullptr ) {
		if ( mFontStyleConfig.Font->getType() == FontType::TTF ) {
			FontTrueType* fontTrueType = static_cast<FontTrueType*>( mFontStyleConfig.Font );
			if ( fontTrueType->isColorEmojiFont() || !fontTrueType->isEmojiFont() )
				mContainsColorEmoji = Font::containsEmojiCodePoint( mString );
		}
	}
}

void Text::setString( const String& string ) {
	if ( mString != string ) {
		mString = string;
		onNewString();
	}
}

void Text::setString( String&& string ) {
	if ( mString != string ) {
		mString = std::move( string );
		onNewString();
	}
}

void Text::setFont( Font* font ) {
	if ( NULL != font && mFontStyleConfig.Font != font ) {
		mFontStyleConfig.Font = font;

		mFontHeight = mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
		if ( !mFontStyleConfig.Font->isScalable() )
			mFontStyleConfig.CharacterSize = mFontHeight;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setFontSize( unsigned int size ) {
	if ( NULL != mFontStyleConfig.Font && mFontStyleConfig.CharacterSize != size ) {
		mFontStyleConfig.CharacterSize = size;

		mFontHeight = mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
		if ( !mFontStyleConfig.Font->isScalable() )
			mFontStyleConfig.CharacterSize = mFontHeight;

		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setStyle( Uint32 style ) {
	if ( mFontStyleConfig.Style != style ) {
		mFontStyleConfig.Style = style;
		mColorsNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setColor( const Color& color ) {
	setFillColor( color );
}

void Text::setFillColor( const Color& color ) {
	if ( color != mFontStyleConfig.FontColor ) {
		mFontStyleConfig.FontColor = color;
		mColorsNeedUpdate = true;
	}
}

void Text::setOutlineColor( const Color& color ) {
	if ( color != mFontStyleConfig.OutlineColor ) {
		mFontStyleConfig.OutlineColor = color;
		mColorsNeedUpdate = true;
	}
}

void Text::setOutlineThickness( Float thickness ) {
	if ( thickness != mFontStyleConfig.OutlineThickness ) {
		mFontStyleConfig.OutlineThickness = thickness;
		mColorsNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::transformText( const TextTransform::Value& transform ) {
	switch ( transform ) {
		case TextTransform::LowerCase:
			setString( String::toLower( mString ) );
			break;
		case TextTransform::UpperCase:
			setString( String::toUpper( mString ) );
			break;
		case TextTransform::Capitalize:
			setString( String::capitalize( mString ) );
			break;
		default:
			break;
	}
}

String& Text::getString() {
	return mString;
}

Font* Text::getFont() const {
	return mFontStyleConfig.Font;
}

unsigned int Text::getCharacterSize() const {
	return mFontStyleConfig.CharacterSize;
}

const Uint32& Text::getFontHeight() const {
	return mFontHeight;
}

Uint32 Text::getStyle() const {
	return mFontStyleConfig.Style;
}

void Text::setAlpha( const Uint8& alpha ) {
	std::size_t s = mColors.size();
	for ( Uint32 i = 0; i < s; i++ ) {
		mColors[i].a = alpha;
	}
	mFontStyleConfig.FontColor.a = alpha;
}

const Color& Text::getFillColor() const {
	return mFontStyleConfig.FontColor;
}

const Color& Text::getColor() const {
	return getFillColor();
}

const Color& Text::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

Float Text::getOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

Vector2f Text::findCharacterPos( std::size_t index ) const {
	// Make sure that we have a valid font
	if ( !mFontStyleConfig.Font || mString.empty() )
		return Vector2f();

	// Adjust the index if it's out of range
	if ( index > mString.size() )
		index = mString.size();

	return Text::findCharacterPos( index, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize,
								   mString, mFontStyleConfig.Style, mTabWidth,
								   mFontStyleConfig.OutlineThickness );
}

Int32 Text::findCharacterFromPos( const Vector2i& pos, bool nearest ) const {
	if ( NULL == mFontStyleConfig.Font || mString.empty() )
		return 0;

	return Text::findCharacterFromPos(
		pos, nearest, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, mString,
		mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness );
}

static bool isStopSelChar( Uint32 c ) {
	return ( !String::isCharacter( c ) && !String::isNumber( c ) ) || ' ' == c || '.' == c ||
		   ',' == c || ';' == c || ':' == c || '\n' == c || '"' == c || '\'' == c || '\t' == c;
}

void Text::findWordFromCharacterIndex( Int32 characterIndex, Int32& initCur, Int32& endCur ) const {
	initCur = 0;
	endCur = mString.size();

	for ( std::size_t i = characterIndex; i < mString.size(); i++ ) {
		if ( isStopSelChar( mString[i] ) ) {
			endCur = i;
			break;
		}
	}

	if ( 0 == characterIndex ) {
		initCur = 0;
	}

	if ( characterIndex >= (Int32)mString.size() ) {
		characterIndex = mString.size() - 1;
	}

	for ( Int32 i = characterIndex; i >= 0; i-- ) {
		if ( isStopSelChar( mString[i] ) ) {
			initCur = i + 1;
			break;
		}
	}

	if ( initCur == endCur ) {
		initCur = endCur = -1;
	}
}

template <typename StringType>
Float Text::getTextWidth( Font* font, const Uint32& fontSize, const StringType& string,
						  const Uint32& style, const Uint32& tabWidth,
						  const Float& outlineThickness ) {
	if ( NULL == font || string.empty() )
		return 0;
	Float width = 0;
	Float maxWidth = 0;
	String::StringBaseType rune;
	Uint32 prevChar = 0;
	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Text::Italic ) != 0;
	Float hspace = static_cast<Float>(
		font->getGlyph( L' ', fontSize, bold, italic, outlineThickness ).advance );
	for ( std::size_t i = 0; i < string.size(); ++i ) {
		rune = string.at( i );
		Glyph glyph = font->getGlyph( rune, fontSize, bold, italic, outlineThickness );
		if ( rune != '\r' && rune != '\t' ) {
			width += font->getKerning( prevChar, rune, fontSize, bold, italic, outlineThickness );
			width += glyph.advance;
		} else if ( rune == '\t' ) {
			width += hspace * tabWidth;
		}
		if ( rune == '\n' ) {
			width = 0;
		}
		maxWidth = eemax( width, maxWidth );
		prevChar = rune;
	}
	return maxWidth;
}

Vector2f Text::findCharacterPos( std::size_t index, Font* font, const Uint32& fontSize,
								 const String& string, const Uint32& style, const Uint32& tabWidth,
								 const Float& outlineThickness ) {
	// Make sure that we have a valid font
	if ( !font )
		return Vector2f();

	// Adjust the index if it's out of range
	if ( index > string.size() )
		index = string.size();

	// Precompute the variables needed by the algorithm
	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Italic ) != 0;
	Float hspace = static_cast<Float>(
		font->getGlyph( L' ', fontSize, bold, italic, outlineThickness ).advance );
	Float vspace = static_cast<Float>( font->getLineSpacing( fontSize ) );

	// Compute the position
	Vector2f position;
	Uint32 prevChar = 0;
	for ( std::size_t i = 0; i < index; ++i ) {
		String::StringBaseType curChar = string[i];

		// Apply the kerning offset
		position.x += static_cast<Float>(
			font->getKerning( prevChar, curChar, fontSize, bold, italic, outlineThickness ) );
		prevChar = curChar;

		// Handle special characters
		switch ( curChar ) {
			case ' ':
				position.x += hspace;
				continue;
			case '\t':
				position.x += hspace * tabWidth;
				continue;
			case '\n':
				position.y += vspace;
				position.x = 0;
				continue;
			case '\r':
				continue;
		}

		// For regular characters, add the advance offset of the glyph
		position.x += static_cast<Float>(
			font->getGlyph( curChar, fontSize, bold, italic, outlineThickness ).advance );
	}

	return position;
}

Int32 Text::findCharacterFromPos( const Vector2i& pos, bool returnNearest, Font* font,
								  const Uint32& fontSize, const String& string, const Uint32& style,
								  const Uint32& tabWidth, const Float& outlineThickness ) {
	if ( NULL == font )
		return 0;

	Float vspace = font->getLineSpacing( fontSize );
	Float width = 0, lWidth = 0, height = vspace, lHeight = 0;
	Uint32 rune;
	Uint32 prevChar = 0;
	Int32 nearest = -1;
	Int32 minDist = std::numeric_limits<Int32>::max();
	Int32 curDist = -1;
	std::size_t tSize = string.size();
	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Italic ) != 0;
	Vector2f fpos( pos.asFloat() );

	Float hspace = static_cast<Float>(
		font->getGlyph( L' ', fontSize, bold, italic, outlineThickness ).advance );

	for ( std::size_t i = 0; i < tSize; ++i ) {
		rune = string[i];
		Glyph glyph = font->getGlyph( rune, fontSize, bold, italic, outlineThickness );

		lWidth = width;

		if ( rune != '\r' && rune != '\t' ) {
			width += font->getKerning( prevChar, rune, fontSize, bold, italic, outlineThickness );
			prevChar = rune;
			width += glyph.advance;
		} else if ( rune == '\t' ) {
			width += hspace * tabWidth;
		}
		if ( rune == '\n' ) {
			lWidth = 0;
			width = 0;
		}

		if ( pos.x <= width && pos.x >= lWidth && pos.y <= height && pos.y >= lHeight ) {
			if ( i + 1 < tSize ) {
				Int32 tcurDist = eeabs( pos.x - lWidth );
				Int32 nextDist = eeabs( pos.x - width );
				if ( nextDist < tcurDist )
					return i + 1;
			}
			return i;
		}

		if ( returnNearest ) {
			curDist = eeabs( fpos.distance( Vector2f( width - ( width - lWidth ) * 0.5f,
													  height - ( height - lHeight ) * 0.5f ) ) );
			if ( curDist < minDist ) {
				nearest = i;
				minDist = curDist;
			}
		}

		if ( rune == '\n' ) {
			lHeight = height;
			height += vspace;
			if ( pos.x > width && pos.y <= lHeight ) {
				return i;
			}
		}
	}

	if ( pos.x >= width )
		return tSize;
	return nearest;
}

void Text::updateWidthCache() {
	if ( NULL == mFontStyleConfig.Font || mString.empty() )
		return;

	mLinesWidth.clear();

	Float width = 0;
	Float maxWidth = 0;
	Uint32 rune;
	Uint32 prevChar = 0;
	bool bold = ( mFontStyleConfig.Style & Bold ) != 0;
	bool italic = ( mFontStyleConfig.Style & Italic ) != 0;

	Float hspace = static_cast<Float>( mFontStyleConfig.Font
										   ->getGlyph( L' ', mFontStyleConfig.CharacterSize, bold,
													   italic, mFontStyleConfig.OutlineThickness )
										   .advance );

	size_t size = mString.size();
	for ( std::size_t i = 0; i < size; ++i ) {
		rune = mString[i];
		const Glyph& glyph = mFontStyleConfig.Font->getGlyph(
			rune, mFontStyleConfig.CharacterSize, bold, italic, mFontStyleConfig.OutlineThickness );
		if ( rune != '\r' && rune != '\t' ) {
			width += mFontStyleConfig.Font->getKerning( prevChar, rune,
														mFontStyleConfig.CharacterSize, bold,
														italic, mFontStyleConfig.OutlineThickness );
			width += glyph.advance;
		} else if ( rune == '\t' ) {
			width += hspace * mTabWidth;
		}

		if ( rune == '\n' ) {
			mLinesWidth.push_back( width - glyph.advance );
			width = 0;
		}

		if ( width > maxWidth )
			maxWidth = width;
		prevChar = rune;
	}

	if ( !mString.empty() && mString[mString.size() - 1] != '\n' )
		mLinesWidth.push_back( width );

	mCachedWidth = maxWidth;
}

void Text::wrapText( const Uint32& maxWidth ) {
	if ( !mString.size() || NULL == mFontStyleConfig.Font )
		return;

	Float tCurWidth = 0.f;
	Float tWordWidth = 0.f;
	Float tMaxWidth = (Float)maxWidth;
	String::StringBaseType* tChar = &mString[0];
	String::StringBaseType* tLastSpace = NULL;
	Uint32 prevChar = 0;
	bool bold = ( mFontStyleConfig.Style & Bold ) != 0;
	bool italic = ( mFontStyleConfig.Style & Italic ) != 0;

	Float hspace = static_cast<Float>( mFontStyleConfig.Font
										   ->getGlyph( L' ', mFontStyleConfig.CharacterSize, bold,
													   italic, mFontStyleConfig.OutlineThickness )
										   .advance );

	while ( *tChar ) {
		Glyph pChar = mFontStyleConfig.Font->getGlyph( *tChar, mFontStyleConfig.CharacterSize, bold,
													   italic, mFontStyleConfig.OutlineThickness );

		Float fCharWidth = (Float)pChar.advance;

		if ( ( *tChar ) == '\t' )
			fCharWidth += hspace * mTabWidth;
		else if ( ( *tChar ) == '\r' )
			fCharWidth = 0;

		// Add the new char width to the current word width
		tWordWidth += fCharWidth;

		if ( *tChar != '\r' ) {
			tWordWidth += mFontStyleConfig.Font->getKerning(
				prevChar, *tChar, mFontStyleConfig.CharacterSize, bold, italic,
				mFontStyleConfig.OutlineThickness );
			prevChar = *tChar;
		}

		if ( ' ' == *tChar || '\0' == *( tChar + 1 ) ) {

			// If current width plus word width is minor to the max width, continue adding
			if ( tCurWidth + tWordWidth < tMaxWidth ) {
				tCurWidth += tWordWidth;
				tLastSpace = tChar;

				tChar++;
			} else {
				// If it was an space before, replace that space for an new line
				// Start counting from the new line first character
				if ( NULL != tLastSpace ) {
					*tLastSpace = '\n';
					tChar = tLastSpace + 1;
				} else { // The word is larger than the current possible width
					*tChar = '\n';
				}

				if ( '\0' == *( tChar + 1 ) )
					tChar++;

				// Set the last spaces as null, because is a new line
				tLastSpace = NULL;

				// New line, new current width
				tCurWidth = 0.f;
			}

			// New word, so we reset the current word width
			tWordWidth = 0.f;
		} else if ( '\n' == *tChar ) {
			tWordWidth = 0.f;
			tCurWidth = 0.f;
			tLastSpace = NULL;
			tChar++;
		} else {
			tChar++;
		}
	}

	invalidate();
}

void Text::invalidateColors() {
	mColorsNeedUpdate = true;
}

void Text::invalidate() {
	mCachedWidthNeedUpdate = true;
	mGeometryNeedUpdate = true;
	mColorsNeedUpdate = true;
}

void Text::setTabWidth( const Uint32& tabWidth ) {
	if ( mTabWidth != tabWidth ) {
		mTabWidth = tabWidth;

		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

const Uint32& Text::getTabWidth() const {
	return mTabWidth;
}

Color Text::getBackgroundColor() const {
	return mBackgroundColor;
}

void Text::setBackgroundColor( const Color& backgroundColor ) {
	mBackgroundColor = backgroundColor;
}

const Vector2f& Text::getShadowOffset() const {
	return mFontStyleConfig.ShadowOffset;
}

void Text::setShadowOffset( const Vector2f& shadowOffset ) {
	mFontStyleConfig.ShadowOffset = shadowOffset;
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

	return NULL != mFontStyleConfig.Font
			   ? mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize ) *
					 ( mLinesWidth.empty() ? 1 : mLinesWidth.size() )
			   : 0;
}

Float Text::getLineSpacing() {
	return NULL != mFontStyleConfig.Font
			   ? mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize )
			   : 0;
}

void Text::draw( const Float& X, const Float& Y, const Vector2f& scale, const Float& rotation,
				 BlendMode effect, const OriginPoint& rotationCenter,
				 const OriginPoint& scaleCenter, const std::vector<Color>& colors,
				 const std::vector<Color>& outlineColors, const Color& backgroundColor ) {
	unsigned int numvert = mVertices.size();

	if ( 0 == numvert )
		return;

	GlobalBatchRenderer::instance()->draw();

	if ( rotation != 0.0f || scale != 1.0f ) {
		Float cX = (Float)( (Int32)X );
		Float cY = (Float)( (Int32)Y );
		Vector2f Center( cX + mCachedWidth * 0.5f, cY + getTextHeight() * 0.5f );

		GLi->pushMatrix();

		Vector2f center = Center;
		if ( OriginPoint::OriginTopLeft == scaleCenter.OriginType )
			center = Vector2f( cX, cY );
		else if ( OriginPoint::OriginCustom == scaleCenter.OriginType )
			center = Vector2f( scaleCenter.x, scaleCenter.y );

		GLi->translatef( center.x, center.y, 0.f );
		GLi->scalef( scale.x, scale.y, 1.0f );
		GLi->translatef( -center.x, -center.y, 0.f );

		center = Center;
		if ( OriginPoint::OriginTopLeft == rotationCenter.OriginType )
			center = Vector2f( cX, cY );
		else if ( OriginPoint::OriginCustom == rotationCenter.OriginType )
			center = Vector2f( rotationCenter.x, rotationCenter.y );

		GLi->translatef( center.x, center.y, 0.f );
		GLi->rotatef( rotation, 0.0f, 0.0f, 1.0f );
		GLi->translatef( -center.x + cX, -center.y + cY, 0.f );
	} else {
		GLi->translatef( X, Y, 0 );
	}

	if ( backgroundColor != Color::Transparent ) {
		Primitives p;
		p.setForceDraw( true );
		p.setColor( backgroundColor );
		p.drawRectangle( getLocalBounds() );
	}

	Texture* texture = mFontStyleConfig.Font->getTexture( mFontStyleConfig.CharacterSize );
	if ( !texture )
		return;
	texture->bind();
	BlendMode::setMode( effect );

	Uint32 alloc = numvert * sizeof( VertexCoords );
	Uint32 allocC = numvert * GLi->quadVertexs();

	if ( 0 != mFontStyleConfig.OutlineThickness ) {
		GLi->colorPointer( 4, GL_UNSIGNED_BYTE, 0,
						   reinterpret_cast<const char*>( outlineColors.data() ), allocC );
		GLi->texCoordPointer( 2, GL_FP, sizeof( VertexCoords ),
							  reinterpret_cast<char*>( &mOutlineVertices[0] ), alloc );
		GLi->vertexPointer( 2, GL_FP, sizeof( VertexCoords ),
							reinterpret_cast<char*>( &mOutlineVertices[0] ) + sizeof( Float ) * 2,
							alloc );

		if ( GLi->quadsSupported() ) {
			GLi->drawArrays( GL_QUADS, 0, numvert );
		} else {
			GLi->drawArrays( GL_TRIANGLES, 0, numvert );
		}
	}

	GLi->colorPointer( 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<const char*>( colors.data() ),
					   allocC );
	GLi->texCoordPointer( 2, GL_FP, sizeof( VertexCoords ),
						  reinterpret_cast<char*>( &mVertices[0] ), alloc );
	GLi->vertexPointer( 2, GL_FP, sizeof( VertexCoords ),
						reinterpret_cast<char*>( &mVertices[0] ) + sizeof( Float ) * 2, alloc );

	if ( GLi->quadsSupported() ) {
		GLi->drawArrays( GL_QUADS, 0, numvert );
	} else {
		GLi->drawArrays( GL_TRIANGLES, 0, numvert );
	}

	if ( rotation != 0.0f || scale != 1.0f ) {
		GLi->popMatrix();
	} else {
		GLi->translatef( -X, -Y, 0 );
	}
}

void Text::draw( const Float& X, const Float& Y, const Vector2f& scale, const Float& rotation,
				 BlendMode effect, const OriginPoint& rotationCenter,
				 const OriginPoint& scaleCenter ) {
	if ( NULL == mFontStyleConfig.Font )
		return;

	ensureColorUpdate();
	ensureGeometryUpdate();

	if ( mFontStyleConfig.Style & Shadow ) {
		std::vector<Color> colors;
		Color shadowColor( getShadowColor() );
		if ( getFillColor().a != 255 )
			shadowColor.a =
				(Uint8)( (Float)shadowColor.a * ( (Float)getFillColor().a / (Float)255 ) );
		colors.assign( mColors.size(), shadowColor );
		draw( X + mFontStyleConfig.ShadowOffset.x, Y + mFontStyleConfig.ShadowOffset.y, scale,
			  rotation, effect, rotationCenter, scaleCenter, colors, {}, Color::Transparent );
	}

	draw( X, Y, scale, rotation, effect, rotationCenter, scaleCenter, mColors, mOutlineColors,
		  mBackgroundColor );
}

void Text::ensureGeometryUpdate() {
	if ( mCachedWidthNeedUpdate && mAlign != TEXT_ALIGN_LEFT )
		cacheWidth();

	// Do nothing, if geometry has not changed
	if ( !mGeometryNeedUpdate )
		return;

	// Mark geometry as updated
	mGeometryNeedUpdate = false;

	// Clear the previous geometry
	mVertices.clear();
	mOutlineVertices.clear();

	if ( mCachedWidthNeedUpdate )
		mLinesWidth.clear();

	mBounds = Rectf();

	// No font or text: nothing to draw
	if ( !mFontStyleConfig.Font || mString.empty() )
		return;

	mVertices.reserve( mString.size() * GLi->quadVertexs() );

	if ( mFontStyleConfig.OutlineThickness != 0.f )
		mOutlineVertices.reserve( mString.size() * GLi->quadVertexs() );

	// Compute values related to the text style
	bool bold = ( mFontStyleConfig.Style & Bold ) != 0;
	bool reqItalic = ( mFontStyleConfig.Style & Italic ) != 0;
	bool underlined = ( mFontStyleConfig.Style & Underlined ) != 0;
	bool strikeThrough = ( mFontStyleConfig.Style & StrikeThrough ) != 0;
	Float italic = reqItalic && !mFontStyleConfig.Font->hasItalic() ? 0.208f : 0.f; // 12 degrees
	Float underlineOffset =
		mFontStyleConfig.Font->getUnderlinePosition( mFontStyleConfig.CharacterSize );
	Float underlineThickness =
		mFontStyleConfig.Font->getUnderlineThickness( mFontStyleConfig.CharacterSize );

	// Compute the location of the strike through dynamically
	// We use the center point of the lowercase 'x' glyph as the reference
	// We reuse the underline thickness as the thickness of the strike through as well
	Float strikeThroughOffset = 0;
	if ( strikeThrough ) {
		Rectf xBounds =
			mFontStyleConfig.Font->getGlyph( L'x', mFontStyleConfig.CharacterSize, bold, reqItalic )
				.bounds;
		strikeThroughOffset = xBounds.Top + xBounds.Bottom / 2.f;
	}

	// Precompute the variables needed by the algorithm
	Float hspace = static_cast<Float>(
		mFontStyleConfig.Font->getGlyph( L' ', mFontStyleConfig.CharacterSize, bold, reqItalic )
			.advance );
	Float vspace = static_cast<Float>(
		mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize ) );
	Float x = 0.f;
	Float y = static_cast<Float>( mFontStyleConfig.CharacterSize );

	// Create one quad for each character
	Float minX = static_cast<Float>( mFontStyleConfig.CharacterSize );
	Float minY = static_cast<Float>( mFontStyleConfig.CharacterSize );
	Float maxX = 0.f;
	Float maxY = 0.f;
	Float maxW = 0.f;
	Uint32 prevChar = 0;

	size_t size = mString.size();
	Float centerDiffX = 0;
	unsigned int line = 0;

	switch ( Font::getHorizontalAlign( mAlign ) ) {
		case TEXT_ALIGN_CENTER:
			centerDiffX = line < mLinesWidth.size()
							  ? (Float)( (Int32)( ( mCachedWidth - mLinesWidth[line] ) * 0.5f ) )
							  : 0.f;
			line++;
			break;
		case TEXT_ALIGN_RIGHT:
			centerDiffX = line < mLinesWidth.size() ? mCachedWidth - mLinesWidth[line] : 0.f;
			line++;
			break;
	}

	for ( std::size_t i = 0; i < size; ++i ) {
		Uint32 curChar = mString[i];

		// Apply the kerning offset
		x +=
			mFontStyleConfig.Font->getKerning( prevChar, curChar, mFontStyleConfig.CharacterSize,
											   bold, reqItalic, mFontStyleConfig.OutlineThickness );
		prevChar = curChar;

		// If we're using the underlined style and there's a new line, draw a line
		if ( underlined && ( curChar == L'\n' ) ) {
			addLine( mVertices, x, y, underlineOffset, underlineThickness, 0, centerDiffX );

			if ( mFontStyleConfig.OutlineThickness != 0 )
				addLine( mOutlineVertices, x, y, underlineOffset, underlineThickness,
						 mFontStyleConfig.OutlineThickness, centerDiffX );
		}

		// If we're using the strike through style and there's a new line, draw a line across all
		// characters
		if ( strikeThrough && ( curChar == L'\n' ) ) {
			addLine( mVertices, x, y, strikeThroughOffset, underlineThickness, 0, centerDiffX );

			if ( mFontStyleConfig.OutlineThickness != 0 )
				addLine( mOutlineVertices, x, y, strikeThroughOffset, underlineThickness,
						 mFontStyleConfig.OutlineThickness, centerDiffX );
		}

		if ( curChar == L'\n' ) {
			switch ( Font::getHorizontalAlign( mAlign ) ) {
				case TEXT_ALIGN_CENTER:
					centerDiffX =
						line < mLinesWidth.size()
							? (Float)( (Int32)( ( mCachedWidth - mLinesWidth[line] ) * 0.5f ) )
							: 0.f;
					break;
				case TEXT_ALIGN_RIGHT:
					centerDiffX =
						line < mLinesWidth.size() ? mCachedWidth - mLinesWidth[line] : 0.f;
					break;
			}

			line++;
		}

		// Handle special characters
		if ( curChar == ' ' || curChar == '\t' || curChar == '\n' || curChar == '\r' ) {
			// Update the current bounds (min coordinates)
			minX = std::min( minX, x );
			minY = std::min( minY, y );

			switch ( curChar ) {
				case ' ':
					x += hspace;
					break;
				case '\t':
					x += hspace * mTabWidth;
					break;
				case '\n':
					y += vspace;

					if ( mCachedWidthNeedUpdate )
						mLinesWidth.push_back( x );

					x = 0;
					break;
				case '\r':
					break;
			}

			// Update the current bounds (max coordinates)
			maxX = std::max( maxX, x );
			maxY = std::max( maxY, y );

			if ( mCachedWidthNeedUpdate )
				maxW = std::max( maxW, x );

			// Next glyph, no need to create a quad for whitespace
			continue;
		}

		// Apply the outline
		if ( mFontStyleConfig.OutlineThickness != 0 ) {
			const Glyph& glyph =
				mFontStyleConfig.Font->getGlyph( curChar, mFontStyleConfig.CharacterSize, bold,
												 italic, mFontStyleConfig.OutlineThickness );

			Float left = glyph.bounds.Left;
			Float top = glyph.bounds.Top;
			Float right = glyph.bounds.Left + glyph.bounds.Right;
			Float bottom = glyph.bounds.Top + glyph.bounds.Bottom;

			// Add the outline glyph to the vertices
			addGlyphQuad( mOutlineVertices, Vector2f( x, y ), glyph, italic,
						  mFontStyleConfig.OutlineThickness, centerDiffX );

			// Update the current bounds with the outlined glyph bounds
			minX = std::min( minX, x + left - italic * bottom - mFontStyleConfig.OutlineThickness );
			maxX = std::max( maxX, x + right - italic * top - mFontStyleConfig.OutlineThickness );
			minY = std::min( minY, y + top - mFontStyleConfig.OutlineThickness );
			maxY = std::max( maxY, y + bottom - mFontStyleConfig.OutlineThickness );
			if ( mCachedWidthNeedUpdate )
				maxW = std::max( maxW, x + glyph.advance - italic * top -
										   mFontStyleConfig.OutlineThickness );
		}

		// Extract the current glyph's description
		const Glyph& glyph = mFontStyleConfig.Font->getGlyph(
			curChar, mFontStyleConfig.CharacterSize, bold, reqItalic );

		// Add the glyph to the vertices
		addGlyphQuad( mVertices, Vector2f( x, y ), glyph, italic, 0, centerDiffX );

		// Update the current bounds with the non outlined glyph bounds
		if ( mFontStyleConfig.OutlineThickness == 0 ) {
			Float left = glyph.bounds.Left;
			Float top = glyph.bounds.Top;
			Float right = glyph.bounds.Left + glyph.bounds.Right;
			Float bottom = glyph.bounds.Top + glyph.bounds.Bottom;

			minX = std::min( minX, x + left - italic * bottom );
			maxX = std::max( maxX, x + right - italic * top );
			minY = std::min( minY, y + top );
			maxY = std::max( maxY, y + bottom );

			if ( mCachedWidthNeedUpdate )
				maxW = std::max( maxW, x + glyph.advance - italic * top );
		}

		// Advance to the next character
		x += glyph.advance;
	}

	// If we're using the underlined style, add the last line
	if ( underlined && ( x > 0 ) ) {
		addLine( mVertices, x, y, underlineOffset, underlineThickness, 0, centerDiffX );

		if ( mFontStyleConfig.OutlineThickness != 0 )
			addLine( mOutlineVertices, x, y, underlineOffset, underlineThickness,
					 mFontStyleConfig.OutlineThickness, centerDiffX );
	}

	// If we're using the strike through style, add the last line across all characters
	if ( strikeThrough && ( x > 0 ) ) {
		addLine( mVertices, x, y, strikeThroughOffset, underlineThickness, 0, centerDiffX );

		if ( mFontStyleConfig.OutlineThickness != 0 )
			addLine( mOutlineVertices, x, y, strikeThroughOffset, underlineThickness,
					 mFontStyleConfig.OutlineThickness, centerDiffX );
	}

	if ( mCachedWidthNeedUpdate && !mString.empty() && mString[mString.size() - 1] != '\n' )
		mLinesWidth.push_back( x );

	// Update the bounding rectangle
	mBounds.Left = minX;
	mBounds.Top = minY;
	mBounds.Right = maxX;
	mBounds.Bottom = maxY;

	if ( mCachedWidthNeedUpdate ) {
		mCachedWidth = maxW;
		mCachedWidthNeedUpdate = false;
	}
}

void Text::ensureColorUpdate() {
	if ( mColorsNeedUpdate ) {
		Uint32 tv = getTotalVertices();

		if ( mColors.size() < tv )
			mColors.resize( tv, mFontStyleConfig.FontColor );

		mColors.assign( tv, mFontStyleConfig.FontColor );

		if ( 0 != mFontStyleConfig.OutlineThickness ) {
			if ( mOutlineColors.size() < tv )
				mOutlineColors.resize( tv, mFontStyleConfig.OutlineColor );

			mOutlineColors.assign( tv, mFontStyleConfig.OutlineColor );
		}

		mColorsNeedUpdate = false;

		if ( mContainsColorEmoji ) {
			auto positions = Font::emojiCodePointsPositions( mString );
			for ( const auto& position : positions )
				setFillColor( Color( 255, 255, 255, mFontStyleConfig.FontColor.a ), position,
							  position );
		}
	}
}

const Color& Text::getShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

void Text::setShadowColor( const Color& color ) {
	mFontStyleConfig.ShadowColor = color;
}

Uint32 Text::getNumLines() {
	if ( !mCachedWidthNeedUpdate )
		return mLinesWidth.size();
	return mString.countChar( '\n' ) + 1;
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

	if ( NULL != mFontStyleConfig.Font && !mString.empty() ) {
		updateWidthCache();
		mCachedWidthNeedUpdate = false;
	} else {
		mCachedWidth = 0;
		mLinesWidth.clear();
	}
}

void Text::setStyleConfig( const FontStyleConfig& styleConfig ) {
	setFont( styleConfig.Font );
	setFontSize( styleConfig.CharacterSize );
	setFillColor( styleConfig.FontColor );
	setStyle( styleConfig.Style );
	setOutlineThickness( styleConfig.OutlineThickness );
	setOutlineColor( styleConfig.OutlineColor );
	setShadowColor( styleConfig.ShadowColor );
	setShadowOffset( styleConfig.ShadowOffset );
}

bool Text::hasSameFontStyleConfig( const FontStyleConfig& styleConfig ) {
	return mFontStyleConfig == styleConfig;
}

void Text::setFillColor( const Color& color, Uint32 from, Uint32 to ) {
	if ( mString.empty() )
		return;

	ensureColorUpdate();

	bool underlined = ( mFontStyleConfig.Style & Underlined ) != 0;
	bool strikeThrough = ( mFontStyleConfig.Style & StrikeThrough ) != 0;
	std::size_t s = mString.size();

	if ( to >= s ) {
		to = s - 1;
	}

	if ( from <= to && from < s && to <= s ) {
		size_t rto = to + 1;
		Int32 rpos = from;
		Int32 lpos = 0;
		Uint32 i;
		String::StringBaseType curChar;

		// Spaces, new lines and tabs are not rendered, and not counted as a color
		// We need to skip those characters as nonexistent chars
		for ( i = 0; i < from; i++ ) {
			curChar = mString[i];

			if ( ' ' == curChar || '\n' == curChar || '\t' == curChar || '\r' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;

					if ( '\n' == curChar ) {
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

			lpos = rpos;
			rpos++;

			// Same here
			if ( ' ' == curChar || '\n' == curChar || '\t' == curChar || '\r' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;

					if ( '\n' == curChar ) {
						if ( underlined || strikeThrough ) {
							for ( int v = 0; v < GLi->quadVertexs(); v++ )
								mColors[rpos * GLi->quadVertexs() + v] = color;
						}

						if ( underlined )
							rpos++;

						if ( strikeThrough )
							rpos++;
					}
				}
			} else {
				for ( int v = 0; v < GLi->quadVertexs(); v++ )
					mColors[lpos * GLi->quadVertexs() + v] = color;
			}
		}

		if ( rto == s ) {
			if ( underlined ) {
				lpos++;
				Uint32 pos = lpos * GLi->quadVertexs();

				if ( pos < mColors.size() ) {
					for ( int v = 0; v < GLi->quadVertexs(); v++ )
						mColors[lpos * GLi->quadVertexs() + v] = color;
				}
			}

			if ( strikeThrough ) {
				lpos++;
				Uint32 pos = lpos * GLi->quadVertexs();

				if ( pos < mColors.size() ) {
					for ( int v = 0; v < GLi->quadVertexs(); v++ )
						mColors[lpos * GLi->quadVertexs() + v] = color;
				}
			}
		}
	}
}

// Add an underline or strikethrough line to the vertex array
void Text::addLine( std::vector<VertexCoords>& vertices, Float lineLength, Float lineTop,
					Float offset, Float thickness, Float outlineThickness, Int32 centerDiffX ) {
	Float top = std::floor( lineTop + offset - ( thickness / 2 ) + 0.5f );
	Float bottom = top + std::floor( thickness + 0.5f );
	Float u1 = 0;
	Float v1 = 0;
	Float u2 = 1;
	Float v2 = 1;
	VertexCoords vc;

	if ( GLi->quadsSupported() ) {
		vc.texCoords.x = u1;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + -outlineThickness;
		vc.position.y = top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u1;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + -outlineThickness;
		vc.position.y = bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + lineLength + outlineThickness;
		vc.position.y = bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + lineLength + outlineThickness;
		vc.position.y = top - outlineThickness;
		vertices.push_back( vc );
	} else {
		vc.texCoords.x = u1;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + -outlineThickness;
		vc.position.y = bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u1;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + -outlineThickness;
		vc.position.y = top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + lineLength + outlineThickness;
		vc.position.y = top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u1;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + -outlineThickness;
		vc.position.y = bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + lineLength + outlineThickness;
		vc.position.y = bottom + outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + lineLength + outlineThickness;
		vc.position.y = top - outlineThickness;
		vertices.push_back( vc );
	}
}

// Add a glyph quad to the vertex array
void Text::addGlyphQuad( std::vector<VertexCoords>& vertices, Vector2f position,
						 const EE::Graphics::Glyph& glyph, Float italic, Float outlineThickness,
						 Int32 centerDiffX ) {
	Float padding = 1.0;
	Float left = glyph.bounds.Left - padding;
	Float top = glyph.bounds.Top - padding;
	Float right = glyph.bounds.Left + glyph.bounds.Right + padding;
	Float bottom = glyph.bounds.Top + glyph.bounds.Bottom + padding;

	Float u1 = static_cast<Float>( glyph.textureRect.Left - padding );
	Float v1 = static_cast<Float>( glyph.textureRect.Top - padding );
	Float u2 = static_cast<Float>( glyph.textureRect.Left + glyph.textureRect.Right + padding );
	Float v2 = static_cast<Float>( glyph.textureRect.Top + glyph.textureRect.Bottom + padding );
	VertexCoords vc;

	if ( GLi->quadsSupported() ) {
		vc.texCoords.x = u1;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + position.x + left - italic * top - outlineThickness;
		vc.position.y = position.y + top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u1;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + position.x + left - italic * bottom - outlineThickness;
		vc.position.y = position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + position.x + right - italic * bottom - outlineThickness;
		vc.position.y = position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + position.x + right - italic * top - outlineThickness;
		vc.position.y = position.y + top - outlineThickness;
		vertices.push_back( vc );
	} else {
		vc.texCoords.x = u1;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + position.x + left - italic * bottom - outlineThickness;
		vc.position.y = position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u1;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + position.x + left - italic * top - outlineThickness;
		vc.position.y = position.y + top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + position.x + right - italic * top - outlineThickness;
		vc.position.y = position.y + top - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u1;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + position.x + left - italic * bottom - outlineThickness;
		vc.position.y = position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v2;
		vc.position.x = centerDiffX + position.x + right - italic * bottom - outlineThickness;
		vc.position.y = position.y + bottom - outlineThickness;
		vertices.push_back( vc );

		vc.texCoords.x = u2;
		vc.texCoords.y = v1;
		vc.position.x = centerDiffX + position.x + right - italic * top - outlineThickness;
		vc.position.y = position.y + top - outlineThickness;
		vertices.push_back( vc );
	}
}

Uint32 Text::getTotalVertices() {
	bool underlined = ( mFontStyleConfig.Style & Underlined ) != 0;
	bool strikeThrough = ( mFontStyleConfig.Style & StrikeThrough ) != 0;
	size_t sl = mString.size();
	size_t sv = sl * GLi->quadVertexs();

	String::StringBaseType* c = &mString[0];
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

}} // namespace EE::Graphics
