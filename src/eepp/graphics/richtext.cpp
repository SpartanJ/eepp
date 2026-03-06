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

			if ( span.block.type == BlockType::Text ) {
				if ( rotation == 0 && scale == Vector2f::One ) {
					span.block.text->draw( std::trunc( X + pos.x ),
										   std::trunc( Y + line.y + pos.y ), Vector2f::One, 0,
										   effect );
				} else {
					span.block.text->draw( std::trunc( X + pos.x * scale.x ),
										   std::trunc( Y + ( line.y + pos.y ) * scale.y ), scale,
										   rotation, effect, rotationCenter, scaleCenter );
				}
			} else if ( span.block.type == BlockType::Drawable && span.block.drawable ) {
				span.block.drawable->draw(
					Vector2f( std::trunc( X + pos.x ), std::trunc( Y + line.y + pos.y ) ),
					span.size );
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
	Block block;
	block.type = BlockType::Text;
	block.text = span;
	mBlocks.push_back( block );
	mNeedsLayoutUpdate = true;
}

void RichText::addDrawable( std::shared_ptr<Drawable> drawable ) {
	if ( !drawable )
		return;
	Block block;
	block.type = BlockType::Drawable;
	block.drawable = drawable;
	mBlocks.push_back( block );
	mNeedsLayoutUpdate = true;
}

void RichText::addCustomSize( const Sizef& size ) {
	Block block;
	block.type = BlockType::CustomSize;
	block.customSize = size;
	mBlocks.push_back( block );
	mNeedsLayoutUpdate = true;
}

void RichText::addSpan( const String& text, Font* font, Uint32 characterSize, Color color,
						Uint32 style, Color backgroundColor ) {
	FontStyleConfig config;
	config.Font = font ? font : mDefaultStyle.Font;
	config.CharacterSize = characterSize != 0 ? characterSize : mDefaultStyle.CharacterSize;
	config.FontColor = color;
	config.Style = style;
	config.ShadowColor = mDefaultStyle.ShadowColor;
	config.ShadowOffset = mDefaultStyle.ShadowOffset;
	config.OutlineThickness = mDefaultStyle.OutlineThickness;
	config.OutlineColor = mDefaultStyle.OutlineColor;
	config.BackgroundColor = backgroundColor;

	addSpan( text, config );
}

void RichText::clear() {
	mBlocks.clear();
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
	for ( auto& block : mBlocks ) {
		if ( block.type == BlockType::Text && block.text ) {
			block.text->invalidate();
		}
	}
}

void RichText::updateLayout() {
	if ( !mNeedsLayoutUpdate )
		return;

	mLines.clear();
	mLines.push_back( RenderParagraph() );

	Float curX = 0;
	Float maxWidth = 0;

	for ( auto& block : mBlocks ) {
		if ( block.type == BlockType::Text ) {
			auto& span = block.text;
			if ( !span || span->getString().empty() )
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
			if ( wrapInfo.wraps.empty() ||
				 wrapInfo.wraps.back() != (Float)span->getString().size() )
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

					Block newBlock;
					newBlock.type = BlockType::Text;
					newBlock.text = renderSpanText;

					RenderSpan renderSpan;
					renderSpan.block = newBlock;
					renderSpan.position = { curX, 0 }; // Y adjusted later

					RenderParagraph& currentLine = mLines.back();
					currentLine.spans.push_back( renderSpan );

					Float ascent = fontStyle.Font->getAscent( fontStyle.CharacterSize );
					Float height = fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );

					currentLine.maxAscent = std::max( currentLine.maxAscent, ascent );
					currentLine.height = std::max( currentLine.height, height );

					Float spanWidth = renderSpan.block.text->getTextWidth();
					renderSpan.size = Sizef( spanWidth, height );
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
		} else if ( block.type == BlockType::Drawable || block.type == BlockType::CustomSize ) {
			Sizef blockSize = block.type == BlockType::Drawable
								  ? ( block.drawable ? block.drawable->getPixelsSize() : Sizef() )
								  : block.customSize;

			// Wrap if needed
			if ( mMaxWidth > 0 && curX + blockSize.getWidth() > mMaxWidth && curX > 0 ) {
				maxWidth = std::max( maxWidth, curX );
				mLines.push_back( RenderParagraph() );
				curX = 0;
			}

			RenderSpan renderSpan;
			renderSpan.block = block;
			renderSpan.position = { curX, 0 };
			renderSpan.size = blockSize;

			RenderParagraph& currentLine = mLines.back();
			currentLine.spans.push_back( renderSpan );

			currentLine.maxAscent = std::max( currentLine.maxAscent, blockSize.getHeight() );
			currentLine.height = std::max( currentLine.height, blockSize.getHeight() );

			curX += blockSize.getWidth();
			currentLine.width += blockSize.getWidth();
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
			if ( span.block.type == BlockType::Text ) {
				Float ascent =
					span.block.text->getFont()->getAscent( span.block.text->getCharacterSize() );
				Float offsetY = line.maxAscent - ascent;
				span.position.x += xOffset;
				span.position.y = offsetY;
			} else {
				Float offsetY = line.height - span.size.getHeight();
				if ( offsetY < 0 )
					offsetY = 0;
				span.position.x += xOffset;
				span.position.y = offsetY;
			}
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
