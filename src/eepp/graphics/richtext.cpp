#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/linewrap.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/richtext.hpp>

namespace EE { namespace Graphics {

RichText* RichText::New() {
	return eeNew( RichText, () );
}

RichText::RichText() : Drawable( Drawable::RICHTEXT ) {}

RichText::~RichText() {}

void RichText::draw() {
	draw( mPosition.x, mPosition.y );
}

void RichText::draw( const Vector2f& position ) {
	draw( position.x, position.y );
}

void RichText::draw( const Vector2f& position, const Sizef& size ) {
	Sizef s = getSize();
	if ( s != Sizef::Zero ) {
		draw( position.x, position.y,
			  { size.getWidth() / s.getWidth(), size.getHeight() / s.getHeight() } );
	}
}

void RichText::draw( const Float& X, const Float& Y, const Vector2f& scale, const Float& rotation,
					 BlendMode effect, const OriginPoint& rotationCenter,
					 const OriginPoint& scaleCenter ) {
	updateLayout();

	for ( auto& line : mLines ) {
		for ( auto& span : line.spans ) {
			// span.position is local to the line (x) + baseline offset (y) line.Y is the line
			// vertical offset

			Vector2f pos = span.position;

			if ( rotation == 0 && scale == Vector2f::One ) {
				span.text->draw( std::trunc( X + pos.x ), std::trunc( Y + line.y + pos.y ),
								 Vector2f::One, 0, effect );
			} else {
				span.text->draw( std::trunc( X + pos.x * scale.x ),
								 std::trunc( Y + ( line.y + pos.y ) * scale.y ), scale, rotation,
								 effect, rotationCenter, scaleCenter );
			}
		}
	}
}

Sizef RichText::getPixelsSize() {
	return getSize();
}

void RichText::addSpan( const String& text, const FontStyleConfig& style ) {
	if ( text.empty() )
		return;

	auto span = std::make_shared<Text>();
	span->setString( text );
	span->setStyleConfig( style );
	mSpans.push_back( span );
	mNeedsLayoutUpdate = true;
}

void RichText::addSpan( const String& text, Font* font, Uint32 characterSize, Color color,
						Uint32 style ) {
	FontStyleConfig config;
	config.Font = font ? font : mDefaultStyle.Font;
	config.CharacterSize = characterSize != 0 ? characterSize : mDefaultStyle.CharacterSize;
	config.FontColor = color;
	config.Style = style;
	config.ShadowColor = mDefaultStyle.ShadowColor;
	config.ShadowOffset = mDefaultStyle.ShadowOffset;
	config.OutlineThickness = mDefaultStyle.OutlineThickness;
	config.OutlineColor = mDefaultStyle.OutlineColor;

	addSpan( text, config );
}

void RichText::clear() {
	mSpans.clear();
	mLines.clear();
	mNeedsLayoutUpdate = true;
}

void RichText::setFontStyleConfig( const FontStyleConfig& styleConfig ) {
	mDefaultStyle = styleConfig;
	mNeedsLayoutUpdate = true;
}

void RichText::setAlign( Uint32 align ) {
	if ( mAlign != align ) {
		mAlign = align;
		mNeedsLayoutUpdate = true;
	}
}

void RichText::setMaxWidth( Float width ) {
	if ( mMaxWidth != width ) {
		mMaxWidth = width;
		mNeedsLayoutUpdate = true;
	}
}

void RichText::invalidate() {
	mNeedsLayoutUpdate = true;
	for ( auto& span : mSpans ) {
		span->invalidate();
	}
}

void RichText::updateLayout() {
	if ( !mNeedsLayoutUpdate )
		return;

	mLines.clear();
	mLines.push_back( RenderParagraph() );

	Float curX = 0;
	Float maxWidth = 0;

	for ( auto& span : mSpans ) {
		if ( span->getString().empty() )
			continue;

		auto& fontStyle = span->getFontStyleConfig();
		if ( !fontStyle.Font )
			continue;

		Uint32 textHints = span->getTextHints();

		LineWrapInfoEx wrapInfo = LineWrap::computeLineBreaksEx(
			span->getString(), fontStyle, mMaxWidth > 0 ? mMaxWidth : 1e9f,
			mMaxWidth > 0 ? LineWrapMode::Word : LineWrapMode::NoWrap, false, 4, 0.f, textHints,
			false, curX );

		// Make sure we have the end of the string as a "wrap" point for the loop
		if ( wrapInfo.wraps.empty() || wrapInfo.wraps.back() != (Float)span->getString().size() )
			wrapInfo.wraps.push_back( span->getString().size() );

		for ( size_t i = 0; i < wrapInfo.wraps.size() - 1; ++i ) {
			size_t startIdx = wrapInfo.wraps[i];
			size_t endIdx = wrapInfo.wraps[i + 1];
			bool isNewline = ( endIdx - startIdx == 1 && span->getString()[startIdx] == '\n' );

			if ( !isNewline ) {
				std::shared_ptr<Text> renderSpanText = std::make_shared<Text>();
				renderSpanText->setString(
					span->getString().substr( startIdx, endIdx - startIdx ) );
				renderSpanText->setStyleConfig( fontStyle );

				RenderSpan renderSpan;
				renderSpan.text = renderSpanText;
				renderSpan.position = { curX, 0 }; // Y adjusted later

				RenderParagraph& currentLine = mLines.back();
				currentLine.spans.push_back( renderSpan );

				Float ascent = fontStyle.Font->getAscent( fontStyle.CharacterSize );
				Float height = fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );

				currentLine.maxAscent = std::max( currentLine.maxAscent, ascent );
				currentLine.height = std::max( currentLine.height, height );

				Float spanWidth = renderSpan.text->getTextWidth();
				curX += spanWidth;
				currentLine.width += spanWidth;
			}

			// If it's a newline, or if it's not the very last segment (which means it wrapped),
			// start a new line. Exception: If the last segment was just a newline, we already
			// handled it.
			if ( i < wrapInfo.wraps.size() - 2 || isNewline ) {
				maxWidth = std::max( maxWidth, curX );
				mLines.push_back( RenderParagraph() );
				curX = 0;
			}
		}
	}

	maxWidth = std::max( maxWidth, curX );

	if ( !mLines.empty() && mLines.back().spans.empty() && mLines.size() > 1 ) {
		mLines.pop_back();
	}

	Float curY = 0;
	for ( auto& line : mLines ) {
		line.y = curY;

		Float xOffset = 0;
		if ( mMaxWidth > 0 && mAlign != 0 ) {
			Uint32 hAlign = Font::getHorizontalAlign( mAlign );
			if ( hAlign == TEXT_ALIGN_CENTER ) {
				xOffset = ( mMaxWidth - line.width ) * 0.5f;
			} else if ( hAlign == TEXT_ALIGN_RIGHT ) {
				xOffset = mMaxWidth - line.width;
			}
		}

		for ( auto& span : line.spans ) {
			Float ascent = span.text->getFont()->getAscent( span.text->getCharacterSize() );
			Float offsetY = line.maxAscent - ascent;
			span.position.x += xOffset;
			span.position.y = offsetY;
		}

		curY += line.height;
	}

	mSize = Sizef( maxWidth, curY );
	mNeedsLayoutUpdate = false;
}

Sizef RichText::getSize() {
	updateLayout();
	return mSize;
}

}} // namespace EE::Graphics
