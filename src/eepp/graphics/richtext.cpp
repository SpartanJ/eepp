#include <eepp/core/overloaded.hpp>
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

	if ( mSelection.start != mSelection.end ) {
		auto rects = getSelectionRects();
		Primitives p;
		p.setColor( mSelectionBackColor );
		for ( const auto& rect : rects ) {
			p.drawRectangle(
				Rectf( rect.getPosition() * scale + Vector2f( X, Y ), rect.getSize() * scale ), 0.f,
				scale );
		}
	}

	for ( auto& line : mLines ) {
		for ( auto& span : line.spans ) {
			// span.position is local to the line (x) + baseline offset (y) line.Y is the line
			// vertical offset

			Vector2f pos = span.position;

			std::visit(
				Overloaded{

					[&]( const std::shared_ptr<Text>& text ) {
						bool selectionApplied = false;
						if ( mSelectionColor != Color::Transparent ) {
							TextSelectionRange spanSel = {
								std::clamp( mSelection.start, span.startCharIndex,
											span.endCharIndex ) -
									span.startCharIndex,
								std::clamp( mSelection.end, span.startCharIndex,
											span.endCharIndex ) -
									span.startCharIndex };

							if ( spanSel.start != spanSel.end ) {
								text->setFillColor(
									mSelectionColor, (Uint32)std::min( spanSel.start, spanSel.end ),
									(Uint32)std::max( spanSel.start, spanSel.end ) - 1 );
								selectionApplied = true;
							}
						}

						if ( rotation == 0 && scale == Vector2f::One ) {
							text->draw( std::trunc( X + pos.x ), std::trunc( Y + line.y + pos.y ),
										Vector2f::One, 0, effect );
						} else {
							text->draw( std::trunc( X + pos.x * scale.x ),
										std::trunc( Y + ( line.y + pos.y ) * scale.y ), scale,
										rotation, effect, rotationCenter, scaleCenter );
						}

						if ( selectionApplied )
							text->invalidateColors();
					},
					[&]( const std::shared_ptr<Drawable>& drawable ) {
						if ( drawable ) {
							drawable->draw( Vector2f( std::trunc( X + pos.x ),
													  std::trunc( Y + line.y + pos.y ) ),
											span.size );
						}
					},
					[]( const Sizef& ) {} },
				span.block );
		}
	}
}

void RichText::setSelection( TextSelectionRange range ) {
	if ( mSelection.start != range.start || mSelection.end != range.end ) {
		mSelection = range;
	}
}

void RichText::setSelectionColor( const Color& color ) {
	mSelectionColor = color;
}

void RichText::setSelectionBackColor( const Color& color ) {
	mSelectionBackColor = color;
}

Int64 RichText::getCharacterCount() const {
	const_cast<RichText*>( this )->updateLayout();
	return mTotalCharacterCount;
}

Int64 RichText::findCharacterFromPos( const Vector2i& pos ) const {
	const_cast<RichText*>( this )->updateLayout();

	for ( const auto& line : mLines ) {
		if ( pos.y >= line.y && pos.y < line.y + line.height ) {
			for ( const auto& span : line.spans ) {
				if ( pos.x >= span.position.x && pos.x < span.position.x + span.size.getWidth() ) {
					if ( auto pText = std::get_if<std::shared_ptr<Text>>( &span.block ) ) {
						return span.startCharIndex +
							   ( *pText )->findCharacterFromPos( Vector2i(
								   pos.x - span.position.x, pos.y - line.y - span.position.y ) );
					} else {
						return ( pos.x < span.position.x + span.size.getWidth() * 0.5f )
								   ? span.startCharIndex
								   : span.endCharIndex;
					}
				}
			}
			// If we are in the line but past the last span
			if ( !line.spans.empty() ) {
				if ( pos.x >= line.spans.back().position.x + line.spans.back().size.getWidth() ) {
					return line.spans.back().endCharIndex;
				} else if ( pos.x < line.spans.front().position.x ) {
					return line.spans.front().startCharIndex;
				}
			}
		}
	}

	// If we are past the last line
	if ( !mLines.empty() ) {
		if ( pos.y >= mLines.back().y + mLines.back().height ) {
			return getCharacterCount();
		} else if ( pos.y < mLines.front().y ) {
			return 0;
		}
	}

	return 0;
}

Vector2f RichText::findCharacterPos( Int64 index ) const {
	const_cast<RichText*>( this )->updateLayout();
	for ( const auto& line : mLines ) {
		for ( const auto& span : line.spans ) {
			if ( index >= span.startCharIndex && index < span.endCharIndex ) {
				if ( auto pText = std::get_if<std::shared_ptr<Text>>( &span.block ) ) {
					Vector2f p = ( *pText )->findCharacterPos( index - span.startCharIndex );
					return { span.position.x + p.x, line.y + span.position.y + p.y };
				} else {
					return { span.position.x, line.y + span.position.y };
				}
			}
		}
	}
	if ( index >= getCharacterCount() && !mLines.empty() && !mLines.back().spans.empty() ) {
		const auto& span = mLines.back().spans.back();
		return { span.position.x + span.size.getWidth(), mLines.back().y };
	}
	return { 0, 0 };
}

std::vector<Rectf> RichText::getSelectionRects() const {
	const_cast<RichText*>( this )->updateLayout();
	std::vector<Rectf> rects;
	if ( mSelection.start == mSelection.end )
		return rects;

	Int64 start = std::min( mSelection.start, mSelection.end );
	Int64 end = std::max( mSelection.start, mSelection.end );

	for ( const auto& line : mLines ) {
		for ( const auto& span : line.spans ) {
			Int64 spanStart = std::max( start, span.startCharIndex );
			Int64 spanEnd = std::min( end, span.endCharIndex );

			if ( spanStart < spanEnd ) {
				if ( auto pText = std::get_if<std::shared_ptr<Text>>( &span.block ) ) {
					auto spanRects = ( *pText )->getSelectionRects(
						{ spanStart - span.startCharIndex, spanEnd - span.startCharIndex } );
					for ( auto& rect : spanRects ) {
						rect.move( { span.position.x, line.y + span.position.y } );
						rects.push_back( rect );
					}
				} else {
					rects.push_back( Rectf( { span.position.x, line.y }, span.size ) );
				}
			}
		}
	}
	return rects;
}

String RichText::getSelectionString() const {
	const_cast<RichText*>( this )->updateLayout();
	if ( mSelection.start == mSelection.end )
		return "";

	Int64 start = std::min( mSelection.start, mSelection.end );
	Int64 end = std::max( mSelection.start, mSelection.end );
	String res;

	Int64 lastEndIdx = 0;
	for ( const auto& line : mLines ) {
		for ( const auto& span : line.spans ) {
			// Check if there was a newline before this span
			while ( lastEndIdx < span.startCharIndex ) {
				if ( lastEndIdx >= start && lastEndIdx < end ) {
					res += '\n';
				}
				lastEndIdx++;
			}

			Int64 spanStart = std::max( start, span.startCharIndex );
			Int64 spanEnd = std::min( end, span.endCharIndex );

			if ( spanStart < spanEnd ) {
				if ( auto pText = std::get_if<std::shared_ptr<Text>>( &span.block ) ) {
					res += ( *pText )->getString().substr( spanStart - span.startCharIndex,
														   spanEnd - spanStart );
				} else {
					// It's a drawable or custom size, it takes 1 "character" index.
					res += ' ';
				}
			}
			lastEndIdx = span.endCharIndex;
		}
	}

	// Check for trailing newlines
	while ( lastEndIdx < mTotalCharacterCount ) {
		if ( lastEndIdx >= start && lastEndIdx < end ) {
			res += '\n';
		}
		lastEndIdx++;
	}

	return res;
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
	mBlocks.push_back( span ); // Implicitly constructs the variant's Text alternative
	mNeedsLayoutUpdate = true;
}

void RichText::addDrawable( std::shared_ptr<Drawable> drawable ) {
	if ( !drawable )
		return;
	mBlocks.push_back( drawable );
	mNeedsLayoutUpdate = true;
}

void RichText::addCustomSize( const Sizef& size ) {
	mBlocks.push_back( size );
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
	mSelection = { 0, 0 };
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
		if ( auto pText = std::get_if<std::shared_ptr<Text>>( &block ) ) {
			if ( *pText )
				( *pText )->invalidate();
		}
	}
}

Float RichText::getMinIntrinsicWidth() {
	Float minW = 0;
	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<std::shared_ptr<Text>>( &block ) ) {
			auto& span = *pText;
			if ( !span || span->getString().empty() )
				continue;
			const String& s = span->getString();
			size_t start = 0;
			size_t end = 0;
			while ( start < s.size() ) {
				while ( start < s.size() && ( s[start] == ' ' || s[start] == '\t' ||
											  s[start] == '\n' || s[start] == '\r' ) )
					start++;
				end = start;
				while ( end < s.size() &&
						!( s[end] == ' ' || s[end] == '\t' || s[end] == '\n' || s[end] == '\r' ) )
					end++;
				if ( start < end ) {
					minW = std::max( minW, Text::getTextWidth( s.substr( start, end - start ),
															   span->getFontStyleConfig() ) );
				}
				start = end;
			}
		} else if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
			minW = std::max( minW, ( *pDrawable )->getPixelsSize().getWidth() );
		} else if ( auto pSize = std::get_if<Sizef>( &block ) ) {
			minW = std::max( minW, pSize->getWidth() );
		}
	}
	return minW;
}

Float RichText::getMaxIntrinsicWidth() {
	Float maxW = 0;
	Float curX = 0;
	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<std::shared_ptr<Text>>( &block ) ) {
			auto& span = *pText;
			if ( !span || span->getString().empty() )
				continue;

			const String& s = span->getString();
			size_t start = 0;
			size_t end = 0;
			while ( ( end = s.find( '\n', start ) ) != String::InvalidPos ) {
				curX += Text::getTextWidth( s.substr( start, end - start ),
											span->getFontStyleConfig() );
				maxW = std::max( maxW, curX );
				curX = 0;
				start = end + 1;
			}
			curX += Text::getTextWidth( s.substr( start ), span->getFontStyleConfig() );
		} else if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
			curX += ( *pDrawable )->getPixelsSize().getWidth();
		} else if ( auto pSize = std::get_if<Sizef>( &block ) ) {
			curX += pSize->getWidth();
		}
	}
	maxW = std::max( maxW, curX );
	return maxW;
}

void RichText::updateLayout() {
	if ( !mNeedsLayoutUpdate )
		return;

	mLines.clear();
	mLines.push_back( RenderParagraph() );

	Float curX = 0;
	Float maxWidth = 0;
	Int64 curCharIdx = 0;

	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<std::shared_ptr<Text>>( &block ) ) {
			auto& span = *pText;
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

					Float ascent = fontStyle.Font->getAscent( fontStyle.CharacterSize );
					Float height = fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );
					Float spanWidth = renderSpanText->getTextWidth();

					RenderSpan renderSpan;
					renderSpan.block = renderSpanText;
					renderSpan.position = { curX, 0 }; // Y adjusted later
					renderSpan.size =
						Sizef( spanWidth, height ); // Configured BEFORE pushing to vector
					renderSpan.startCharIndex = curCharIdx;
					renderSpan.endCharIndex = curCharIdx + ( endIdx - startIdx );
					curCharIdx = renderSpan.endCharIndex;

					RenderParagraph& currentLine = mLines.back();
					currentLine.spans.push_back( renderSpan );

					currentLine.maxAscent = std::max( currentLine.maxAscent, ascent );
					currentLine.height = std::max( currentLine.height, height );

					curX += spanWidth;
					currentLine.width += spanWidth;
				}

				// If it's a newline, or if it's not the very last segment (which means it wrapped),
				// start a new line. Exception: If the last segment was just a newline, we already
				// handled it.
				if ( i < wrapInfo.wraps.size() - 2 || isNewline ) {
					if ( isNewline ) {
						curCharIdx++;
					}
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}
			}
		} else { // Drawable or CustomSize
			Sizef blockSize;
			if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
				auto& drawable = *pDrawable;
				blockSize = drawable ? drawable->getPixelsSize() : Sizef();
			} else if ( auto pSize = std::get_if<Sizef>( &block ) ) {
				blockSize = *pSize;
			}

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
			renderSpan.startCharIndex = curCharIdx;
			renderSpan.endCharIndex = curCharIdx + 1;
			curCharIdx = renderSpan.endCharIndex;

			RenderParagraph& currentLine = mLines.back();
			currentLine.spans.push_back( renderSpan );

			currentLine.maxAscent = std::max( currentLine.maxAscent, blockSize.getHeight() );
			currentLine.height = std::max( currentLine.height, blockSize.getHeight() );

			curX += blockSize.getWidth();
			currentLine.width += blockSize.getWidth();

			if ( mMaxWidth > 0 && curX + blockSize.getWidth() > mMaxWidth && curX > 0 ) {
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

		Float maxLineHeight = 0;
		for ( auto& span : line.spans ) {
			if ( auto pText = std::get_if<std::shared_ptr<Text>>( &span.block ) ) {
				auto& textBlock = *pText;
				Float offsetY = line.maxAscent - textBlock->getCharacterSize();
				span.position.x += xOffset;
				span.position.y = offsetY;
				maxLineHeight = std::max( maxLineHeight, offsetY + span.size.getHeight() );
			} else {
				Float offsetY = line.maxAscent - span.size.getHeight();
				if ( offsetY < 0 )
					offsetY = 0;
				span.position.x += xOffset;
				span.position.y = offsetY;
				maxLineHeight = std::max( maxLineHeight, offsetY + span.size.getHeight() );
			}
		}

		line.height = std::max( line.height, maxLineHeight );
		curY += line.height;
	}

	mSize = Sizef( maxWidth, curY );
	mTotalCharacterCount = curCharIdx;
	mNeedsLayoutUpdate = false;
}

Sizef RichText::getSize() {
	updateLayout();
	return mSize;
}

}} // namespace EE::Graphics
