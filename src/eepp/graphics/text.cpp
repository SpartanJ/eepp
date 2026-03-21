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
#include <eepp/graphics/textlayout.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <limits>

namespace EE { namespace Graphics {

#ifdef EE_TEXT_SHAPER_ENABLED
bool Text::TextShaperEnabled = true;
#else
bool Text::TextShaperEnabled = false;
#endif
bool Text::TextShaperOptimizations = true;
Uint32 Text::GlobalInvalidationId = 0;

Float Text::tabAdvance( Float hspace, Uint32 tabWidth, std::optional<Float> tabOffset ) {
	Float advance = hspace * tabWidth;
	if ( tabOffset ) {
		Float offset = fmodf( *tabOffset, advance );
		advance = advance - offset;
		// If there is not enough space until the next stop, skip it
		if ( advance < hspace )
			advance += hspace * tabWidth;
	}
	return advance;
}

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
						  const Float& outlineThickness, Uint32 textDrawHints,
						  TextDirection direction, std::optional<Float> tabOffset ) {
	return getTextWidth<String>( font, fontSize, string, style, tabWidth, outlineThickness,
								 textDrawHints, direction, tabOffset );
}

Float Text::getTextWidth( Font* font, const Uint32& fontSize, const String::View& string,
						  const Uint32& style, const Uint32& tabWidth,
						  const Float& outlineThickness, Uint32 textDrawHints,
						  TextDirection direction, std::optional<Float> tabOffset ) {
	return getTextWidth<String::View>( font, fontSize, string, style, tabWidth, outlineThickness,
									   textDrawHints, direction, tabOffset );
}

Float Text::getTextWidth( const String& string, const FontStyleConfig& config,
						  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
						  std::optional<Float> tabOffset ) {
	return getTextWidth<String>( config.Font, config.CharacterSize, string, config.Style, tabWidth,
								 config.OutlineThickness, textDrawHints, direction, tabOffset );
}

Float Text::getTextWidth( const String::View& string, const FontStyleConfig& config,
						  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
						  std::optional<Float> tabOffset ) {
	return getTextWidth<String::View>( config.Font, config.CharacterSize, string, config.Style,
									   tabWidth, config.OutlineThickness, textDrawHints, direction,
									   tabOffset );
}

Sizef Text::draw( const String& string, const Vector2f& pos, Font* font, Float fontSize,
				  const Color& fontColor, Uint32 style, Float outlineThickness,
				  const Color& outlineColor, const Color& shadowColor, const Vector2f& shadowOffset,
				  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
				  const WhitespaceDisplayConfig& whitespaceDisplayConfig ) {
	return draw<String>( string, pos, font, fontSize, fontColor, style, outlineThickness,
						 outlineColor, shadowColor, shadowOffset, tabWidth, textDrawHints,
						 direction, whitespaceDisplayConfig );
}

Sizef Text::draw( const String& string, const Vector2f& pos, const FontStyleConfig& config,
				  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
				  const WhitespaceDisplayConfig& whitespaceDisplayConfig ) {
	return draw<String>( string, pos, config, tabWidth, textDrawHints, direction,
						 whitespaceDisplayConfig );
}

Sizef Text::draw( const String::View& string, const Vector2f& pos, Font* font, Float fontSize,
				  const Color& fontColor, Uint32 style, Float outlineThickness,
				  const Color& outlineColor, const Color& shadowColor, const Vector2f& shadowOffset,
				  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
				  const WhitespaceDisplayConfig& whitespaceDisplayConfig ) {
	return draw<String::View>( string, pos, font, fontSize, fontColor, style, outlineThickness,
							   outlineColor, shadowColor, shadowOffset, tabWidth, textDrawHints,
							   direction, whitespaceDisplayConfig );
}

Sizef Text::draw( const String::View& string, const Vector2f& pos, const FontStyleConfig& config,
				  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
				  const WhitespaceDisplayConfig& whitespaceDisplayConfig ) {
	return draw<String::View>( string, pos, config, tabWidth, textDrawHints, direction,
							   whitespaceDisplayConfig );
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

static inline void drawGlyph( BatchRenderer* BR, GlyphDrawable* gd, const Vector2f& position,
							  const Color& color, bool isItalic ) {
	BR->quadsSetColor( color );
	BR->quadsSetTexCoord( gd->getSrcRect().Left, gd->getSrcRect().Top,
						  gd->getSrcRect().Left + gd->getSrcRect().Right,
						  gd->getSrcRect().Top + gd->getSrcRect().Bottom );
	if ( isItalic && !gd->isItalic() ) {
		Float x = position.x + gd->getGlyphOffset().x;
		Float y = position.y + gd->getGlyphOffset().y;
		Float italic = 0.208f * gd->getDestSize().getWidth(); // 12 degrees
		BR->batchQuadFree( x + italic, y, x, y + gd->getDestSize().getHeight(),
						   x + gd->getDestSize().getWidth(), y + gd->getDestSize().getHeight(),
						   x + gd->getDestSize().getWidth() + italic, y );
	} else {
		BR->batchQuad( position.x + gd->getGlyphOffset().x, position.y + gd->getGlyphOffset().y,
					   gd->getDestSize().getWidth(), gd->getDestSize().getHeight() );
	}
}

static inline void _drawUnderline( Font* font, Float fontSize, const Color& fontColor,
								   const Vector2f& cpos, const Uint32& style, BatchRenderer* BR,
								   Float outlineThickness, const Vector2f& pos, Float width,
								   const Color& shadowColor, const Vector2f& shadowOffset,
								   const Color& outlineColor ) {
	Float underlineOffset = font->getUnderlinePosition( fontSize );
	Float underlineThickness = font->getUnderlineThickness( fontSize );
	Float top =
		cpos.y + std::floor( fontSize + underlineOffset - ( underlineThickness / 2 ) + 0.5f );
	Float bottom = top + std::floor( underlineThickness + 0.5f );
	if ( top == bottom )
		bottom = top + 1;

	if ( style & Text::Shadow ) {
		BR->quadsSetTexCoord( 0, 0, 1, 1 );
		BR->quadsSetColor( shadowColor );
		BR->batchQuad( Rectf( pos.x + shadowOffset.x, top + shadowOffset.y,
							  pos.x + width + shadowOffset.x, bottom + shadowOffset.y ) );
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
}

void Text::drawUnderline( const Vector2f& pos, Float width, Font* font, Float fontSize,
						  const Color& fontColor, const Uint32& style, Float outlineThickness,
						  const Color& outlineColor, const Color& shadowColor,
						  const Vector2f& shadowOffset ) {
	BatchRenderer* BR = GlobalBatchRenderer::instance();
	_drawUnderline( font, fontSize, fontColor, pos, style, BR, outlineThickness, pos, width,
					shadowColor, shadowOffset, outlineColor );
}

static inline void _drawStrikeThrough( Font* font, Float fontSize, const Color& fontColor,
									   const Vector2f& cpos, const Uint32& style, BatchRenderer* BR,
									   Float outlineThickness, const Vector2f& pos, Float width,
									   const Color& shadowColor, const Vector2f& shadowOffset,
									   const Color& outlineColor ) {
	Rectf xBounds =
		font->getGlyph( L'x', fontSize, style & Text::Bold, style & Text::Italic ).bounds;
	Float strikeThroughOffset = xBounds.Top + xBounds.Bottom * 0.5f;
	Float underlineThickness = font->getUnderlineThickness( fontSize );
	Float top =
		std::floor( cpos.y + fontSize + strikeThroughOffset - ( underlineThickness / 2 ) + 0.5f );
	Float bottom = top + std::floor( underlineThickness + 0.5f );
	if ( top == bottom )
		bottom = top + 1;

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
}

void Text::drawStrikeThrough( const Vector2f& pos, Float width, Font* font, Float fontSize,
							  const Color& fontColor, const Uint32& style, Float outlineThickness,
							  const Color& outlineColor, const Color& shadowColor,
							  const Vector2f& shadowOffset ) {
	BatchRenderer* BR = GlobalBatchRenderer::instance();
	_drawStrikeThrough( font, fontSize, fontColor, pos, style, BR, outlineThickness, pos, width,
						shadowColor, shadowOffset, outlineColor );
}

template <typename StringType>
Sizef Text::draw( const StringType& string, const Vector2f& pos, Font* font, Float fontSize,
				  const Color& fontColor, Uint32 style, Float outlineThickness,
				  const Color& outlineColor, const Color& shadowColor, const Vector2f& shadowOffset,
				  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
				  const WhitespaceDisplayConfig& whitespaceDisplayConfig ) {
	Vector2f cpos{ pos };
	String::StringBaseType ch;
	String::StringBaseType prevChar = 0;
	bool isBold = ( style & Text::Bold ) != 0;
	bool isItalic = ( style & Text::Italic ) != 0;
	bool fallbacksToColorEmoji =
		font && font->getType() == FontType::TTF &&
		!static_cast<FontTrueType*>( font )->isColorEmojiFont() &&
		FontManager::instance()->getColorEmojiFont() != nullptr &&
		FontManager::instance()->getColorEmojiFont()->getType() == FontType::TTF;
	bool isMonospace = font && ( font->isMonospace() ||
								 ( font->getType() == FontType::TTF &&
								   static_cast<FontTrueType*>( font )->isIdentifiedAsMonospace() &&
								   canSkipShaping( textDrawHints ) ) );
	Float kerning = 0;
	Float width = 0;
	Float height = font->getLineSpacing( fontSize );
	Sizef size{ 0, height };
	size_t ssize = string.size();
	BatchRenderer* BR = GlobalBatchRenderer::instance();
	Texture* fontTexture = font->getTexture( fontSize );
	Float tabAlign = 0;
	GlyphDrawable* spaceGlyph = nullptr;
	GlyphDrawable* tabGlyph = nullptr;
	Float hspace = font->getGlyph( ' ', fontSize, isBold, isItalic ).advance;
	std::optional<Float> tabOffset{ whitespaceDisplayConfig.tabOffset };
	if ( whitespaceDisplayConfig.tabDisplayCharacter )
		tabGlyph = font->getGlyphDrawable( whitespaceDisplayConfig.tabDisplayCharacter, fontSize );

	BR->setBlendMode( BlendMode::Alpha() );
	BR->quadsBegin();
	BR->setTexture( fontTexture, fontTexture->getCoordinateType() );

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && font->getType() == FontType::TTF &&
		 !canSkipShaping( textDrawHints ) ) {
		FontTrueType* rFont = static_cast<FontTrueType*>( font );

		auto layout = TextLayout::layout( string, rFont, fontSize, style, tabWidth,
										  outlineThickness, tabOffset, textDrawHints, direction );

		for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
			for ( const ShapedGlyph& sg : sp.shapedGlyphs ) {
				auto ch = string[sg.stringIndex];
				auto gpos( ( sg.position + pos ).trunc() );

				if ( ch == '\t' ) {
					if ( whitespaceDisplayConfig.tabDisplayCharacter ) {
						Float advance = tabAdvance( hspace, tabWidth,
													tabOffset ? gpos.x - pos.x + *tabOffset
															  : std::optional<Float>{} );

						switch ( whitespaceDisplayConfig.tabAlign ) {
							case CharacterAlignment::Center:
								tabAlign =
									( advance - tabGlyph->getPixelsSize().getWidth() ) * 0.5f;
								break;
							case CharacterAlignment::Right:
								tabAlign = advance - tabGlyph->getPixelsSize().getWidth();
								break;
							case CharacterAlignment::Left:
								break;
						}

						if ( tabGlyph ) {
							drawGlyph( BR, tabGlyph, { gpos.x + tabAlign, gpos.y },
									   whitespaceDisplayConfig.color, isItalic );
						}
					}
					continue;
				}

				if ( ch == ' ' ) {
					if ( whitespaceDisplayConfig.spaceDisplayCharacter ) {
						if ( spaceGlyph == nullptr ) {
							spaceGlyph = font->getGlyphDrawable(
								whitespaceDisplayConfig.spaceDisplayCharacter, fontSize );
						}
						drawGlyph( BR, spaceGlyph, gpos, whitespaceDisplayConfig.color, isItalic );
					}
					continue;
				}

				if ( style & Text::Shadow ) {
					auto* gds = sg.font->getGlyphDrawableFromGlyphIndex(
						sg.glyphIndex, fontSize, isBold, isItalic, outlineThickness,
						rFont->getPage( fontSize ) );
					if ( gds )
						drawGlyph( BR, gds, gpos, shadowColor, isItalic );
				}

				if ( outlineThickness != 0.f ) {
					auto* gdo = sg.font->getGlyphDrawableFromGlyphIndex(
						sg.glyphIndex, fontSize, isBold, isItalic, outlineThickness,
						rFont->getPage( fontSize ) );
					if ( gdo )
						drawGlyph( BR, gdo, gpos, outlineColor, isItalic );
				}

				auto* gd = sg.font->getGlyphDrawableFromGlyphIndex(
					sg.glyphIndex, fontSize, isBold, isItalic, 0, rFont->getPage( fontSize ) );
				if ( gd ) {
					drawGlyph( BR, gd, gpos,
							   fallbacksToColorEmoji && Font::isEmojiCodePoint( ch ) ? Color::White
																					 : fontColor,
							   isItalic );
				}
			}
		}

		BR->drawOpt();

		return layout->size;
	}
#endif

	for ( size_t i = 0; i < ssize; ++i ) {
		ch = string[i];

		switch ( ch ) {
			case '\r':
				continue;
			case '\t': {
				Float advance =
					tabAdvance( hspace, tabWidth,
								tabOffset ? cpos.x - pos.x + *tabOffset : std::optional<Float>{} );

				if ( whitespaceDisplayConfig.tabDisplayCharacter ) {
					switch ( whitespaceDisplayConfig.tabAlign ) {
						case CharacterAlignment::Center:
							tabAlign = ( advance - tabGlyph->getPixelsSize().getWidth() ) * 0.5f;
							break;
						case CharacterAlignment::Right:
							tabAlign = advance - tabGlyph->getPixelsSize().getWidth();
							break;
						case CharacterAlignment::Left:
							break;
					}
				}

				if ( tabGlyph ) {
					drawGlyph( BR, tabGlyph, { cpos.x + tabAlign, cpos.y },
							   whitespaceDisplayConfig.color, isItalic );
				}
				width += advance;
				cpos.x += advance;
				prevChar = ch;
				continue;
			}
			case ' ': {
				if ( whitespaceDisplayConfig.spaceDisplayCharacter ) {
					if ( spaceGlyph == nullptr ) {
						spaceGlyph = font->getGlyphDrawable(
							whitespaceDisplayConfig.spaceDisplayCharacter, fontSize );
					}
					drawGlyph( BR, spaceGlyph, cpos, whitespaceDisplayConfig.color, isItalic );
				}
				width += hspace;
				cpos.x += hspace;
				prevChar = ch;
				continue;
			}
			case '\n': {
				if ( style & Text::Underlined ) {
					_drawUnderline( font, fontSize, fontColor, cpos, style, BR, outlineThickness,
									pos, width, shadowColor, shadowOffset, outlineColor );
				}
				if ( style & Text::StrikeThrough ) {
					_drawStrikeThrough( font, fontSize, fontColor, cpos, style, BR,
										outlineThickness, pos, width, shadowColor, shadowOffset,
										outlineColor );
				}
				size.x = eemax( width, size.x );
				width = 0;
				cpos.x = pos.x;
				cpos.y += height;
				if ( i != ssize - 1 )
					size.y += height;
				continue;
			}
		}

		if ( style & Text::Shadow ) {
			auto* gds = font->getGlyphDrawable( ch, fontSize, isBold, isItalic, outlineThickness );
			if ( gds )
				drawGlyph( BR, gds, cpos, shadowColor, isItalic );
		}

		if ( outlineThickness != 0.f ) {
			auto* gdo = font->getGlyphDrawable( ch, fontSize, isBold, isItalic, outlineThickness );
			if ( gdo )
				drawGlyph( BR, gdo, cpos, outlineColor, isItalic );
		}

		auto* gd = font->getGlyphDrawable( ch, fontSize, isBold, isItalic );
		if ( gd ) {
			if ( !isMonospace && !( textDrawHints & TextHints::NoKerning ) ) {
				kerning =
					font->getKerning( prevChar, ch, fontSize, isBold, isItalic, outlineThickness );
				cpos.x += kerning;
				width += kerning;
			}

			drawGlyph( BR, gd, cpos,
					   fallbacksToColorEmoji && Font::isEmojiCodePoint( ch ) ? Color::White
																			 : fontColor,
					   isItalic );

			cpos.x += gd->getAdvance();
			width += gd->getAdvance();
		}

		prevChar = ch;
	}

	if ( ( style & Text::Underlined ) && width != 0 ) {
		_drawUnderline( font, fontSize, fontColor, cpos, style, BR, outlineThickness, pos, width,
						shadowColor, shadowOffset, outlineColor );
	}

	if ( ( style & Text::StrikeThrough ) && width != 0 ) {
		_drawStrikeThrough( font, fontSize, fontColor, cpos, style, BR, outlineThickness, pos,
							width, shadowColor, shadowOffset, outlineColor );
	}

	size.x = eemax( width, size.x );

	BR->drawOpt();

	return size;
}

template <typename StringType>
Sizef Text::draw( const StringType& string, const Vector2f& pos, const FontStyleConfig& config,
				  const Uint32& tabWidth, Uint32 textDrawHints, TextDirection direction,
				  const WhitespaceDisplayConfig& whitespaceDisplayConfig ) {
	return draw<StringType>( string, pos, config.Font, config.CharacterSize, config.FontColor,
							 config.Style, config.OutlineThickness, config.OutlineColor,
							 config.ShadowColor, config.ShadowOffset, tabWidth, textDrawHints,
							 direction, whitespaceDisplayConfig );
}

bool Text::hardWrapText( Font* font, const Uint32& fontSize, String& string, const Float& maxWidth,
						 const Uint32& style, const Uint32& tabWidth, const Float& outlineThickness,
						 std::optional<Float> tabOffset, Uint32 textHints ) {

	if ( string.empty() || NULL == font )
		return false;

	LineWrapInfo lineWrap = LineWrap::computeLineBreaks(
		string, font, fontSize, maxWidth, LineWrapMode::Word, style, outlineThickness,
		tabOffset.has_value(), tabWidth, tabOffset ? *tabOffset : 0.f, textHints );

	if ( lineWrap.wraps.size() <= 1 )
		return false;

	for ( Int64 i = lineWrap.wraps.size() - 1; i > 0; i-- ) {
		if ( string[lineWrap.wraps[i]] != '\n' ) // Skip real line-breaks!
			string.insert( lineWrap.wraps[i], '\n' );
	}

	return true;
}

bool Text::hardWrapText( String& string, const Float& maxWidth, const FontStyleConfig& config,
						 const Uint32& tabWidth, std::optional<Float> tabOffset,
						 Uint32 textHints ) {
	return hardWrapText( config.Font, config.CharacterSize, string, maxWidth, config.Style,
						 tabWidth, config.OutlineThickness, tabOffset, textHints );
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
	if ( !mFontStyleConfig.Font->isScalable() ) {
		mFontStyleConfig.CharacterSize =
			mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
	}
}

void Text::create( Font* font, const String& text, Color FontColor, Color FontShadowColor,
				   Uint32 characterSize ) {
	mFontStyleConfig.Font = font;
	mFontStyleConfig.CharacterSize = characterSize;
	if ( !mFontStyleConfig.Font->isScalable() ) {
		mFontStyleConfig.CharacterSize =
			mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
	}
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
	mVisualLinesNeedUpdate = true;
	mTextHints = mString.getTextHints();
	if ( mFontStyleConfig.Font && FontManager::instance()->getColorEmojiFont() != nullptr ) {
		if ( mFontStyleConfig.Font->getType() == FontType::TTF ) {
			FontTrueType* fontTrueType = static_cast<FontTrueType*>( mFontStyleConfig.Font );
			if ( fontTrueType->isColorEmojiFont() || !fontTrueType->isEmojiFont() )
				mContainsColorEmoji = Font::containsEmojiCodePoint( mString );
		}
	}
}

bool Text::setString( const String::View& string ) {
	if ( mString.view() != string ) {
		mString = string;
		onNewString();
		return true;
	}
	return false;
}

bool Text::setString( const String& string ) {
	if ( mString != string ) {
		mString = string;
		onNewString();
		return true;
	}
	return false;
}

bool Text::setString( String&& string ) {
	if ( mString != string ) {
		mString = std::move( string );
		onNewString();
		return true;
	}
	return false;
}

void Text::setFont( Font* font ) {
	if ( NULL != font && mFontStyleConfig.Font != font ) {
		mFontStyleConfig.Font = font;
		if ( !mFontStyleConfig.Font->isScalable() ) {
			mFontStyleConfig.CharacterSize =
				mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
		}
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
		mVisualLinesNeedUpdate = true;
	}
}

void Text::setFontSize( unsigned int size ) {
	if ( NULL != mFontStyleConfig.Font && mFontStyleConfig.CharacterSize != size ) {
		mFontStyleConfig.CharacterSize = size;
		if ( !mFontStyleConfig.Font->isScalable() ) {
			mFontStyleConfig.CharacterSize =
				mFontStyleConfig.Font->getFontHeight( mFontStyleConfig.CharacterSize );
		}
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
		mVisualLinesNeedUpdate = true;
	}
}

void Text::setStyle( Uint32 style ) {
	if ( mFontStyleConfig.Style != style ) {
		mFontStyleConfig.Style = style;
		mColorsNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
		mVisualLinesNeedUpdate = true;
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
		mVisualLinesNeedUpdate = true;
	}
}

void Text::setTabStops( bool enabled ) {
	if ( mTabStops != enabled ) {
		mTabStops = enabled;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
		mVisualLinesNeedUpdate = true;
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

const String& Text::getString() const {
	return mString;
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

	if ( mLineWrapMode == LineWrapMode::NoWrap || mMaxWrapWidth <= 0 ) {
		return Text::findCharacterPos( index, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize,
									   mString, mFontStyleConfig.Style, mTabWidth,
									   mFontStyleConfig.OutlineThickness, {}, true, mTextHints,
									   TextDirection::Unspecified, mInitialOffset );
	}

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && mFontStyleConfig.Font->getType() == FontType::TTF &&
		 !canSkipShaping( mTextHints ) ) {
		return Text::findCharacterPos(
			index, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, mString,
			mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness, {}, true,
			mTextHints, TextDirection::Unspecified, mLineWrapMode, mMaxWrapWidth, mInitialOffset );
	}
#endif

	const_cast<Text*>( this )->ensureVisualLinesUpdate();

	std::size_t visualLinesSize = mVisualLines.size();
	std::size_t lineIndex = const_cast<Text*>( this )->findVisualLineFromCharIndex( index );
	Float vspace = static_cast<Float>(
		mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize ) );
	Float y = lineIndex * vspace;
	Float centerDiffX = 0;

	if ( lineIndex < mLinesWidth.size() ) {
		switch ( Font::getHorizontalAlign( mAlign ) ) {
			case TEXT_ALIGN_CENTER:
				centerDiffX = std::trunc( ( mCachedWidth - mLinesWidth[lineIndex] ) * 0.5f );
				break;
			case TEXT_ALIGN_RIGHT:
				centerDiffX = mCachedWidth - mLinesWidth[lineIndex];
				break;
		}
	}

	if ( mLineWrapKeepIndentation && lineIndex > 0 ) {
		Int64 prevLineStart = mVisualLines[lineIndex];
		if ( prevLineStart > 0 && mString[prevLineStart - 1] != '\n' ) {
			centerDiffX += LineWrap::computeOffsets( mString.view(), mFontStyleConfig, mTabWidth,
													 mMaxWrapWidth, mTabStops );
		}
	}

	Int64 startIdx = mVisualLines[lineIndex];
	if ( mString[startIdx] == '\n' )
		startIdx++;
	Int64 endIdx =
		( lineIndex + 1 < visualLinesSize ) ? mVisualLines[lineIndex + 1] : (Int64)mString.size();
	if ( endIdx > (Int64)mString.size() )
		endIdx = mString.size();

	if ( index < (size_t)startIdx )
		index = startIdx;

	String strWrapper(
		String::View( mString.view().data() + startIdx, (std::size_t)( endIdx - startIdx ) ) );

	Vector2f pos = Text::findCharacterPos(
		index - startIdx, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, strWrapper,
		mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness, {}, true, mTextHints,
		TextDirection::Unspecified, lineIndex == 0 ? mInitialOffset : Vector2f::Zero );

	return Vector2f( pos.x + centerDiffX, y );
}

Int32 Text::findCharacterFromPos( const Vector2i& pos, bool returnNearest ) const {
	if ( NULL == mFontStyleConfig.Font || mString.empty() )
		return 0;

	if ( mLineWrapMode == LineWrapMode::NoWrap || mMaxWrapWidth <= 0 ) {
		return Text::findCharacterFromPos(
			pos, returnNearest, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, mString,
			mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness, {}, mTextHints,
			TextDirection::Unspecified, mInitialOffset );
	}

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && mFontStyleConfig.Font->getType() == FontType::TTF &&
		 !canSkipShaping( mTextHints ) ) {
		return Text::findCharacterFromPos(
			pos, returnNearest, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, mString,
			mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness, {}, mTextHints,
			TextDirection::Unspecified, mLineWrapMode, mMaxWrapWidth, mInitialOffset );
	}
#endif

	const_cast<Text*>( this )->ensureVisualLinesUpdate();

	Float vspace = mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize );
	int lineIndex = std::floor( pos.y / vspace );
	std::size_t visualLinesSize = mVisualLines.size();

	if ( lineIndex < 0 )
		lineIndex = 0;
	if ( lineIndex >= (int)visualLinesSize ) {
		if ( returnNearest )
			lineIndex = visualLinesSize - 1;
		else
			return -1;
	}

	Float centerDiffX = 0;

	if ( lineIndex < (int)mLinesWidth.size() ) {
		switch ( Font::getHorizontalAlign( mAlign ) ) {
			case TEXT_ALIGN_CENTER:
				centerDiffX = std::trunc( ( mCachedWidth - mLinesWidth[lineIndex] ) * 0.5f );
				break;
			case TEXT_ALIGN_RIGHT:
				centerDiffX = mCachedWidth - mLinesWidth[lineIndex];
				break;
		}
	}

	if ( mLineWrapKeepIndentation && lineIndex > 0 ) {
		Int64 prevLineStart = mVisualLines[lineIndex];
		if ( prevLineStart > 0 && mString[prevLineStart - 1] != '\n' ) {
			centerDiffX += LineWrap::computeOffsets( mString.view(), mFontStyleConfig, mTabWidth,
													 mMaxWrapWidth, mTabStops );
		}
	}

	Int64 startIdx = mVisualLines[lineIndex];
	if ( mString[startIdx] == '\n' )
		startIdx++;

	Int64 endIdx = ( lineIndex + 1 < (int)visualLinesSize ) ? mVisualLines[lineIndex + 1]
															: (Int64)mString.size();
	if ( endIdx > (Int64)mString.size() )
		endIdx = mString.size();

	String strWrapper(
		String::View( mString.view().data() + startIdx, (std::size_t)( endIdx - startIdx ) ) );
	Vector2i localPos( pos.x - centerDiffX, 0 ); // Y is treated as 0 in single line

	Int32 foundIndex = Text::findCharacterFromPos(
		localPos, returnNearest, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, strWrapper,
		mFontStyleConfig.Style, mTabWidth, mFontStyleConfig.OutlineThickness, {}, mTextHints );

	if ( foundIndex != -1 )
		return startIdx + foundIndex;

	return -1;
}

static bool isStopSelChar( Uint32 c ) {
	static std::string_view DEFAULT_NON_WORD_CHARS( " \t\n/\\()\"':,.;<>~!@#$%^&*|+=[]{}`?-" );
	return DEFAULT_NON_WORD_CHARS.find_first_of( c ) != std::string_view::npos;
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
						  const Float& outlineThickness, Uint32 textDrawHints,
						  TextDirection direction, std::optional<Float> tabOffset ) {
	if ( NULL == font || string.empty() )
		return 0;
	Float width = 0;
	Float maxWidth = 0;
	String::StringBaseType codepoint;
	Uint32 prevChar = 0;
	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Text::Italic ) != 0;
	bool isMonospace = font && ( font->isMonospace() ||
								 ( font->getType() == FontType::TTF &&
								   static_cast<FontTrueType*>( font )->isIdentifiedAsMonospace() &&
								   canSkipShaping( textDrawHints ) ) );
	Float hspace = static_cast<Float>(
		font->getGlyph( L' ', fontSize, bold, italic, outlineThickness ).advance );

	if ( isMonospace ) {
		size_t len = string.length();
		Float width = 0;
		Float maxWidth = 0;
		for ( size_t i = 0; i < len; i++ ) {
			if ( string[i] == '\n' ) {
				width = 0;
			} else {
				width +=
					( string[i] == '\t' )
						? tabAdvance( hspace, tabWidth, tabOffset ? *tabOffset + width : tabOffset )
						: hspace;
			}
			maxWidth = eemax( width, maxWidth );
		}
		return maxWidth;
	}

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && font->getType() == FontType::TTF &&
		 !canSkipShaping( textDrawHints ) ) {
		return TextLayout::layout( string, static_cast<FontTrueType*>( font ), fontSize, style,
								   tabWidth, outlineThickness, tabOffset, textDrawHints, direction )
			->size.getWidth();
	}
#endif

	for ( std::size_t i = 0; i < string.size(); ++i ) {
		codepoint = string.at( i );
		Glyph glyph = font->getGlyph( codepoint, fontSize, bold, italic, outlineThickness );
		if ( codepoint == '\t' ) {
			width += tabAdvance( hspace, tabWidth, tabOffset ? *tabOffset + width : tabOffset );
		} else if ( codepoint == '\n' ) {
			width = 0;
			prevChar = 0;
			continue;
		} else if ( codepoint != '\r' ) {
			if ( !( textDrawHints & TextHints::NoKerning ) ) {
				width += font->getKerning( prevChar, codepoint, fontSize, bold, italic,
										   outlineThickness );
			}
			width += glyph.advance;
		}
		maxWidth = eemax( width, maxWidth );
		prevChar = codepoint;
	}
	return maxWidth;
}

template <typename StringType>
std::size_t
Text::findLastCharPosWithinLength( Font* font, const Uint32& fontSize, const StringType& string,
								   Float maxWidth, const Uint32& style, const Uint32& tabWidth,
								   const Float& outlineThickness, std::optional<Float> tabOffset,
								   Uint32 textDrawHints, TextDirection direction ) {
	if ( NULL == font || string.empty() )
		return 0;
	String::StringBaseType codepoint;
	Uint32 prevChar = 0;
	Float width = 0;
	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Text::Italic ) != 0;
	Float hspace = static_cast<Float>(
		font->getGlyph( L' ', fontSize, bold, italic, outlineThickness ).advance );

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && font->getType() == FontType::TTF &&
		 !canSkipShaping( textDrawHints ) ) {
		auto layout =
			TextLayout::layout( string, static_cast<FontTrueType*>( font ), fontSize, style,
								tabWidth, outlineThickness, tabOffset, 0, direction );
		size_t lastStringIndex = 0;
		for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
			for ( const ShapedGlyph& sg : sp.shapedGlyphs ) {
				Glyph metrics = sg.font->getGlyphByIndex( sg.glyphIndex, fontSize, bold, italic,
														  outlineThickness );
				if ( sg.position.x + metrics.advance > maxWidth )
					return lastStringIndex;
				lastStringIndex = sg.stringIndex;
			}
		}
		return string.size();
	}
#endif

	for ( std::size_t i = 0; i < string.size(); ++i ) {
		codepoint = string.at( i );
		Glyph glyph = font->getGlyph( codepoint, fontSize, bold, italic, outlineThickness );
		if ( codepoint != '\r' && codepoint != '\t' ) {
			if ( !( textDrawHints & TextHints::NoKerning ) ) {
				width += font->getKerning( prevChar, codepoint, fontSize, bold, italic,
										   outlineThickness );
			}
			width += glyph.advance;
		} else if ( codepoint == '\t' )
			width += tabAdvance( hspace, tabWidth, tabOffset ? *tabOffset + width : tabOffset );

		if ( width > maxWidth )
			return i > 0 ? i - 1 : 0;
		if ( codepoint == '\n' )
			width = 0;
		prevChar = codepoint;
	}
	return width <= maxWidth ? string.size() : string.size() - 1;
}

Vector2f Text::findCharacterPos( std::size_t index, Font* font, const Uint32& fontSize,
								 const String& string, const Uint32& style, const Uint32& tabWidth,
								 const Float& outlineThickness, std::optional<Float> tabOffset,
								 bool allowNewLine, Uint32 textDrawHints, TextDirection direction,
								 const Vector2f& initialOffset ) {
	return findCharacterPos( index, font, fontSize, string, style, tabWidth, outlineThickness,
							 tabOffset, allowNewLine, textDrawHints, direction,
							 LineWrapMode::NoWrap, 0.f, initialOffset );
}

Vector2f Text::findCharacterPos( std::size_t index, Font* font, const Uint32& fontSize,
								 const String& string, const Uint32& style, const Uint32& tabWidth,
								 const Float& outlineThickness, std::optional<Float> tabOffset,
								 bool allowNewLine, Uint32 textDrawHints, TextDirection direction,
								 LineWrapMode lineWrapMode, Float maxWrapWidth,
								 const Vector2f& initialOffset ) {
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

	// Compute the position, starting from initialOffset
	Vector2f position = initialOffset;

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && font->getType() == FontType::TTF &&
		 !canSkipShaping( textDrawHints ) ) {
		auto layout = TextLayout::layout( string, font, fontSize, style, tabWidth, outlineThickness,
										  tabOffset, textDrawHints, direction, lineWrapMode,
										  maxWrapWidth, false, initialOffset.x );
		bool hasGlyphs =
			allowNewLine
				? !layout->paragraphs.empty() && !layout->paragraphs.back().shapedGlyphs.empty()
				: !layout->paragraphs.empty() && !layout->paragraphs.front().shapedGlyphs.empty();

		// We only need to add initialOffset.y for the vertical shift

		if ( index == string.size() && hasGlyphs ) {
			const auto& lastGlyph = allowNewLine ? layout->paragraphs.back().shapedGlyphs.back()
												 : layout->paragraphs.front().shapedGlyphs.back();
			return ( lastGlyph.position + lastGlyph.advance + Vector2f{ 0, initialOffset.y } )
				.trunc();
		}

		Uint32 maxStringIndex = 0;
		Uint32 closestDist = std::numeric_limits<Uint32>::max();

		const ShapedGlyph* msg = nullptr;
		const ShapedGlyph* csg = nullptr;
		for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
			for ( const ShapedGlyph& sg : sp.shapedGlyphs ) {
				if ( sg.stringIndex >= maxStringIndex ) {
					maxStringIndex = std::max( maxStringIndex, sg.stringIndex );
					msg = &sg;
				}

				auto dist = std::abs( (Int64)index - (Int64)sg.stringIndex );
				if ( dist < closestDist ) {
					closestDist = dist;
					csg = &sg;
				}

				if ( sg.stringIndex == index ) {
					if ( layout->isRTL() )
						return ( sg.position + sg.advance + Vector2f{ 0, initialOffset.y } )
							.trunc();
					return ( sg.position + Vector2f{ 0, initialOffset.y } ).trunc();
				}
			}
		}

		if ( hasGlyphs && !layout->isRTL() && index >= maxStringIndex + 1 && msg ) {
			Glyph metrics = msg->font->getGlyphByIndex( msg->glyphIndex, fontSize, bold, italic,
														outlineThickness );
			if ( string[msg->stringIndex] == '\t' ) {
				Float advance = Text::tabAdvance( hspace, tabWidth,
												  tabOffset ? msg->position.x + *tabOffset
															: std::optional<Float>{} );
				return ( msg->position + Vector2f{ advance, initialOffset.y } ).trunc();
			}
			return ( msg->position + Vector2f{ metrics.advance, initialOffset.y } ).trunc();
		}

		if ( csg && closestDist != std::numeric_limits<Uint32>::max() ) {
			return ( csg->position + Vector2f{ 0, initialOffset.y } ).trunc();
		}

		return position;
	}
#endif

	// If soft-wrap is enabled and we are not using the shaper (or it's skipped), we need to compute
	// line breaks to correctly calculate the position.
	if ( lineWrapMode != LineWrapMode::NoWrap ) {
		if ( index == 0 )
			return position;

		LineWrapInfo info = LineWrap::computeLineBreaks(
			string, font, fontSize, maxWrapWidth, lineWrapMode, style, outlineThickness, false,
			tabWidth, 0.f, textDrawHints, false, initialOffset.x );

		size_t lineIndex = 0;
		size_t lineStartIdx = 0;

		for ( size_t i = 1; i < info.wraps.size(); ++i ) {
			if ( index < static_cast<size_t>( info.wraps[i] ) ) {
				break;
			}
			lineIndex = i;
			lineStartIdx = info.wraps[i];
		}

		if ( lineIndex > 0 ) {
			position.x = info.paddingStart;
			position.x += info.paddingStart;
			position.y += vspace * lineIndex;
		}

		Float segmentWidth = 0;
		if ( index > lineStartIdx ) {
			std::optional<Float> currentTabOffset =
				lineIndex == 0 ? initialOffset.x : info.paddingStart;
			String::View segment = string.view().substr( lineStartIdx, index - lineStartIdx );
			segmentWidth =
				Text::getTextWidth( font, fontSize, segment, style, tabWidth, outlineThickness,
									textDrawHints, direction, currentTabOffset );
		}

		position.x += segmentWidth;
		return position;
	}

	Uint32 prevChar = 0;
	bool isMonospace = font->isMonospace();
	for ( std::size_t i = 0; i <= index; ++i ) {
		String::StringBaseType curChar = string[i];

		// Apply the kerning offset
		position.x += isMonospace || ( textDrawHints & TextHints::NoKerning )
						  ? 0
						  : static_cast<Float>( font->getKerning( prevChar, curChar, fontSize, bold,
																  italic, outlineThickness ) );
		prevChar = curChar;

		if ( index == i )
			break;

		// Handle special characters
		switch ( curChar ) {
			case ' ':
				position.x += hspace;
				continue;
			case '\t':
				position.x +=
					tabAdvance( hspace, tabWidth, tabOffset ? *tabOffset + position.x : tabOffset );
				continue;
			case '\n':
				if ( allowNewLine ) {
					position.y += vspace;
					position.x = 0;
				}
				continue;
			case '\r':
				continue;
		}

		// For regular characters, add the advance offset of the glyph
		position.x +=
			isMonospace
				? hspace
				: static_cast<Float>(
					  font->getGlyph( curChar, fontSize, bold, italic, outlineThickness ).advance );
	}

	return position;
}

Int32 Text::findCharacterFromPos( const Vector2i& pos, bool returnNearest, Font* font,
								  const Uint32& fontSize, const String& string, const Uint32& style,
								  const Uint32& tabWidth, const Float& outlineThickness,
								  std::optional<Float> tabOffset, Uint32 textDrawHints,
								  TextDirection direction, const Vector2f& initialOffset ) {
	return findCharacterFromPos( pos, returnNearest, font, fontSize, string, style, tabWidth,
								 outlineThickness, tabOffset, textDrawHints, direction,
								 LineWrapMode::NoWrap, 0.f, initialOffset );
}

Int32 Text::findCharacterFromPos( const Vector2i& pos, bool returnNearest, Font* font,
								  const Uint32& fontSize, const String& string, const Uint32& style,
								  const Uint32& tabWidth, const Float& outlineThickness,
								  std::optional<Float> tabOffset, Uint32 textDrawHints,
								  TextDirection direction, LineWrapMode lineWrapMode,
								  Float maxWrapWidth, const Vector2f& initialOffset ) {
	if ( NULL == font )
		return 0;

	Float vspace = font->getLineSpacing( fontSize );
	Float width = 0, lWidth = 0, height = vspace, lHeight = 0;
	Uint32 codepoint;
	Uint32 prevChar = 0;
	Int32 nearest = -1;
	Int32 minDist = std::numeric_limits<Int32>::max();
	Int32 curDist = -1;
	std::size_t tSize = string.size();
	bool bold = ( style & Text::Bold ) != 0;
	bool italic = ( style & Italic ) != 0;

	// Always subtract Y offset (shifts entire text block)
	// Only subtract X offset for first line (Y < vspace after subtracting Y offset)
	Float adjY = pos.y - initialOffset.y;
	Float adjX = pos.x - ( adjY < vspace ? initialOffset.x : 0.f );
	Vector2f fpos( adjX, adjY );

	Float hspace = static_cast<Float>(
		font->getGlyph( L' ', fontSize, bold, italic, outlineThickness ).advance );

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && font->getType() == FontType::TTF &&
		 !canSkipShaping( textDrawHints ) ) {
		auto layout =
			TextLayout::layout( string, font, fontSize, style, tabWidth, outlineThickness,
								tabOffset, textDrawHints, direction, lineWrapMode, maxWrapWidth );

		int sgs = 0;

		if ( fpos.x < 0 )
			return layout->isRTL() ? tSize : 0;

		Float charLeft = 0, charTop = 0, charBottom = 0, charRight = 0;
		for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
			sgs = sp.shapedGlyphs.size();
			if ( sgs == 0 )
				return 0;

			for ( auto i = 0; i < sgs; i++ ) {
				const ShapedGlyph* sg = &sp.shapedGlyphs[i];

				charLeft = sg->position.x;
				charTop = sg->position.y;
				charBottom = charTop + vspace;
				charRight = sg->position.x + sg->advance.x;

				// Expand bounds over the whole cluster (multiple glyphs for one string index)
				while ( i + 1 < sgs && sp.shapedGlyphs[i + 1].stringIndex == sg->stringIndex ) {
					i++;
					sg = &sp.shapedGlyphs[i];
					charBottom = sg->position.y + vspace;
					charRight = sg->position.x + sg->advance.x;
				};

				if ( fpos.y >= charTop && fpos.y <= charBottom ) {
					auto findNextInsertionIndex = [&layout, sg, i, sgs, tSize, sp]() -> Int32 {
						if ( layout->isRTL() ) {
							if ( i > 0 ) {
								for ( auto j = i - 1; j >= 0; j-- ) {
									if ( sp.shapedGlyphs[j].stringIndex > sg->stringIndex )
										return sp.shapedGlyphs[j].stringIndex;
								}
							}
							return 0;
						} else {
							for ( auto j = i + 1; j < sgs; ++j ) {
								if ( sp.shapedGlyphs[j].stringIndex > sg->stringIndex )
									return sp.shapedGlyphs[j].stringIndex;
							}
							return tSize;
						}
					};

					if ( fpos.x >= charLeft && fpos.x < charRight ) {
						Float midPoint = charLeft + ( charRight - charLeft ) * 0.5f;
						if ( fpos.x < midPoint ) {
							return sg->stringIndex;
						} else {
							return findNextInsertionIndex();
						}
					}
				}

				if ( i == sgs - 1 && fpos.x > charRight ) {
					return tSize;
				}

				if ( returnNearest ) {
					Vector2f cellCenter( charLeft + ( charRight - charLeft ) * 0.5f,
										 charTop + vspace * 0.5f );
					Int32 dist = static_cast<Int32>( fpos.distance( cellCenter ) );
					if ( dist < minDist ) {
						minDist = dist;
						nearest = sg->stringIndex;
					}
				}
			}
		}
		return ( sgs && nearest == sgs - 1 && fpos.x >= charRight ) ? tSize : nearest;
	}
#endif

	for ( std::size_t i = 0; i < tSize; ++i ) {
		codepoint = string[i];
		Glyph glyph = font->getGlyph( codepoint, fontSize, bold, italic, outlineThickness );

		lWidth = width;

		if ( codepoint != '\r' && codepoint != '\t' ) {
			if ( !( textDrawHints & TextHints::NoKerning ) ) {
				width += font->getKerning( prevChar, codepoint, fontSize, bold, italic,
										   outlineThickness );
			}
			prevChar = codepoint;
			width += glyph.advance;
		} else if ( codepoint == '\t' ) {
			width += tabAdvance( hspace, tabWidth, tabOffset ? *tabOffset + width : tabOffset );
		}

		if ( codepoint == '\n' ) {
			lWidth = 0;
			width = 0;
			fpos.x = static_cast<Float>( pos.x );
		}

		if ( fpos.x <= width && fpos.x >= lWidth && fpos.y <= height && fpos.y >= lHeight ) {
			if ( i + 1 <= tSize ) {
				Float tcurDist = std::abs( fpos.x - lWidth );
				Float nextDist = std::abs( fpos.x - width );
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

		if ( codepoint == '\n' ) {
			lHeight = height;
			height += vspace;
			// After newline, X offset no longer applies (subsequent lines start at x=0)
			fpos.x = static_cast<Float>( pos.x );
		}
	}

	if ( fpos.x >= width )
		return tSize;
	return nearest;
}

std::size_t Text::findLastCharPosWithinLength( Font* font, const Uint32& fontSize,
											   const String& string, Float maxWidth,
											   const Uint32& style, const Uint32& tabWidth,
											   const Float& outlineThickness,
											   std::optional<Float> tabOffset, Uint32 textHints,
											   TextDirection direction ) {
	return findLastCharPosWithinLength<String>( font, fontSize, string, maxWidth, style, tabWidth,
												outlineThickness, tabOffset, textHints, direction );
}

std::size_t Text::findLastCharPosWithinLength( Font* font, const Uint32& fontSize,
											   const String::View& string, Float maxWidth,
											   const Uint32& style, const Uint32& tabWidth,
											   const Float& outlineThickness,
											   std::optional<Float> tabOffset, Uint32 textHints,
											   TextDirection direction ) {
	return findLastCharPosWithinLength<String::View>( font, fontSize, string, maxWidth, style,
													  tabWidth, outlineThickness, tabOffset,
													  textHints, direction );
}

std::size_t Text::findLastCharPosWithinLength( const String& string, Float maxWidth,
											   const FontStyleConfig& config,
											   const Uint32& tabWidth,
											   std::optional<Float> tabOffset, Uint32 textHints,
											   TextDirection direction ) {
	return findLastCharPosWithinLength<String>( config.Font, config.CharacterSize, string, maxWidth,
												config.Style, tabWidth, config.OutlineThickness,
												tabOffset, textHints, direction );
}

std::size_t Text::findLastCharPosWithinLength( const String::View& string, Float maxWidth,
											   const FontStyleConfig& config,
											   const Uint32& tabWidth,
											   std::optional<Float> tabOffset, Uint32 textDrawHints,
											   TextDirection direction ) {
	return findLastCharPosWithinLength<String::View>(
		config.Font, config.CharacterSize, string, maxWidth, config.Style, tabWidth,
		config.OutlineThickness, tabOffset, textDrawHints, direction );
}

void Text::updateWidthCache() {
	if ( NULL == mFontStyleConfig.Font || mString.empty() )
		return;

	mLinesWidth.clear();

	Float width = 0;
	Float maxWidth = 0;
	bool bold = ( mFontStyleConfig.Style & Bold ) != 0;
	bool italic = ( mFontStyleConfig.Style & Italic ) != 0;
	Float hspace = static_cast<Float>( mFontStyleConfig.Font
										   ->getGlyph( L' ', mFontStyleConfig.CharacterSize, bold,
													   italic, mFontStyleConfig.OutlineThickness )
										   .advance );

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && mFontStyleConfig.Font->getType() == FontType::TTF &&
		 !canSkipShaping( mTextHints ) ) {
		auto layout = TextLayout::layout(
			mString, mFontStyleConfig.Font, mFontStyleConfig.CharacterSize, mFontStyleConfig.Style,
			mTabWidth, mFontStyleConfig.OutlineThickness, {}, mTextHints, mDirection );
		mLinesWidth = layout->getLinesWidth();
		mCachedWidth = layout->size.getWidth();
		return;
	}
#endif

	Uint32 codepoint;
	Uint32 prevChar = 0;
	size_t size = mString.size();
	for ( std::size_t i = 0; i < size; ++i ) {
		codepoint = mString[i];
		auto glyph =
			mFontStyleConfig.Font->getGlyph( codepoint, mFontStyleConfig.CharacterSize, bold,
											 italic, mFontStyleConfig.OutlineThickness );

		if ( codepoint != '\t' && codepoint != '\n' ) {
			if ( !( mTextHints & TextHints::NoKerning ) ) {
				auto kerning = mFontStyleConfig.Font->getKerning(
					prevChar, codepoint, mFontStyleConfig.CharacterSize, bold, italic,
					mFontStyleConfig.OutlineThickness );
				width += kerning;
			}

			width += glyph.advance;
		} else if ( codepoint == '\t' ) {
			width += tabAdvance( hspace, mTabWidth, mTabStops ? width : std::optional<Float>{} );
			codepoint = 0;
		} else if ( codepoint == '\r' ) {
			prevChar = 0;
			continue;
		}

		if ( codepoint == '\n' ) {
			mLinesWidth.push_back( width );
			width = 0;
			prevChar = 0;
		} else {
			maxWidth = eemax( maxWidth, width );
			prevChar = codepoint;
		}
	}

	if ( !mString.empty() && mString[mString.size() - 1] != '\n' )
		mLinesWidth.push_back( width );

	mCachedWidth = maxWidth;
}

void Text::hardWrapText( const Uint32& maxWidth ) {
	if ( hardWrapText( mString, maxWidth, mFontStyleConfig, mTabWidth, {}, mTextHints ) )
		invalidate();
}

void Text::invalidateColors() {
	mColorsNeedUpdate = true;
}

void Text::invalidate() {
	mCachedWidthNeedUpdate = true;
	mGeometryNeedUpdate = true;
	mColorsNeedUpdate = true;
	mVisualLinesNeedUpdate = true;
}

void Text::setTabWidth( const Uint32& tabWidth ) {
	if ( mTabWidth != tabWidth ) {
		mTabWidth = tabWidth;

		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
		mVisualLinesNeedUpdate = true;
	}
}

const Uint32& Text::getTabWidth() const {
	return mTabWidth;
}

Color Text::getBackgroundColor() const {
	return mFontStyleConfig.BackgroundColor;
}

void Text::setBackgroundColor( const Color& backgroundColor ) {
	mFontStyleConfig.BackgroundColor = backgroundColor;
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

Uint32 Text::getTextHints() const {
	return mTextHints;
}

void Text::setTextHints( Uint32 textHints ) {
	mTextHints = textHints;
}

void Text::draw( const Float& X, const Float& Y, const Vector2f& scale, const Float& rotation,
				 BlendMode effect, const OriginPoint& rotationCenter,
				 const OriginPoint& scaleCenter, const std::vector<Color>& colors,
				 const std::vector<Color>& outlineColors, const Color& backgroundColor ) {
	if ( NULL == mFontStyleConfig.Font || mString.empty() )
		return;

	unsigned int numvert = mVertices.size();

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
		ensureVisualLinesUpdate();
		Float vspace = getLineSpacing();
		for ( size_t i = 0; i < mVisualLines.size(); ++i ) {
			Float centerDiffX = 0;
			if ( i < mLinesWidth.size() ) {
				switch ( Font::getHorizontalAlign( mAlign ) ) {
					case TEXT_ALIGN_CENTER:
						centerDiffX = std::trunc( ( mCachedWidth - mLinesWidth[i] ) * 0.5f );
						break;
					case TEXT_ALIGN_RIGHT:
						centerDiffX = mCachedWidth - mLinesWidth[i];
						break;
				}
			}
			p.drawRectangle( Rectf( centerDiffX, i * vspace, centerDiffX + mLinesWidth[i],
									( i + 1 ) * vspace ) );
		}
	}

	if ( 0 == numvert ) {
		if ( rotation != 0.0f || scale != 1.0f ) {
			GLi->popMatrix();
		} else {
			GLi->translatef( -X, -Y, 0 );
		}
		return;
	}

	Texture* texture = mFontStyleConfig.Font->getTexture( mFontStyleConfig.CharacterSize );
	if ( !texture )
		return;
	texture->bind();
	BlendMode::setMode( effect );

	Uint32 alloc = numvert * sizeof( VertexCoords );
	Uint32 allocC = numvert * GLi->quadVertex();

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

	if ( mInvalidationId != Text::GlobalInvalidationId ) {
		mInvalidationId = Text::GlobalInvalidationId;
		invalidate();
	}

	ensureGeometryUpdate();
	ensureColorUpdate();

	if ( mFontStyleConfig.Style & Shadow ) {
		std::vector<Color> colors;
		Color shadowColor( getShadowColor() );
		if ( getFillColor().a != 255 ) {
			shadowColor.a =
				(Uint8)( (Float)shadowColor.a * ( (Float)getFillColor().a / (Float)255 ) );
		}
		colors.assign( mColors.size(), shadowColor );
		draw( X + mFontStyleConfig.ShadowOffset.x, Y + mFontStyleConfig.ShadowOffset.y, scale,
			  rotation, effect, rotationCenter, scaleCenter, colors,
			  mFontStyleConfig.OutlineThickness > 0 ? mOutlineColors : std::vector<Color>{},
			  Color::Transparent );
	}

	draw( X, Y, scale, rotation, effect, rotationCenter, scaleCenter, mColors, mOutlineColors,
		  mFontStyleConfig.BackgroundColor );
}

void Text::ensureGeometryUpdate() {
	// For soft wrapping, pre-compute visual line info and wrapping padding
	bool useSoftWrap = mLineWrapMode != LineWrapMode::NoWrap && mMaxWrapWidth > 0;

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

	mVertices.reserve( mString.size() * GLi->quadVertex() );

	if ( mFontStyleConfig.OutlineThickness != 0.f )
		mOutlineVertices.reserve( mString.size() * GLi->quadVertex() );

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
	Glyph hglyph =
		mFontStyleConfig.Font->getGlyph( L' ', mFontStyleConfig.CharacterSize, bold, reqItalic );
	Float hspace = static_cast<Float>( hglyph.advance );
	Float vspace = static_cast<Float>(
		mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize ) );
	Float x = mInitialOffset.x;
	Float y = mFontStyleConfig.CharacterSize;

	// Create one quad for each character
	Float minX = mFontStyleConfig.CharacterSize;
	Float minY = mFontStyleConfig.CharacterSize;
	Float maxX = 0.f;
	Float maxY = 0.f;
	Float maxW = 0.f;
	Uint32 prevChar = 0;

	size_t size = mString.size();
	Float centerDiffX = 0;
	unsigned int line = 0;

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && mFontStyleConfig.Font->getType() == FontType::TTF &&
		 !canSkipShaping( mTextHints ) ) {
		FontTrueType* rFont = static_cast<FontTrueType*>( mFontStyleConfig.Font );
		auto layout = TextLayout::layout(
			mString, rFont, mFontStyleConfig.CharacterSize, mFontStyleConfig.Style, mTabWidth,
			mFontStyleConfig.OutlineThickness, {}, mTextHints, mDirection, mLineWrapMode,
			mMaxWrapWidth, false, mInitialOffset.x );

		mLinesWidth = layout->getLinesWidth();
		mCachedWidth = layout->size.getWidth();

		for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
			for ( const ShapedGlyph& sg : sp.shapedGlyphs ) {
				if ( mString[sg.stringIndex] == '\t' )
					continue;

				Float currentX = sg.position.x;
				Float currentY = y + sg.position.y;

				line = std::floor( sg.position.y / vspace );

				switch ( Font::getHorizontalAlign( mAlign ) ) {
					case TEXT_ALIGN_CENTER:
						centerDiffX =
							line < mLinesWidth.size()
								? std::trunc( ( mCachedWidth - mLinesWidth[line] ) * 0.5f )
								: 0.f;
						break;
					case TEXT_ALIGN_RIGHT:
						centerDiffX =
							line < mLinesWidth.size() ? mCachedWidth - mLinesWidth[line] : 0.f;
						break;
				}

				// Apply the outline
				if ( mFontStyleConfig.OutlineThickness != 0 ) {
					Glyph glyph = sg.font->getGlyphByIndex(
						sg.glyphIndex, mFontStyleConfig.CharacterSize, bold, reqItalic,
						mFontStyleConfig.OutlineThickness,
						rFont->getPage( mFontStyleConfig.CharacterSize ) );

					Float left = glyph.bounds.Left;
					Float top = glyph.bounds.Top;
					Float right = glyph.bounds.Left + glyph.bounds.Right;
					Float bottom = glyph.bounds.Top + glyph.bounds.Bottom;

					// Add the outline glyph to the vertices
					if ( glyph.bounds.Right > 0 && glyph.bounds.Bottom > 0 ) {
						addGlyphQuad( mOutlineVertices, Vector2f( currentX, currentY ), glyph,
									  italic, mFontStyleConfig.OutlineThickness, centerDiffX );
					}

					// Update the current bounds with the outlined glyph bounds
					minX = std::min( minX, currentX + left - italic * bottom -
											   mFontStyleConfig.OutlineThickness );
					maxX = std::max( maxX, currentX + right - italic * top -
											   mFontStyleConfig.OutlineThickness );
					minY = std::min( minY, currentY + top - mFontStyleConfig.OutlineThickness );
					maxY = std::max( maxY, currentY + bottom - mFontStyleConfig.OutlineThickness );
					maxW = std::max( maxW, currentX + glyph.advance - italic * top -
											   mFontStyleConfig.OutlineThickness );
				}

				// Extract the current glyph's description
				Glyph glyph = sg.font->getGlyphByIndex(
					sg.glyphIndex, mFontStyleConfig.CharacterSize, bold, reqItalic, 0,
					rFont->getPage( mFontStyleConfig.CharacterSize ) );

				Float left = glyph.bounds.Left;
				Float top = glyph.bounds.Top;
				Float right =
					glyph.bounds.Left + ( glyph.bounds.Right ? glyph.bounds.Right : glyph.advance );
				Float bottom = glyph.bounds.Top + glyph.bounds.Bottom;

				// Add a quad for the current character
				if ( glyph.bounds.Right > 0 && glyph.bounds.Bottom > 0 ) {
					addGlyphQuad( mVertices, Vector2f( currentX, currentY ), glyph, italic, 0,
								  centerDiffX );
				}

				// Update the current bounds
				minX = std::min( minX, currentX + left - italic * bottom );
				maxX = std::max( maxX, currentX + right - italic * top );
				minY = std::min( minY, currentY + top );
				maxY = std::max( maxY, currentY + bottom );
				maxW = std::max( maxW, currentX + glyph.advance - italic * top );
			}
		}

		// If we're using the underlined style, add the last line
		if ( underlined ) {
			Float lineTop = y;

			for ( size_t lineIdx = 0; lineIdx < mLinesWidth.size(); lineIdx++ ) {
				switch ( Font::getHorizontalAlign( mAlign ) ) {
					case TEXT_ALIGN_CENTER:
						centerDiffX =
							lineIdx < mLinesWidth.size()
								? std::trunc( ( mCachedWidth - mLinesWidth[lineIdx] ) * 0.5f )
								: 0.f;
						break;
					case TEXT_ALIGN_RIGHT:
						centerDiffX = lineIdx < mLinesWidth.size()
										  ? mCachedWidth - mLinesWidth[lineIdx]
										  : 0.f;
						break;
				}

				addLine( mVertices, mLinesWidth[lineIdx], lineTop, underlineOffset,
						 underlineThickness, 0, centerDiffX );

				if ( mFontStyleConfig.OutlineThickness != 0 )
					addLine( mOutlineVertices, mLinesWidth[lineIdx], lineTop, underlineOffset,
							 underlineThickness, mFontStyleConfig.OutlineThickness, centerDiffX );

				lineTop += vspace;
			}
		}

		// If we're using the strike through style, add the last line across all characters
		if ( strikeThrough ) {
			Float lineTop = y;

			for ( size_t lineIdx = 0; lineIdx < mLinesWidth.size(); lineIdx++ ) {
				switch ( Font::getHorizontalAlign( mAlign ) ) {
					case TEXT_ALIGN_CENTER:
						centerDiffX =
							lineIdx < mLinesWidth.size()
								? std::trunc( ( mCachedWidth - mLinesWidth[lineIdx] ) * 0.5f )
								: 0.f;
						break;
					case TEXT_ALIGN_RIGHT:
						centerDiffX = lineIdx < mLinesWidth.size()
										  ? mCachedWidth - mLinesWidth[lineIdx]
										  : 0.f;
						break;
				}

				addLine( mVertices, mLinesWidth[lineIdx], lineTop, strikeThroughOffset,
						 underlineThickness, 0, centerDiffX );

				if ( mFontStyleConfig.OutlineThickness != 0 )
					addLine( mOutlineVertices, mLinesWidth[lineIdx], lineTop, strikeThroughOffset,
							 underlineThickness, mFontStyleConfig.OutlineThickness, centerDiffX );

				lineTop += vspace;
			}
		}

		// Update the bounding rectangle
		mBounds.Left = minX;
		mBounds.Top = minY;
		mBounds.Right = maxX;
		mBounds.Bottom = maxY;

		if ( mCachedWidthNeedUpdate ) {
			mCachedWidth = maxW;
			mCachedWidthNeedUpdate = false;
		}

		return;
	}
#endif

	Float wrappedLinePadding = 0.f;
	size_t currentVisualLine = 0;

	if ( useSoftWrap ) {
		ensureVisualLinesUpdate();
		// Compute indentation padding for wrapped lines
		if ( mLineWrapKeepIndentation && !mVisualLines.empty() && mVisualLines.size() > 1 ) {
			// Find the first paragraph's indentation
			wrappedLinePadding = LineWrap::computeOffsets( mString.view(), mFontStyleConfig,
														   mTabWidth, mMaxWrapWidth, mTabStops );
		}
	}

	auto isSoftWrapLineStart = [this, &useSoftWrap, &currentVisualLine]( Int64 idx ) -> bool {
		return !( !useSoftWrap || currentVisualLine + 1 >= mVisualLines.size() ) &&
			   idx == mVisualLines[currentVisualLine + 1] && idx > 0 && mString[idx] != '\n';
	};

	auto updateAlignmentForLine = [this, &centerDiffX, &line]() {
		switch ( Font::getHorizontalAlign( mAlign ) ) {
			case TEXT_ALIGN_CENTER:
				centerDiffX =
					line < mLinesWidth.size()
						? (Float)( (Int32)( ( mCachedWidth - mLinesWidth[line] ) * 0.5f ) )
						: 0.f;
				break;
			case TEXT_ALIGN_RIGHT:
				centerDiffX = line < mLinesWidth.size() ? mCachedWidth - mLinesWidth[line] : 0.f;
				break;
			default:
				centerDiffX = 0.f;
				break;
		}
	};

	updateAlignmentForLine();
	line++;

	for ( std::size_t i = 0; i < size; ++i ) {
		Uint32 curChar = mString[i];

		// Check for soft wrap at this position (before processing the character)
		if ( isSoftWrapLineStart( i ) ) {
			// Draw underline/strikethrough for the previous visual line
			if ( underlined && x > 0 ) {
				addLine( mVertices, x, y, underlineOffset, underlineThickness, 0, centerDiffX );
				if ( mFontStyleConfig.OutlineThickness != 0 )
					addLine( mOutlineVertices, x, y, underlineOffset, underlineThickness,
							 mFontStyleConfig.OutlineThickness, centerDiffX );
			}
			if ( strikeThrough && x > 0 ) {
				addLine( mVertices, x, y, strikeThroughOffset, underlineThickness, 0, centerDiffX );
				if ( mFontStyleConfig.OutlineThickness != 0 )
					addLine( mOutlineVertices, x, y, strikeThroughOffset, underlineThickness,
							 mFontStyleConfig.OutlineThickness, centerDiffX );
			}

			// Move to next visual line
			currentVisualLine++;
			y += vspace;

			// Apply wrapping indentation if keeping indentation
			// Only apply padding to continuation lines within the same paragraph
			bool isParagraphContinuation = currentVisualLine > 0 &&
										   mVisualLines[currentVisualLine] > 0 &&
										   mString[mVisualLines[currentVisualLine] - 1] != '\n';
			x = ( mLineWrapKeepIndentation && isParagraphContinuation ) ? wrappedLinePadding : 0.f;

			prevChar = 0;

			// Update alignment for new line
			updateAlignmentForLine();
			line++;

			// Update bounds
			minX = std::min( minX, x );
			minY = std::min( minY, y );
		}

		// Apply the kerning offset
		if ( curChar != '\n' && curChar != '\r' ) {
			if ( !( mTextHints & TextHints::NoKerning ) ) {
				x += mFontStyleConfig.Font->getKerning(
					prevChar, curChar, mFontStyleConfig.CharacterSize, bold, reqItalic,
					mFontStyleConfig.OutlineThickness );
			}
			prevChar = curChar;
		} else
			prevChar = 0;

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
			// Advance to next visual line (the one after the newline)
			if ( useSoftWrap ) {
				currentVisualLine++;
			}

			updateAlignmentForLine();
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
					x += tabAdvance( hspace, mTabWidth, mTabStops ? x : std::optional<Float>{} );
					break;
				case '\n':
					y += vspace;

					if ( mCachedWidthNeedUpdate && !useSoftWrap )
						mLinesWidth.push_back( x );

					x = 0;
					prevChar = 0;
					break;
				case '\r':
					break;
			}

			// Update the current bounds (max coordinates)
			maxX = std::max( maxX, x );
			maxY = std::max( maxY, y );

			if ( mCachedWidthNeedUpdate && !useSoftWrap )
				maxW = std::max( maxW, x );

			// Next glyph, no need to create a quad for whitespace
			continue;
		}

		// Apply the outline
		if ( mFontStyleConfig.OutlineThickness != 0 ) {
			auto glyph =
				mFontStyleConfig.Font->getGlyph( curChar, mFontStyleConfig.CharacterSize, bold,
												 reqItalic, mFontStyleConfig.OutlineThickness );

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
		auto glyph = mFontStyleConfig.Font->getGlyph( curChar, mFontStyleConfig.CharacterSize, bold,
													  reqItalic );

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

	if ( mCachedWidthNeedUpdate && !useSoftWrap && !mString.empty() && mString.back() != '\n' ) {
		mLinesWidth.push_back( x );
	}

	// Update the bounding rectangle
	mBounds.Left = minX;
	mBounds.Top = minY;
	mBounds.Right = maxX;
	mBounds.Bottom = maxY;

	if ( mCachedWidthNeedUpdate && !useSoftWrap ) {
		mCachedWidth = maxW;
		mCachedWidthNeedUpdate = false;
	}

	// For soft wrap, the width cache was already handled by ensureVisualLinesUpdate
	if ( useSoftWrap && mCachedWidthNeedUpdate ) {
		mCachedWidthNeedUpdate = false;
		if ( !mLinesWidth.empty() )
			mCachedWidth = *std::max_element( mLinesWidth.begin(), mLinesWidth.end() );
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
			for ( const auto& position : positions ) {
				setFillColor( Color( 255, 255, 255, mFontStyleConfig.FontColor.a ), position,
							  position );
			}
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

Float Text::getLastLineWidth() {
	cacheWidth();

	return mLinesWidth.empty() ? 0 : mLinesWidth.back();
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

	bool useSoftWrap = mLineWrapMode != LineWrapMode::NoWrap && mMaxWrapWidth > 0;

	if ( useSoftWrap ) {
		ensureVisualLinesUpdate();
		return;
	}

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
	setBackgroundColor( styleConfig.BackgroundColor );
}

bool Text::hasSameFontStyleConfig( const FontStyleConfig& styleConfig ) {
	return mFontStyleConfig == styleConfig;
}

void Text::setFillColor( const Color& color, Uint32 from, Uint32 to ) {
	if ( mString.empty() )
		return;

	ensureGeometryUpdate();
	ensureColorUpdate();

	size_t numVerts = mVertices.size();
	if ( mColors.size() < numVerts ) {
		mColors.resize( numVerts, mFontStyleConfig.FontColor );
		mColorsNeedUpdate = false;
	}

	bool underlined = ( mFontStyleConfig.Style & Underlined ) != 0;
	bool strikeThrough = ( mFontStyleConfig.Style & StrikeThrough ) != 0;
	std::size_t s = mString.size();

	if ( to >= s ) {
		to = s - 1;
	}

	if ( from <= to && from < s && to <= s ) {
#ifdef EE_TEXT_SHAPER_ENABLED
		if ( TextShaperEnabled && mFontStyleConfig.Font->getType() == FontType::TTF &&
			 !canSkipShaping( mTextHints ) ) {
			FontTrueType* rFont = static_cast<FontTrueType*>( mFontStyleConfig.Font );
			auto layout = TextLayout::layout(
				mString, rFont, mFontStyleConfig.CharacterSize, mFontStyleConfig.Style, mTabWidth,
				mFontStyleConfig.OutlineThickness, {}, mTextHints, mDirection );
			size_t vIdx = 0;
			bool bold = ( mFontStyleConfig.Style & Bold ) != 0;
			bool italic = ( mFontStyleConfig.Style & Italic ) != 0;

			for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
				for ( const ShapedGlyph& sg : sp.shapedGlyphs ) {
					if ( mString[sg.stringIndex] == '\t' )
						continue;

					Glyph glyph = sg.font->getGlyphByIndex(
						sg.glyphIndex, mFontStyleConfig.CharacterSize, bold, italic,
						mFontStyleConfig.OutlineThickness,
						rFont->getPage( mFontStyleConfig.CharacterSize ) );

					if ( glyph.bounds.Right > 0 && glyph.bounds.Bottom > 0 ) {
						if ( vIdx + GLi->quadVertex() <= mColors.size() && sg.stringIndex >= from &&
							 sg.stringIndex <= to ) {
							for ( int i = 0; i < GLi->quadVertex(); ++i )
								mColors[vIdx + i] = color;
						}
						vIdx += GLi->quadVertex();
					}
				}
			}

			mColorsNeedUpdate = false;
			return;
		}
#endif

		size_t realTo = to + 1;
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

		for ( i = from; i < realTo; i++ ) {
			curChar = mString[i];

			lpos = rpos;
			rpos++;

			// Same here
			if ( ' ' == curChar || '\n' == curChar || '\t' == curChar || '\r' == curChar ) {
				if ( rpos > 0 ) {
					rpos--;

					if ( '\n' == curChar ) {
						if ( underlined || strikeThrough ) {
							for ( int v = 0; v < GLi->quadVertex(); v++ )
								mColors[rpos * GLi->quadVertex() + v] = color;
						}

						if ( underlined )
							rpos++;

						if ( strikeThrough )
							rpos++;
					}
				}
			} else {
				for ( int v = 0; v < GLi->quadVertex(); v++ )
					mColors[lpos * GLi->quadVertex() + v] = color;
			}
		}

		if ( to == s ) {
			if ( underlined ) {
				lpos++;
				Uint32 pos = lpos * GLi->quadVertex();

				if ( pos < mColors.size() ) {
					for ( int v = 0; v < GLi->quadVertex(); v++ )
						mColors[lpos * GLi->quadVertex() + v] = color;
				}
			}

			if ( strikeThrough ) {
				lpos++;
				Uint32 pos = lpos * GLi->quadVertex();

				if ( pos < mColors.size() ) {
					for ( int v = 0; v < GLi->quadVertex(); v++ )
						mColors[lpos * GLi->quadVertex() + v] = color;
				}
			}
		}
	}
}

void Text::setFillColor( const std::vector<Color>& colors ) {
	if ( mString.empty() || colors.empty() )
		return;

	ensureGeometryUpdate();
	ensureColorUpdate();

	size_t numVerts = mVertices.size();
	if ( mColors.size() < numVerts ) {
		mColors.resize( numVerts, mFontStyleConfig.FontColor );
		mColorsNeedUpdate = false;
	}

#ifdef EE_TEXT_SHAPER_ENABLED
	if ( TextShaperEnabled && mFontStyleConfig.Font->getType() == FontType::TTF &&
		 !canSkipShaping( mTextHints ) ) {
		FontTrueType* rFont = static_cast<FontTrueType*>( mFontStyleConfig.Font );
		auto layout = TextLayout::layout(
			mString, rFont, mFontStyleConfig.CharacterSize, mFontStyleConfig.Style, mTabWidth,
			mFontStyleConfig.OutlineThickness, {}, mTextHints, mDirection );
		size_t vIdx = 0;
		bool bold = ( mFontStyleConfig.Style & Bold ) != 0;
		bool italic = ( mFontStyleConfig.Style & Italic ) != 0;

		for ( const ShapedTextParagraph& sp : layout->paragraphs ) {
			for ( const ShapedGlyph& sg : sp.shapedGlyphs ) {
				if ( mString[sg.stringIndex] == '\t' )
					continue;

				Glyph glyph =
					sg.font->getGlyphByIndex( sg.glyphIndex, mFontStyleConfig.CharacterSize, bold,
											  italic, mFontStyleConfig.OutlineThickness,
											  rFont->getPage( mFontStyleConfig.CharacterSize ) );

				if ( glyph.bounds.Right > 0 && glyph.bounds.Bottom > 0 ) {
					if ( vIdx + GLi->quadVertex() <= mColors.size() &&
						 sg.stringIndex < colors.size() ) {
						Color color = colors[sg.stringIndex];
						if ( mContainsColorEmoji &&
							 Font::isEmojiCodePoint( mString[sg.stringIndex] ) )
							color = Color( 255, 255, 255, color.a );
						for ( int i = 0; i < GLi->quadVertex(); ++i )
							mColors[vIdx + i] = color;
						vIdx += GLi->quadVertex();
					}
				}
			}
		}

		mColorsNeedUpdate = false;
		return;
	}
#endif

	size_t s = mString.size();
	size_t vIdx = 0;

	for ( size_t i = 0; i < s; i++ ) {
		String::StringBaseType curChar = mString[i];
		if ( ' ' == curChar || '\n' == curChar || '\t' == curChar || '\r' == curChar )
			continue;

		if ( vIdx + GLi->quadVertex() <= mColors.size() && i < colors.size() ) {
			Color color = colors[i];
			if ( mContainsColorEmoji && Font::isEmojiCodePoint( curChar ) )
				color = Color( 255, 255, 255, color.a );
			for ( int v = 0; v < GLi->quadVertex(); v++ )
				mColors[vIdx + v] = color;
			vIdx += GLi->quadVertex();
		}
	}

	mColorsNeedUpdate = false;
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
	size_t sv = mString.size() * GLi->quadVertex();
	Uint32 skipped = 0;
	bool lineHasChars = false;

	for ( const auto& ch : mString ) {
		lineHasChars = true;

		if ( ' ' == ch || '\n' == ch || '\t' == ch || '\r' == ch ) {
			lineHasChars = false;
			skipped++;

			if ( '\n' == ch ) {
				if ( underlined )
					skipped--;

				if ( strikeThrough )
					skipped--;
			}
		}
	}

	if ( lineHasChars ) {
		if ( underlined )
			skipped--;

		if ( strikeThrough )
			skipped--;
	}

	sv -= skipped * GLi->quadVertex();

	return sv;
}

void Text::setDirection( TextDirection direction ) {
	if ( direction == mDirection )
		return;

	mDirection = direction;
	invalidate();
}

TextDirection Text::getDirection() const {
	return mDirection;
}

void Text::setInitialOffset( const Vector2f& offset ) {
	if ( mInitialOffset != offset ) {
		mInitialOffset = offset;
		mVisualLinesNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setLineWrapMode( LineWrapMode mode ) {
	if ( mLineWrapMode != mode ) {
		mLineWrapMode = mode;
		mVisualLinesNeedUpdate = true;
		mGeometryNeedUpdate = true;
		mCachedWidthNeedUpdate = true;
	}
}

void Text::setMaxWrapWidth( Float maxWidth ) {
	if ( mMaxWrapWidth != maxWidth ) {
		mMaxWrapWidth = maxWidth;
		if ( mLineWrapMode != LineWrapMode::NoWrap ) {
			mVisualLinesNeedUpdate = true;
			mGeometryNeedUpdate = true;
			mCachedWidthNeedUpdate = true;
		}
	}
}

void Text::setLineWrapKeepIndentation( bool keep ) {
	if ( mLineWrapKeepIndentation != keep ) {
		mLineWrapKeepIndentation = keep;
		if ( mLineWrapMode != LineWrapMode::NoWrap ) {
			mVisualLinesNeedUpdate = true;
			mGeometryNeedUpdate = true;
			mCachedWidthNeedUpdate = true;
		}
	}
}

void Text::ensureVisualLinesUpdate() {
	if ( mVisualLinesNeedUpdate ) {
		computeVisualLines();
		mVisualLinesNeedUpdate = false;
	}
}

void Text::computeVisualLines() {
	mVisualLines.clear();

	if ( mString.empty() || !mFontStyleConfig.Font ) {
		mVisualLines.push_back( { 0 } );
		mCachedWidth = 0;
		return;
	}

	LineWrapInfoEx wrapInfo = LineWrap::computeLineBreaksEx(
		mString, mFontStyleConfig, mMaxWrapWidth, mLineWrapMode, mLineWrapKeepIndentation,
		mTabWidth, 0.f, mTextHints, mTabStops, mInitialOffset.x );

	mVisualLines = std::move( wrapInfo.wraps );
	mLinesWidth = std::move( wrapInfo.wrapsWidth );
	if ( !mLinesWidth.empty() )
		mCachedWidth = *std::max_element( mLinesWidth.begin(), mLinesWidth.end() );

	mCachedWidthNeedUpdate = false;
	mVisualLinesNeedUpdate = false;
}

Uint32 Text::getVisualLineCount() {
	ensureVisualLinesUpdate();
	return static_cast<Uint32>( mVisualLines.size() );
}

void Text::forEachVisualLine( const VisualLineCallback& callback ) {
	ensureVisualLinesUpdate();
	for ( size_t i = 0; i < mVisualLines.size(); ++i ) {
		auto vl = mVisualLines[i];
		auto vle = i + 1 < mVisualLines.size() ? mVisualLines[i + 1] : mString.size();
		auto lw = mLinesWidth[i];
		callback( i, vl, vle, lw );
	}
}

size_t Text::findVisualLineFromCharIndex( size_t charIndex ) {
	ensureVisualLinesUpdate();
	Int64 visualLinesSize = mVisualLines.size();
	for ( Int64 i = 0; i < visualLinesSize; ++i ) {
		if ( static_cast<Int64>( charIndex ) >= mVisualLines[i] &&
			 static_cast<Int64>( charIndex ) < ( i + 1 < visualLinesSize
													 ? mVisualLines[i + 1]
													 : static_cast<Int64>( mString.size() ) ) ) {
			return i;
		}
	}
	// If at end of text, return last line
	if ( !mVisualLines.empty() && charIndex == mString.size() ) {
		return visualLinesSize - 1;
	}
	return 0;
}

std::vector<Rectf> Text::getSelectionRects( TextSelectionRange range ) {
	std::vector<Rectf> rects;

	if ( range.start == range.end || !mFontStyleConfig.Font )
		return rects;

	if ( range.start > range.end )
		std::swap( range.start, range.end );

	ensureVisualLinesUpdate();
	cacheWidth();

	size_t startLine = findVisualLineFromCharIndex( range.start );
	size_t endLine = findVisualLineFromCharIndex( range.end );
	Float hspace =
		mFontStyleConfig.Font
			->getGlyph( ' ', mFontStyleConfig.CharacterSize, mFontStyleConfig.Style & Text::Bold,
						mFontStyleConfig.Style & Text::Italic )
			.advance;
	Float vspace = static_cast<Float>(
		mFontStyleConfig.Font->getLineSpacing( mFontStyleConfig.CharacterSize ) );

	for ( size_t i = startLine; i <= endLine; ++i ) {
		Float top = i * vspace;
		Float bottom = top + vspace;
		Float left = 0;
		Float right = 0;
		Float centerDiffX = 0;

		if ( i < mLinesWidth.size() ) {
			switch ( Font::getHorizontalAlign( mAlign ) ) {
				case TEXT_ALIGN_CENTER:
					centerDiffX = std::trunc( ( mCachedWidth - mLinesWidth[i] ) * 0.5f );
					break;
				case TEXT_ALIGN_RIGHT:
					centerDiffX = mCachedWidth - mLinesWidth[i];
					break;
			}
		}

		// Calculate Left
		if ( i == startLine ) {
			left = findCharacterPos( range.start ).x;
		} else {
			left = centerDiffX;
		}

		// Calculate Right
		if ( i == endLine ) {
			// If it's a newline character, we select a small chunk to indicate the newline
			// selection
			if ( range.end < (Int64)mString.size() && mString[range.end] == '\n' ) {
				right = findCharacterPos( range.end ).x + hspace;
			} else {
				right = findCharacterPos( range.end ).x;
			}
		} else {
			right = centerDiffX + ( i < mLinesWidth.size() ? mLinesWidth[i] : 0 );
		}

		if ( left != right ) {
			rects.push_back( Rectf( left, top, right, bottom ) );
		}
	}

	return rects;
}

}} // namespace EE::Graphics
