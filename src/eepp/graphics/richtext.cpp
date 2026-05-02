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

					[&]( const SpanBlock& spanBlock ) {
						const std::shared_ptr<Text>& text = spanBlock.text;
						Color oldBgColor = text->getFontStyleConfig().BackgroundColor;

						if ( oldBgColor != Color::Transparent ) {
							Primitives p;
							p.setColor( oldBgColor );
							Rectf bgRect(
								Vector2f(
									std::trunc( X + pos.x - spanBlock.padding.Left ),
									std::trunc( Y + line.y + pos.y - spanBlock.padding.Top ) ),
								Sizef( span.size.getWidth() + spanBlock.padding.Left +
										   spanBlock.padding.Right,
									   span.size.getHeight() + spanBlock.padding.Top +
										   spanBlock.padding.Bottom ) );
							p.drawRectangle( bgRect, rotation, scale );
						}

						if ( oldBgColor != Color::Transparent )
							text->setBackgroundColor( Color::Transparent );

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

						if ( oldBgColor != Color::Transparent )
							text->setBackgroundColor( oldBgColor );

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
					[]( const CustomBlock& ) {} },
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
					if ( auto pText = std::get_if<SpanBlock>( &span.block ) ) {
						return span.startCharIndex +
							   pText->text->findCharacterFromPos( Vector2i(
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
				if ( auto pText = std::get_if<SpanBlock>( &span.block ) ) {
					Vector2f p = pText->text->findCharacterPos( index - span.startCharIndex );
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

SmallVector<Rectf> RichText::getSelectionRects() const {
	const_cast<RichText*>( this )->updateLayout();
	SmallVector<Rectf> rects;
	if ( mSelection.start == mSelection.end )
		return rects;

	Int64 start = std::min( mSelection.start, mSelection.end );
	Int64 end = std::max( mSelection.start, mSelection.end );

	for ( const auto& line : mLines ) {
		for ( const auto& span : line.spans ) {
			Int64 spanStart = std::max( start, span.startCharIndex );
			Int64 spanEnd = std::min( end, span.endCharIndex );

			if ( spanStart < spanEnd ) {
				if ( auto pText = std::get_if<SpanBlock>( &span.block ) ) {
					auto spanRects = pText->text->getSelectionRects(
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
				if ( auto pText = std::get_if<SpanBlock>( &span.block ) ) {
					res += pText->text->getString().substr( spanStart - span.startCharIndex,
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

void RichText::addSpan( const String& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding ) {
	if ( text.empty() && margin == Rectf::Zero && padding == Rectf::Zero )
		return;

	auto span = std::make_shared<Text>();
	span->setString( text );
	span->setStyleConfig( style );
	mBlocks.push_back( SpanBlock{ span, margin, padding } );
	invalidateLayout();
}

void RichText::addDrawable( std::shared_ptr<Drawable> drawable ) {
	if ( !drawable )
		return;
	mBlocks.push_back( drawable );
	invalidateLayout();
}

void RichText::addCustomSize( const Sizef& size, bool isBlock, UI::CSSFloat floatType,
							  UI::CSSClear clearType ) {
	mBlocks.push_back( CustomBlock{ size, isBlock, floatType, clearType } );
	invalidateLayout();
}

void RichText::addSpan( const String& text, const FontStyleConfig& style ) {
	addSpan( text, style, Rectf::Zero, Rectf::Zero );
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
	invalidateLayout();
}

void RichText::setFontStyleConfig( const FontStyleConfig& styleConfig ) {
	mDefaultStyle = styleConfig;
	invalidateLayout();
}

void RichText::setAlign( Uint32 align ) {
	if ( mAlign != align ) {
		mAlign = align;
		invalidateLayout();
	}
}

void RichText::setMaxWidth( Float width ) {
	if ( mMaxWidth != width ) {
		mMaxWidth = width;
		invalidateLayout();
	}
}

void RichText::invalidate() {
	invalidateLayout();
	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<SpanBlock>( &block ) ) {
			if ( pText->text )
				pText->text->invalidate();
		}
	}
}

Float RichText::getMinIntrinsicWidth() {
	Float minW = 0;
	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<SpanBlock>( &block ) ) {
			auto& span = pText->text;
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
															   span->getFontStyleConfig() ) +
											   pText->margin.Left + pText->margin.Right +
											   pText->padding.Left + pText->padding.Right );
				}
				start = end;
			}
		} else if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
			minW = std::max( minW, ( *pDrawable )->getPixelsSize().getWidth() );
		} else if ( auto pSize = std::get_if<CustomBlock>( &block ) ) {
			minW = std::max( minW, pSize->size.getWidth() );
		}
	}
	return minW;
}

Float RichText::getMaxIntrinsicWidth() {
	Float maxW = 0;
	Float curX = 0;
	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<SpanBlock>( &block ) ) {
			auto& span = pText->text;
			if ( !span || span->getString().empty() )
				continue;

			const String& s = span->getString();
			size_t start = 0;
			size_t end = 0;
			curX += pText->margin.Left + pText->padding.Left;
			while ( ( end = s.find( '\n', start ) ) != String::InvalidPos ) {
				curX += Text::getTextWidth( s.substr( start, end - start ),
											span->getFontStyleConfig(), 4, span->getTextHints() );
				maxW = std::max( maxW, curX + pText->margin.Right + pText->padding.Right );
				curX = 0;
				start = end + 1;
			}
			curX += Text::getTextWidth( s.substr( start ), span->getFontStyleConfig(), 4,
										span->getTextHints() ) +
					pText->margin.Right + pText->padding.Right;
		} else if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
			curX += ( *pDrawable )->getPixelsSize().getWidth();
		} else if ( auto pSize = std::get_if<CustomBlock>( &block ) ) {
			if ( pSize->isBlock ) {
				if ( curX > 0 ) {
					maxW = std::max( maxW, curX );
					curX = 0;
				}
				maxW = std::max( maxW, pSize->size.getWidth() );
			} else {
				curX += pSize->size.getWidth();
			}
		}
	}
	maxW = std::max( maxW, curX );
	return maxW;
}

void RichText::updateLayout() {
	if ( !mNeedsLayoutUpdate )
		return;

	// Detect whether any block has float/clear — if not, use the original
	// non-float layout path which is simpler and faster.
	bool hasFloats = false;
	for ( auto& block : mBlocks ) {
		if ( auto pSize = std::get_if<CustomBlock>( &block ) ) {
			if ( pSize->floatType != UI::CSSFloat::None ||
				 pSize->clearType != UI::CSSClear::None ) {
				hasFloats = true;
				break;
			}
		}
	}

	// ─── Fast path: no floats or clears ─────────────────────────────
	if ( !hasFloats ) {
		mLines.clear();
		mLines.push_back( RenderParagraph() );

		Float curX = 0;
		Float maxWidth = 0;
		Int64 curCharIdx = 0;

		// Pass 1: flow blocks into lines, wrapping at mMaxWidth.
		for ( auto& block : mBlocks ) {
			if ( auto pText = std::get_if<SpanBlock>( &block ) ) {
				auto& span = pText->text;
				if ( !span )
					continue;

				// Empty-string spans contribute only their margin/padding.
				if ( span->getString().empty() ) {
					Float l = pText->margin.Left + pText->padding.Left;
					Float r = pText->margin.Right + pText->padding.Right;
					if ( l <= 0 && r <= 0 )
						continue;
					curX += l + r;
					if ( !mLines.empty() )
						mLines.back().width += l + r;
					continue;
				}

				auto& fontStyle = span->getFontStyleConfig();
				if ( !fontStyle.Font )
					continue;

				Float extraLeft = pText->margin.Left + pText->padding.Left;
				curX += extraLeft;
				if ( !mLines.empty() )
					mLines.back().width += extraLeft;

				Uint32 textHints = span->getTextHints();

				// Compute where lines break within this text span.
				LineWrapInfoEx wrapInfo = LineWrap::computeLineBreaksEx(
					span->getString(), fontStyle, mMaxWidth > 0 ? mMaxWidth : 1e9f,
					mMaxWidth > 0 ? LineWrapMode::Word : LineWrapMode::NoWrap, false, 4, 0.f,
					textHints, false, curX );

				if ( wrapInfo.wraps.empty() ||
					 wrapInfo.wraps.back() != (Float)span->getString().size() )
					wrapInfo.wraps.push_back( span->getString().size() );

				// Emit a RenderSpan for each segment, wrapping to new lines as needed.
				for ( size_t i = 0; i < wrapInfo.wraps.size() - 1; ++i ) {
					size_t startIdx = wrapInfo.wraps[i];
					size_t endIdx = wrapInfo.wraps[i + 1];
					bool isNewline =
						( endIdx - startIdx == 1 && span->getString()[startIdx] == '\n' );

					if ( !isNewline ) {
						std::shared_ptr<Text> renderSpanText = std::make_shared<Text>();
						renderSpanText->setString(
							span->getString().substr( startIdx, endIdx - startIdx ) );
						renderSpanText->setStyleConfig( fontStyle );

						Float ascent = fontStyle.Font->getAscent( fontStyle.CharacterSize );
						Float height = fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );
						Float spanWidth = renderSpanText->getTextWidth();

						RenderSpan renderSpan;
						renderSpan.block =
							SpanBlock{ renderSpanText, pText->margin, pText->padding };
						renderSpan.position = { curX, 0 };
						renderSpan.size = Sizef( spanWidth, height );
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

					// After the last segment, add trailing margin and check if the
					// margin itself forces a wrap.
					if ( i == wrapInfo.wraps.size() - 2 && !isNewline ) {
						Float extraRight = pText->margin.Right + pText->padding.Right;
						curX += extraRight;
						mLines.back().width += extraRight;
						if ( !isNewline && mMaxWidth > 0 && curX > mMaxWidth ) {
							maxWidth = std::max( maxWidth, curX );
							mLines.push_back( RenderParagraph() );
							curX = 0;
							continue;
						}
					}

					// Start a new line for hard breaks (newlines) or soft wraps.
					if ( i < wrapInfo.wraps.size() - 2 || isNewline ) {
						if ( isNewline ) {
							curCharIdx++;
							if ( i == wrapInfo.wraps.size() - 2 ) {
								Float extraRight = pText->margin.Right + pText->padding.Right;
								curX += extraRight;
								mLines.back().width += extraRight;
							}
						}
						maxWidth = std::max( maxWidth, curX );
						mLines.push_back( RenderParagraph() );
						curX = 0;
					}
				}
			} else {
				// Drawable or CustomBlock (non-float).
				Sizef blockSize;
				bool isBlock = false;
				if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
					auto& drawable = *pDrawable;
					blockSize = drawable ? drawable->getPixelsSize() : Sizef();
				} else if ( auto pSize = std::get_if<CustomBlock>( &block ) ) {
					blockSize = pSize->size;
					isBlock = pSize->isBlock;
				}

				// Block elements force a line break before themselves.
				if ( isBlock && curX > 0 ) {
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}

				// Inline elements that don't fit wrap to the next line.
				if ( mMaxWidth > 0 && !isBlock &&
					 ( curX + blockSize.getWidth() >= mMaxWidth || curX >= mMaxWidth ) &&
					 curX > 0 ) {
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

				// Block elements also force a line break after themselves.
				if ( ( mMaxWidth > 0 && curX >= mMaxWidth ) || isBlock ) {
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}
			}
		}

		maxWidth = std::max( maxWidth, curX );

		// Remove trailing empty line if present.
		if ( !mLines.empty() && mLines.back().spans.empty() && mLines.size() > 1 ) {
			mLines.pop_back();
		}

		// Pass 2: assign Y positions to each line, apply text alignment,
		// and compute vertical offsets for spans within their line.
		Float curY = 0;
		for ( auto& line : mLines ) {
			line.y = curY;

			// Compute horizontal alignment offset for this line.
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
				if ( auto pText = std::get_if<SpanBlock>( &span.block ) ) {
					auto& textBlock = pText->text;
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
		return;
	}

	// ─── Float-aware path ────────────────────────────────────────────

	mLines.clear();
	mLines.push_back( RenderParagraph() );

	Float curX = 0;
	Float maxWidth = 0;
	Int64 curCharIdx = 0;

	// Active float rectangles: { left, top, right, bottom } in local coords.
	std::vector<Rectf> leftFloats;
	std::vector<Rectf> rightFloats;
	Float curY = 0;

	// ── Helper lambdas ─────────────────────────────────────────────
	// Returns the rightmost x-coordinate occupied by left floats at the given y.
	auto floatLeftEdge = [&]( Float y ) -> Float {
		Float l = 0;
		for ( auto& f : leftFloats ) {
			if ( y >= f.Top && y < f.Bottom )
				l = std::max( l, f.Right );
		}
		return l;
	};

	// Returns the leftmost x-coordinate occupied by right floats at the given y.
	auto floatRightEdge = [&]( Float y ) -> Float {
		Float r = mMaxWidth > 0 ? mMaxWidth : 1e9f;
		for ( auto& f : rightFloats ) {
			if ( y >= f.Top && y < f.Bottom )
				r = std::min( r, f.Left );
		}
		return r;
	};

	// Available horizontal space at y, narrowed by active floats on both sides.
	auto effectiveMaxWidthAt = [&]( Float y ) -> Float {
		return floatRightEdge( y ) - floatLeftEdge( y );
	};

	// Advances curY past the bottom of active floats specified by clearType.
	// Returns true if curY was moved.
	auto clearFloats = [&]( UI::CSSClear clearType ) -> bool {
		bool advanced = false;
		if ( clearType == UI::CSSClear::Left || clearType == UI::CSSClear::Both ) {
			for ( auto& f : leftFloats ) {
				if ( f.Bottom > curY ) {
					curY = f.Bottom;
					advanced = true;
				}
			}
		}
		if ( clearType == UI::CSSClear::Right || clearType == UI::CSSClear::Both ) {
			for ( auto& f : rightFloats ) {
				if ( f.Bottom > curY ) {
					curY = f.Bottom;
					advanced = true;
				}
			}
		}
		return advanced;
	};

	// ── Pass 1: flow blocks with float awareness ────────────────────
	for ( auto& block : mBlocks ) {
		if ( auto pText = std::get_if<SpanBlock>( &block ) ) {
			// ── Text span ─────────────────────────────────────────
			auto& span = pText->text;
			if ( !span )
				continue;

			if ( span->getString().empty() ) {
				Float l = pText->margin.Left + pText->padding.Left;
				Float r = pText->margin.Right + pText->padding.Right;
				if ( l <= 0 && r <= 0 )
					continue;
				curX += l + r;
				if ( !mLines.empty() )
					mLines.back().width += l + r;
				continue;
			}

			auto& fontStyle = span->getFontStyleConfig();
			if ( !fontStyle.Font )
				continue;

			Float extraLeft = pText->margin.Left + pText->padding.Left;
			curX += extraLeft;
			if ( !mLines.empty() )
				mLines.back().width += extraLeft;

			// Shift curX inside to the left edge — text starts
			// to the right of any left floats.
			Float le = floatLeftEdge( curY );
			if ( curX < le )
				curX = le;

			// Narrow the available width by active floats at this Y.
			Uint32 textHints = span->getTextHints();
			Float effW = effectiveMaxWidthAt( curY );
			if ( mMaxWidth > 0 && mMaxWidth < effW )
				effW = mMaxWidth;

			LineWrapInfoEx wrapInfo =
				LineWrap::computeLineBreaksEx( span->getString(), fontStyle, effW > 0 ? effW : 1e9f,
											   effW > 0 ? LineWrapMode::Word : LineWrapMode::NoWrap,
											   false, 4, 0.f, textHints, false, curX );

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
					renderSpan.block = SpanBlock{ renderSpanText, pText->margin, pText->padding };
					renderSpan.position = { curX, 0 };
					renderSpan.size = Sizef( spanWidth, height );
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

				// Trailing margin may force a wrap.
				if ( i == wrapInfo.wraps.size() - 2 && !isNewline ) {
					Float extraRight = pText->margin.Right + pText->padding.Right;
					curX += extraRight;
					mLines.back().width += extraRight;
					if ( effW > 0 && effW < 1e9f && curX > effW ) {
						maxWidth = std::max( maxWidth, curX );
						mLines.push_back( RenderParagraph() );
						curX = 0;
						continue;
					}
				}

				// Newline or soft-wrap → start a new line.
				if ( i < wrapInfo.wraps.size() - 2 || isNewline ) {
					if ( isNewline ) {
						curCharIdx++;
						if ( i == wrapInfo.wraps.size() - 2 ) {
							Float extraRight = pText->margin.Right + pText->padding.Right;
							curX += extraRight;
							mLines.back().width += extraRight;
						}
					}
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}
			}
		} else {
			// ── Drawable or CustomBlock ────────────────────────────
			Sizef blockSize;
			bool isBlock = false;
			UI::CSSFloat floatType = UI::CSSFloat::None;
			UI::CSSClear clearType = UI::CSSClear::None;
			if ( auto pDrawable = std::get_if<std::shared_ptr<Drawable>>( &block ) ) {
				auto& drawable = *pDrawable;
				blockSize = drawable ? drawable->getPixelsSize() : Sizef();
			} else if ( auto pSize = std::get_if<CustomBlock>( &block ) ) {
				blockSize = pSize->size;
				isBlock = pSize->isBlock;
				floatType = pSize->floatType;
				clearType = pSize->clearType;
			}

			// ── Clear: advance curY past active floats ─────────────
			if ( clearType != UI::CSSClear::None ) {
				if ( clearFloats( clearType ) ) {
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}
			}

			// Left edge of open space at current Y (after any clears).
			Float le = floatLeftEdge( curY );

			if ( floatType != UI::CSSFloat::None ) {
				// ── Float placement ────────────────────────────────
				// Position the float at the left/right edge of the
				// available space. Floats do NOT consume inline-flow
				// horizontal space (curX is not advanced) and are not
				// affected by text-align (see pass 2).
				Float posX;
				if ( floatType == UI::CSSFloat::Left ) {
					posX = le;
				} else {
					Float re = floatRightEdge( curY );
					posX = re - blockSize.getWidth();
					if ( posX < le )
						posX = le;
				}

				RenderSpan renderSpan;
				renderSpan.block = block;
				renderSpan.position = { posX, 0 };
				renderSpan.size = blockSize;
				renderSpan.startCharIndex = curCharIdx;
				renderSpan.endCharIndex = curCharIdx + 1;
				curCharIdx = renderSpan.endCharIndex;

				mLines.back().spans.push_back( renderSpan );

				// Record the float's bounding box so subsequent
				// content can wrap around it.
				Rectf fr( posX, curY, posX + blockSize.getWidth(),
					curY + blockSize.getHeight() );
				if ( floatType == UI::CSSFloat::Left )
					leftFloats.push_back( fr );
				else
					rightFloats.push_back( fr );
			} else {
				// ── Normal (non-float) block ────────────────────
				if ( curX < le )
					curX = le;

				// Block elements force a line break before.
				if ( isBlock && curX > 0 ) {
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}

				// Wrap if the block doesn't fit in the available width
				// (narrowed by active floats).
				Float effW = effectiveMaxWidthAt( curY );
				if ( effW > 0 && effW < 1e9f && !isBlock &&
					 ( curX + blockSize.getWidth() >= effW || curX >= effW ) && curX > 0 ) {
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

				// Block elements or overflow force a line break after.
				if ( ( effW > 0 && effW < 1e9f && curX >= effW ) || isBlock ) {
					maxWidth = std::max( maxWidth, curX );
					mLines.push_back( RenderParagraph() );
					curX = 0;
				}
			}
		}
	}

	maxWidth = std::max( maxWidth, curX );

	if ( !mLines.empty() && mLines.back().spans.empty() && mLines.size() > 1 ) {
		mLines.pop_back();
	}

	// ── Pass 2: assign Y positions and apply text alignment ───────
	// NOTE: float spans are excluded from the xOffset because
	// text-align only affects inline-flow content, not floated elements.
	Float accumY = 0;
	for ( auto& line : mLines ) {
		line.y = accumY;

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
			bool isFloat = false;
			if ( auto pSize = std::get_if<CustomBlock>( &span.block ) ) {
				if ( pSize->floatType != UI::CSSFloat::None )
					isFloat = true;
			}
			if ( auto pText = std::get_if<SpanBlock>( &span.block ) ) {
				auto& textBlock = pText->text;
				Float offsetY = line.maxAscent - textBlock->getCharacterSize();
				span.position.x += xOffset;
				span.position.y = offsetY;
				maxLineHeight = std::max( maxLineHeight, offsetY + span.size.getHeight() );
			} else {
				Float offsetY = line.maxAscent - span.size.getHeight();
				if ( offsetY < 0 )
					offsetY = 0;
				// Float spans keep their edge-aligned x; only inline-flow spans shift.
				if ( !isFloat )
					span.position.x += xOffset;
				span.position.y = offsetY;
				maxLineHeight = std::max( maxLineHeight, offsetY + span.size.getHeight() );
			}
		}

		line.height = std::max( line.height, maxLineHeight );
		accumY += line.height;
	}

	mSize = Sizef( maxWidth, accumY );
	mTotalCharacterCount = curCharIdx;
	mNeedsLayoutUpdate = false;
}

Sizef RichText::getSize() {
	updateLayout();
	return mSize;
}

void RichText::invalidateLayout() {
	mNeedsLayoutUpdate = true;
}

}} // namespace EE::Graphics
