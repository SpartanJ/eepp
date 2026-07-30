#include <algorithm>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/linewrap.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/richtext.hpp>

namespace EE { namespace Graphics {

static Float getTextRunLineHeight( const std::shared_ptr<Text>& text, Float lineHeight ) {
	if ( !text || !text->getFontStyleConfig().Font )
		return lineHeight;
	const auto& fontStyle = text->getFontStyleConfig();
	return lineHeight > 0 ? lineHeight : fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );
}

static Float getTextVisualLineHeight( const std::shared_ptr<Text>& text, Float lineHeight ) {
	if ( !text || !text->getFontStyleConfig().Font )
		return lineHeight;
	const auto& fontStyle = text->getFontStyleConfig();
	return fontStyle.Font->getLineSpacing( fontStyle.CharacterSize );
}

static Float getTextVisualBaseline( const std::shared_ptr<Text>& text ) {
	if ( !text || !text->getFontStyleConfig().Font )
		return 0.f;
	const auto& fontStyle = text->getFontStyleConfig();
	return fontStyle.Font->getAscent( fontStyle.CharacterSize );
}

static Float getTextLineBoxBaselineForHeight( const std::shared_ptr<Text>& text,
											  Float usedLineHeight ) {
	if ( !text || !text->getFontStyleConfig().Font )
		return 0.f;
	Float fontLineSpacing = getTextVisualLineHeight( text, usedLineHeight );
	Float leading = eemax( 0.f, usedLineHeight - fontLineSpacing ) * 0.5f;
	return leading + getTextVisualBaseline( text );
}

static Float getTextLineBoxBaseline( const std::shared_ptr<Text>& text, Float lineHeight ) {
	return getTextLineBoxBaselineForHeight( text, getTextRunLineHeight( text, lineHeight ) );
}

static Float getFontXHeight( const FontStyleConfig& fontStyle ) {
	if ( fontStyle.Font ) {
		Glyph glyph = fontStyle.Font->getGlyph( 'x', fontStyle.CharacterSize, false, false );
		Float xHeight = eemax( 0.f, -glyph.bounds.Top );
		if ( xHeight > 0 )
			return xHeight;
	}
	return fontStyle.CharacterSize * 0.5f;
}

static Float getParentAscent( const FontStyleConfig& fontStyle ) {
	return fontStyle.Font ? fontStyle.Font->getAscent( fontStyle.CharacterSize )
						  : static_cast<Float>( fontStyle.CharacterSize );
}

static Float getParentDescent( const FontStyleConfig& fontStyle ) {
	if ( !fontStyle.Font )
		return 0.f;
	return eemax( 0.f, fontStyle.Font->getLineSpacing( fontStyle.CharacterSize ) -
						   fontStyle.Font->getAscent( fontStyle.CharacterSize ) );
}

static Float getBaselineAlignedOffset( const RichText::RenderParagraph& line, const Sizef& size,
									   Float naturalBaseline, Float ownLineHeight,
									   const RichText::BaselineAlignValue& align,
									   const FontStyleConfig& parentFontStyle ) {
	Float baseline = line.maxAscent;
	switch ( align.type ) {
		case RichText::BaselineAlignment::Sub:
			return baseline - naturalBaseline + parentFontStyle.CharacterSize * 0.2f;
		case RichText::BaselineAlignment::Super:
			return baseline - naturalBaseline - parentFontStyle.CharacterSize * 0.33f;
		case RichText::BaselineAlignment::TextTop:
			return baseline - getParentAscent( parentFontStyle );
		case RichText::BaselineAlignment::TextBottom:
			return baseline + getParentDescent( parentFontStyle ) - size.getHeight();
		case RichText::BaselineAlignment::Middle:
			return baseline + getFontXHeight( parentFontStyle ) * 0.5f - size.getHeight() * 0.5f;
		case RichText::BaselineAlignment::Top:
			return 0.f;
		case RichText::BaselineAlignment::Bottom:
			return line.height - size.getHeight();
		case RichText::BaselineAlignment::Length:
			return baseline - naturalBaseline - align.value;
		case RichText::BaselineAlignment::Percentage:
			return baseline - naturalBaseline - ownLineHeight * align.value / 100.f;
		case RichText::BaselineAlignment::Auto:
		case RichText::BaselineAlignment::Baseline:
		default:
			return baseline - naturalBaseline;
	}
}

RichText* RichText::New() {
	return eeNew( RichText, () );
}

RichText::RichText() : Drawable( Drawable::RICHTEXT ) {}

RichText::~RichText() {}

static void drawInlineFragmentBackgroundColor( const RichText::InlineFragment& fragment,
											   const Rectf& rect, Float rotation ) {
	if ( fragment.backgroundColor == Color::Transparent ||
		 fragment.backgroundDrawableUsesFragmentColor )
		return;

	if ( fragment.backgroundColorDrawable != nullptr ) {
		const Color oldColor = fragment.backgroundColorDrawable->getColor();
		fragment.backgroundColorDrawable->setColor( fragment.backgroundColor );
		fragment.backgroundColorDrawable->draw( rect.getPosition(), rect.getSize() );
		fragment.backgroundColorDrawable->setColor( oldColor );
		return;
	}

	Primitives p;
	p.setColor( fragment.backgroundColor );
	p.drawRectangle( rect, rotation, Vector2f::One );
}

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

	for ( const auto& fragment : mInlineFragments ) {
		if ( fragment.type != InlineFragment::Type::Box )
			continue;
		const Rectf& localPaintBounds =
			fragment.paintBounds != Rectf::Zero ? fragment.paintBounds : fragment.bounds;
		Rectf rect( localPaintBounds.getPosition() * scale + Vector2f( X, Y ),
					localPaintBounds.getSize() * scale );
		drawInlineFragmentBackgroundColor( fragment, rect, rotation );
		if ( fragment.backgroundDrawable != nullptr ) {
			if ( fragment.backgroundDrawableUsesFragmentColor &&
				 fragment.backgroundColor != Color::Transparent ) {
				const Color oldColor = fragment.backgroundDrawable->getColor();
				fragment.backgroundDrawable->setColor( fragment.backgroundColor );
				fragment.backgroundDrawable->draw( rect.getPosition(), rect.getSize() );
				fragment.backgroundDrawable->setColor( oldColor );
			} else {
				fragment.backgroundDrawable->draw( rect.getPosition(), rect.getSize() );
			}
		}
		if ( fragment.borderDrawable != nullptr ) {
			fragment.borderDrawable->draw( rect.getPosition(), rect.getSize() );
		} else if ( fragment.borderWidth > 0 && fragment.borderColor != Color::Transparent ) {
			Primitives p;
			p.setColor( fragment.borderColor );
			const Float bw = fragment.borderWidth * scale.x;
			p.drawRectangle( Rectf( rect.Left, rect.Top, rect.Right, rect.Top + bw ) );
			p.drawRectangle( Rectf( rect.Left, rect.Bottom - bw, rect.Right, rect.Bottom ) );
			if ( fragment.startsInlineBox )
				p.drawRectangle( Rectf( rect.Left, rect.Top, rect.Left + bw, rect.Bottom ) );
			if ( fragment.endsInlineBox )
				p.drawRectangle( Rectf( rect.Right - bw, rect.Top, rect.Right, rect.Bottom ) );
		}
	}

	for ( auto& line : mLines ) {
		for ( auto& span : line.spans ) {
			// span.position is local to the line (x) + baseline offset (y) line.Y is the line
			// vertical offset

			Vector2f pos = span.position;

			if ( span.type == RenderSpan::Type::Text && span.text ) {
				const std::shared_ptr<Text>& text = span.text;
				Color oldBgColor = text->getFontStyleConfig().BackgroundColor;

				if ( oldBgColor != Color::Transparent && !span.suppressBackground ) {
					Primitives p;
					p.setColor( oldBgColor );
					Rectf bgRect(
						Vector2f( std::trunc( X + pos.x - span.padding.Left ),
								  std::trunc( Y + line.y + pos.y - span.padding.Top ) ),
						Sizef( span.size.getWidth() + span.padding.Left + span.padding.Right,
							   span.size.getHeight() + span.padding.Top + span.padding.Bottom ) );
					p.drawRectangle( bgRect, rotation, scale );
				}

				if ( oldBgColor != Color::Transparent )
					text->setBackgroundColor( Color::Transparent );

				bool selectionApplied = false;
				if ( mSelectionColor != Color::Transparent ) {
					TextSelectionRange spanSel = {
						std::clamp( mSelection.start, span.startCharIndex, span.endCharIndex ) -
							span.startCharIndex,
						std::clamp( mSelection.end, span.startCharIndex, span.endCharIndex ) -
							span.startCharIndex };

					if ( spanSel.start != spanSel.end ) {
						text->setFillColor( mSelectionColor,
											(Uint32)std::min( spanSel.start, spanSel.end ),
											(Uint32)std::max( spanSel.start, spanSel.end ) - 1 );
						selectionApplied = true;
					}
				}

				if ( rotation == 0 && scale == Vector2f::One ) {
					text->draw( std::trunc( X + pos.x ), std::trunc( Y + line.y + pos.y ),
								Vector2f::One, 0, effect );
				} else {
					text->draw( std::trunc( X + pos.x * scale.x ),
								std::trunc( Y + ( line.y + pos.y ) * scale.y ), scale, rotation,
								effect, rotationCenter, scaleCenter );
				}

				if ( oldBgColor != Color::Transparent )
					text->setBackgroundColor( oldBgColor );

				if ( selectionApplied )
					text->invalidateColors();
			} else if ( span.type == RenderSpan::Type::Drawable && span.drawable ) {
				span.drawable->draw(
					Vector2f( std::trunc( X + pos.x ), std::trunc( Y + line.y + pos.y ) ),
					span.size );
			}
		}
	}
}

void RichText::setSelection( TextSelectionRange range ) {
	if ( mSelection.start != range.start || mSelection.end != range.end ) {
		mSelection = range;
	}
}

void RichText::setLineHeight( Float height ) {
	if ( mLineHeight != height ) {
		mLineHeight = height;
		invalidate();
	}
}

void RichText::setTextIndent( Float indent ) {
	if ( mTextIndent != indent ) {
		mTextIndent = indent;
		invalidate();
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
			const RenderSpan* previousSpan = nullptr;
			for ( const auto& span : line.spans ) {
				if ( pos.x < span.position.x ) {
					if ( previousSpan == nullptr )
						return span.startCharIndex;

					Float previousEnd = previousSpan->position.x + previousSpan->size.getWidth();
					Float gapMidpoint = previousEnd + ( span.position.x - previousEnd ) * 0.5f;
					return pos.x < gapMidpoint ? previousSpan->endCharIndex : span.startCharIndex;
				}

				if ( pos.x >= span.position.x && pos.x < span.position.x + span.size.getWidth() ) {
					if ( span.type == RenderSpan::Type::Text && span.text ) {
						return span.startCharIndex +
							   span.text->findCharacterFromPos( Vector2i(
								   pos.x - span.position.x, pos.y - line.y - span.position.y ) );
					} else {
						return ( pos.x < span.position.x + span.size.getWidth() * 0.5f )
								   ? span.startCharIndex
								   : span.endCharIndex;
					}
				}
				previousSpan = &span;
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
				if ( span.type == RenderSpan::Type::Text && span.text ) {
					Vector2f p = span.text->findCharacterPos( index - span.startCharIndex );
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

	if ( !mInlineFragments.empty() ) {
		for ( const auto& fragment : mInlineFragments ) {
			if ( fragment.type == InlineFragment::Type::Box )
				continue;

			Int64 fragmentStart = std::max( start, fragment.startCharIndex );
			Int64 fragmentEnd = std::min( end, fragment.endCharIndex );
			if ( fragmentStart >= fragmentEnd )
				continue;

			if ( fragment.type == InlineFragment::Type::TextRun && fragment.text ) {
				auto spanRects =
					fragment.text->getSelectionRects( { fragmentStart - fragment.startCharIndex,
														fragmentEnd - fragment.startCharIndex } );
				for ( auto& rect : spanRects ) {
					rect.move( fragment.bounds.getPosition() );
					rects.push_back( rect );
				}
			} else {
				rects.push_back( fragment.bounds );
			}
		}
		return rects;
	}

	for ( const auto& line : mLines ) {
		for ( const auto& span : line.spans ) {
			Int64 spanStart = std::max( start, span.startCharIndex );
			Int64 spanEnd = std::min( end, span.endCharIndex );

			if ( spanStart < spanEnd ) {
				if ( span.type == RenderSpan::Type::Text && span.text ) {
					auto spanRects = span.text->getSelectionRects(
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
				if ( span.type == RenderSpan::Type::Text && span.text ) {
					res += span.text->getString().substr( spanStart - span.startCharIndex,
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

namespace {

// Navigate the inline tree along a path of indices and return the target vector.
static std::vector<RichText::InlineItem>*
resolveInlinePath( std::vector<RichText::InlineItem>& root,
				   const RichText::RenderSpan::InlinePath& path ) {
	std::vector<RichText::InlineItem>* target = &root;
	for ( size_t idx : path ) {
		if ( idx < target->size() && ( *target )[idx].isBox() )
			target = &( *target )[idx].asBox().children;
		else
			break;
	}
	return target;
}

static void invalidateInlineItemText( std::vector<RichText::InlineItem>& items ) {
	for ( auto& item : items ) {
		if ( item.isTextRun() ) {
			if ( item.asTextRun().text )
				item.asTextRun().text->invalidate();
		} else if ( item.isBox() ) {
			invalidateInlineItemText( item.asBox().children );
		}
	}
}

static void setInlineItemTextTabWidth( std::vector<RichText::InlineItem>& items, Uint32 tabWidth ) {
	for ( auto& item : items ) {
		if ( item.isTextRun() ) {
			if ( item.asTextRun().text )
				item.asTextRun().text->setTabWidth( tabWidth );
		} else if ( item.isBox() ) {
			setInlineItemTextTabWidth( item.asBox().children, tabWidth );
		}
	}
}

struct InlineAncestorRef {
	RichText::RenderSpan::InlinePath path;
	const RichText::InlineItem::Box* box{ nullptr };
};

struct InlineLeafRef {
	RichText::InlineFragment::Type type{ RichText::InlineFragment::Type::TextRun };
	RichText::RenderSpan::InlinePath path;
	SmallVector<InlineAncestorRef, 4> ancestors;
	std::shared_ptr<Text> text;
	RichText::InlineSource source;
	bool isLineBreak{ false };
	RichText::BaselineAlignValue baselineAlign;
};

struct InlineBoxFragmentAccumulator {
	RichText::RenderSpan::InlinePath path;
	size_t lineIndex{ 0 };
	Rectf bounds;
	Rectf contentBounds;
	bool valid{ false };
	bool contentValid{ false };
	const RichText::InlineItem::Box* box{ nullptr };
	size_t firstLeafIndex{ 0 };
	size_t lastLeafIndex{ 0 };
	bool startsInlineBox{ false };
	bool endsInlineBox{ false };
};

struct InlineVerticalEdges {
	Float top{ 0.f };
	Float bottom{ 0.f };
};

struct InlineLayoutRun {
	RichText::RenderSpan payload;
};

static bool sameInlinePath( const RichText::RenderSpan::InlinePath& lhs,
							const RichText::RenderSpan::InlinePath& rhs ) {
	return lhs == rhs;
}

static void expandInlineBounds( Rectf& target, bool& valid, const Rectf& rect ) {
	if ( !valid ) {
		target = rect;
		valid = true;
	} else {
		target.expand( rect );
	}
}

static Rectf expandInlineBoundsForLineHeight( const Rectf& bounds,
											  const RichText::InlineItem::Box& box ) {
	if ( !box.participatesInLineMetrics || box.lineHeight <= bounds.getHeight() )
		return bounds;

	Rectf out = bounds;
	Float extra = box.lineHeight - bounds.getHeight();
	Float top = extra * 0.5f;
	out.Top -= top;
	out.Bottom += extra - top;
	return out;
}

static bool isDefaultBaselineAlign( const RichText::BaselineAlignValue& align ) {
	return align.type == RichText::BaselineAlignment::Baseline ||
		   align.type == RichText::BaselineAlignment::Auto;
}

static const RichText::InlineItem::Box*
resolveInlineBox( const std::vector<RichText::InlineItem>& root,
				  const RichText::RenderSpan::InlinePath& path, size_t pathSize ) {
	const std::vector<RichText::InlineItem>* target = &root;
	const RichText::InlineItem::Box* box = nullptr;
	for ( size_t i = 0; i < pathSize; ++i ) {
		size_t idx = path[i];
		if ( idx >= target->size() || !( *target )[idx].isBox() )
			return nullptr;
		box = &( *target )[idx].asBox();
		target = &box->children;
	}
	return box;
}

static RichText::BaselineAlignValue
effectiveInlineBaselineAlign( const std::vector<RichText::InlineItem>& root,
							  const RichText::RenderSpan::InlinePath& inlinePath,
							  const RichText::BaselineAlignValue& requested ) {
	if ( !isDefaultBaselineAlign( requested ) )
		return requested;

	for ( size_t pathSize = inlinePath.size(); pathSize > 0; --pathSize ) {
		const auto* box = resolveInlineBox( root, inlinePath, pathSize );
		if ( box != nullptr && !isDefaultBaselineAlign( box->baselineAlign ) )
			return box->baselineAlign;
	}

	return requested;
}

static InlineVerticalEdges
inlineAncestorLineHeightEdges( const std::vector<RichText::InlineItem>& inlineItems,
							   const RichText::RenderSpan::InlinePath& inlinePath,
							   Float leafHeight ) {
	InlineVerticalEdges edges;
	if ( inlinePath.empty() )
		return edges;

	Float usedLineHeight = leafHeight;
	for ( size_t pathSize = 1; pathSize < inlinePath.size(); ++pathSize ) {
		const auto* box = resolveInlineBox( inlineItems, inlinePath, pathSize );
		if ( box == nullptr )
			continue;
		if ( box->participatesInLineMetrics && box->lineHeight > usedLineHeight )
			usedLineHeight = box->lineHeight;
	}

	Float extra = eemax( 0.f, usedLineHeight - leafHeight );
	edges.top = extra * 0.5f;
	edges.bottom = extra - edges.top;
	return edges;
}

static bool findFirstInlineLeafPath( const std::vector<RichText::InlineItem>& items,
									 RichText::RenderSpan::InlinePath& path,
									 RichText::RenderSpan::InlinePath& leafPath ) {
	for ( size_t i = 0; i < items.size(); ++i ) {
		path.push_back( i );
		const auto& item = items[i];
		if ( item.isBox() ) {
			if ( findFirstInlineLeafPath( item.asBox().children, path, leafPath ) )
				return true;
		} else if ( item.isTextRun() ||
					( item.isAtomicBox() && !item.asAtomicBox().isLineBreak ) ) {
			leafPath = path;
			return true;
		}
		path.pop_back();
	}
	return false;
}

static bool findLastInlineLeafPath( const std::vector<RichText::InlineItem>& items,
									RichText::RenderSpan::InlinePath& path,
									RichText::RenderSpan::InlinePath& leafPath ) {
	for ( size_t i = items.size(); i > 0; --i ) {
		path.push_back( i - 1 );
		const auto& item = items[i - 1];
		if ( item.isBox() ) {
			if ( findLastInlineLeafPath( item.asBox().children, path, leafPath ) )
				return true;
		} else if ( item.isTextRun() ||
					( item.isAtomicBox() && !item.asAtomicBox().isLineBreak ) ) {
			leafPath = path;
			return true;
		}
		path.pop_back();
	}
	return false;
}

static bool isFirstInlineLeafInBox( const RichText::InlineItem::Box& box,
									const RichText::RenderSpan::InlinePath& boxPath,
									const RichText::RenderSpan::InlinePath& leafPath ) {
	RichText::RenderSpan::InlinePath path = boxPath;
	RichText::RenderSpan::InlinePath firstLeafPath;
	return findFirstInlineLeafPath( box.children, path, firstLeafPath ) &&
		   sameInlinePath( firstLeafPath, leafPath );
}

static bool isLastInlineLeafInBox( const RichText::InlineItem::Box& box,
								   const RichText::RenderSpan::InlinePath& boxPath,
								   const RichText::RenderSpan::InlinePath& leafPath ) {
	RichText::RenderSpan::InlinePath path = boxPath;
	RichText::RenderSpan::InlinePath lastLeafPath;
	return findLastInlineLeafPath( box.children, path, lastLeafPath ) &&
		   sameInlinePath( lastLeafPath, leafPath );
}

static Float inlineAncestorStartSpacing( const std::vector<RichText::InlineItem>& inlineItems,
										 const RichText::RenderSpan::InlinePath& inlinePath ) {
	Float spacing = 0.f;
	for ( size_t pathSize = 1; pathSize < inlinePath.size(); ++pathSize ) {
		const auto* box = resolveInlineBox( inlineItems, inlinePath, pathSize );
		if ( box == nullptr || !box->contributesInlineSpacing )
			continue;

		RichText::RenderSpan::InlinePath boxPath( inlinePath.begin(),
												  inlinePath.begin() + pathSize );
		if ( isFirstInlineLeafInBox( *box, boxPath, inlinePath ) )
			spacing += box->margin.Left + box->padding.Left + box->borderWidth;
	}
	return spacing;
}

static Float inlineAncestorEndSpacing( const std::vector<RichText::InlineItem>& inlineItems,
									   const RichText::RenderSpan::InlinePath& inlinePath ) {
	Float spacing = 0.f;
	for ( size_t pathSize = inlinePath.size(); pathSize > 0; --pathSize ) {
		const auto* box = resolveInlineBox( inlineItems, inlinePath, pathSize );
		if ( box == nullptr || !box->contributesInlineSpacing )
			continue;

		RichText::RenderSpan::InlinePath boxPath( inlinePath.begin(),
												  inlinePath.begin() + pathSize );
		if ( isLastInlineLeafInBox( *box, boxPath, inlinePath ) )
			spacing += box->margin.Right + box->padding.Right + box->borderWidth;
	}
	return spacing;
}

static Uint32 inlineAncestorTextDecoration( const std::vector<RichText::InlineItem>& inlineItems,
											const RichText::RenderSpan::InlinePath& inlinePath ) {
	Uint32 textDecoration = 0;
	for ( size_t pathSize = 1; pathSize < inlinePath.size(); ++pathSize ) {
		const auto* box = resolveInlineBox( inlineItems, inlinePath, pathSize );
		if ( box != nullptr )
			textDecoration |= box->textDecoration;
	}
	return textDecoration;
}

class RichTextInlineLayouter {
  public:
	struct LayoutResult {
		std::vector<RichText::RenderParagraph> lines;
		Sizef size;
		Int64 totalCharacterCount{ 0 };
	};

	static bool needsFloatAwareLayout( const std::vector<RichText::InlineItem>& inlineItems ) {
		for ( const auto& run : buildLayoutRuns( inlineItems ) ) {
			const auto& payload = run.payload;
			if ( payload.type != RichText::RenderSpan::Type::Text ) {
				if ( payload.floatType != RichText::InlineFloat::None || payload.propagatedFloats ||
					 payload.clearType != RichText::InlineClear::None )
					return true;
			}
		}
		return false;
	}

	static Float getMinIntrinsicWidth( const std::vector<RichText::InlineItem>& inlineItems,
									   RichText::WhiteSpaceWrapMode whiteSpaceWrapMode ) {
		Float minW = 0;
		for ( const auto& run : buildLayoutRuns( inlineItems ) ) {
			const auto& payload = run.payload;
			if ( payload.type == RichText::RenderSpan::Type::Text ) {
				auto& span = payload.text;
				if ( !span || span->getString().empty() )
					continue;
				const String& s = span->getString();
				Float spacing = payload.margin.Left + payload.margin.Right + payload.padding.Left +
								payload.padding.Right +
								inlineAncestorStartSpacing( inlineItems, payload.inlinePath ) +
								inlineAncestorEndSpacing( inlineItems, payload.inlinePath );
				if ( whiteSpaceWrapMode == RichText::WhiteSpaceWrapMode::BreakSpaces ) {
					for ( size_t i = 0; i < s.size(); ++i ) {
						if ( s[i] == '\n' || s[i] == '\r' )
							continue;
						minW = std::max(
							minW, Text::getTextWidth( s.substr( i, 1 ), span->getFontStyleConfig(),
													  span->getTabWidth(), span->getTextHints() ) +
									  spacing );
					}
				} else {
					size_t start = 0;
					size_t end = 0;
					while ( start < s.size() ) {
						while ( start < s.size() && ( s[start] == ' ' || s[start] == '\t' ||
													  s[start] == '\n' || s[start] == '\r' ) )
							start++;
						end = start;
						while ( end < s.size() && !( s[end] == ' ' || s[end] == '\t' ||
													 s[end] == '\n' || s[end] == '\r' ) )
							end++;
						if ( start < end ) {
							minW = std::max( minW, Text::getTextWidth(
													   s.substr( start, end - start ),
													   span->getFontStyleConfig(),
													   span->getTabWidth(), span->getTextHints() ) +
													   spacing );
						}
						start = end;
					}
				}
			} else if ( payload.type == RichText::RenderSpan::Type::Drawable ) {
				if ( payload.drawable )
					minW = std::max( minW, payload.drawable->getPixelsSize().getWidth() );
			} else {
				minW = std::max( minW,
								 payload.size.getWidth() +
									 inlineAncestorStartSpacing( inlineItems, payload.inlinePath ) +
									 inlineAncestorEndSpacing( inlineItems, payload.inlinePath ) );
			}
		}
		return minW;
	}

	static Float getMaxIntrinsicWidth( const std::vector<RichText::InlineItem>& inlineItems ) {
		Float maxW = 0;
		Float curX = 0;
		for ( const auto& run : buildLayoutRuns( inlineItems ) ) {
			const auto& payload = run.payload;
			if ( payload.type == RichText::RenderSpan::Type::Text ) {
				auto& span = payload.text;
				if ( !span || span->getString().empty() )
					continue;

				const String& s = span->getString();
				size_t start = 0;
				size_t end = 0;
				curX += payload.margin.Left + payload.padding.Left +
						inlineAncestorStartSpacing( inlineItems, payload.inlinePath );
				while ( ( end = s.find( '\n', start ) ) != String::InvalidPos ) {
					curX += Text::getTextWidth( s.substr( start, end - start ),
												span->getFontStyleConfig(), span->getTabWidth(),
												span->getTextHints() );
					maxW = std::max(
						maxW, curX + payload.margin.Right + payload.padding.Right +
								  inlineAncestorEndSpacing( inlineItems, payload.inlinePath ) );
					curX = 0;
					start = end + 1;
				}
				curX += Text::getTextWidth( s.substr( start ), span->getFontStyleConfig(),
											span->getTabWidth(), span->getTextHints() ) +
						payload.margin.Right + payload.padding.Right +
						inlineAncestorEndSpacing( inlineItems, payload.inlinePath );
			} else if ( payload.type == RichText::RenderSpan::Type::Drawable ) {
				if ( payload.drawable )
					curX += payload.drawable->getPixelsSize().getWidth();
			} else if ( !payload.isLineBreak ) {
				curX += inlineAncestorStartSpacing( inlineItems, payload.inlinePath ) +
						payload.size.getWidth() +
						inlineAncestorEndSpacing( inlineItems, payload.inlinePath );
			}
		}
		maxW = std::max( maxW, curX );
		return maxW;
	}

	static void alignLineSpans( RichText::RenderParagraph& line, Float xOffset,
								const FontStyleConfig& defaultStyle, Float forcedLineHeight,
								bool preserveFloatPositions,
								const std::vector<RichText::InlineItem>& inlineItems ) {
		recomputeLineMetrics( line, forcedLineHeight, preserveFloatPositions, inlineItems );

		Float maxLineHeight = 0;
		Float minLineTop = 0;
		for ( auto& span : line.spans ) {
			bool isFloat = span.floatType != RichText::InlineFloat::None;

			if ( span.type == RichText::RenderSpan::Type::Text ) {
				Float baseline = getTextVisualBaseline( span.text );
				RichText::BaselineAlignValue baselineAlign = effectiveInlineBaselineAlign(
					inlineItems, span.inlinePath, span.baselineAlign );
				Float offsetY = getBaselineAlignedOffset(
					line, span.size, baseline, getTextRunLineHeight( span.text, span.lineHeight ),
					baselineAlign, defaultStyle );
				span.position.x += xOffset;
				span.position.y = offsetY;
				minLineTop = std::min( minLineTop, offsetY );
				maxLineHeight = std::max( maxLineHeight, offsetY + span.size.getHeight() );
			} else {
				Float baseline = span.baseline;
				RichText::BaselineAlignValue baselineAlign = effectiveInlineBaselineAlign(
					inlineItems, span.inlinePath, span.baselineAlign );
				Float offsetY = baselineAlign.type == RichText::BaselineAlignment::Middle &&
										baseline > 0.f && baseline < span.size.getHeight()
									? line.maxAscent - baseline
									: getBaselineAlignedOffset( line, span.size, baseline,
																span.size.getHeight(),
																baselineAlign, defaultStyle );
				if ( preserveFloatPositions && isFloat ) {
					span.position.y = 0;
					continue;
				}
				span.position.x += xOffset;
				span.position.y = offsetY;
				minLineTop = std::min( minLineTop, offsetY );
				maxLineHeight = std::max( maxLineHeight, offsetY + span.size.getHeight() );
			}
		}

		if ( minLineTop < 0 ) {
			for ( auto& span : line.spans ) {
				bool isFloat = span.floatType != RichText::InlineFloat::None;
				if ( !preserveFloatPositions || !isFloat )
					span.position.y -= minLineTop;
			}
			maxLineHeight -= minLineTop;
		}

		line.height = std::max( line.height, maxLineHeight );
		if ( forcedLineHeight > 0 )
			line.height = std::max( line.height, forcedLineHeight );
	}

	static LayoutResult layoutNoFloats( const std::vector<RichText::InlineItem>& inlineItems,
										Float maxLayoutWidth, Float textIndent, Uint32 align,
										Float forcedLineHeight, const FontStyleConfig& defaultStyle,
										bool lineWrap,
										RichText::WhiteSpaceWrapMode whiteSpaceWrapMode ) {
		LayoutResult result;
		result.lines.push_back( RichText::RenderParagraph() );

		SmallVector<InlineLayoutRun, 32> runs = buildLayoutRuns( inlineItems );
		Float curX = textIndent;
		Float maxWidth = 0;
		Int64 curCharIdx = 0;

		for ( const auto& run : runs ) {
			const auto& payload = run.payload;
			if ( payload.type == RichText::RenderSpan::Type::Text ) {
				auto& span = payload.text;
				if ( !span )
					continue;

				if ( span->getString().empty() ) {
					Float startSpacing = inlineStartSpacing( payload, inlineItems );
					Float endSpacing = inlineEndSpacing( payload, inlineItems );
					if ( startSpacing <= 0 && endSpacing <= 0 )
						continue;
					addInlineSpacingToCurrentLine( result, curX, startSpacing + endSpacing );
					continue;
				}

				auto& fontStyle = span->getFontStyleConfig();
				if ( !fontStyle.Font )
					continue;

				addInlineSpacingToCurrentLine( result, curX,
											   inlineStartSpacing( payload, inlineItems ) );

				LineWrapInfoEx wrapInfo =
					computeTextWraps( payload, fontStyle, maxLayoutWidth,
									  lineWrap && maxLayoutWidth > 0, curX, whiteSpaceWrapMode );

				for ( size_t i = 0; i < wrapInfo.wraps.size() - 1; ++i ) {
					size_t startIdx = wrapInfo.wraps[i];
					size_t endIdx = wrapInfo.wraps[i + 1];
					bool startsWithNewline =
						startIdx < endIdx && span->getString()[startIdx] == '\n';
					bool isNewline = startsWithNewline && endIdx - startIdx == 1;
					if ( startsWithNewline && !isNewline ) {
						curCharIdx++;
						startIdx++;
					}

					if ( !isNewline ) {
						appendTextRenderSpan( result.lines.back(), payload, fontStyle, startIdx,
											  endIdx, curX, curCharIdx, inlineItems );
					}

					if ( i == wrapInfo.wraps.size() - 2 && !isNewline ) {
						addInlineSpacingToCurrentLine( result, curX,
													   inlineEndSpacing( payload, inlineItems ) );
						if ( lineWrap && maxLayoutWidth > 0 && curX > maxLayoutWidth ) {
							maxWidth = std::max( maxWidth, curX );
							result.lines.push_back( RichText::RenderParagraph() );
							curX = 0;
							continue;
						}
					}

					bool isLastTextSegment = i + 2 == wrapInfo.wraps.size();
					bool trailingNewlineAlreadyAdvanced =
						isNewline && isLastTextSegment && result.lines.back().spans.empty() &&
						i > 0 && wrapInfo.wraps[i - 1] != wrapInfo.wraps[i];
					if ( i < wrapInfo.wraps.size() - 2 || isNewline ) {
						if ( isNewline ) {
							curCharIdx++;
							if ( isLastTextSegment ) {
								addInlineSpacingToCurrentLine(
									result, curX, inlineEndSpacing( payload, inlineItems ) );
							}
						}
						if ( !trailingNewlineAlreadyAdvanced ) {
							maxWidth = std::max( maxWidth, curX );
							result.lines.push_back( RichText::RenderParagraph() );
							curX = 0;
						}
					}
				}
			} else {
				AtomicBlockMetrics metrics = getAtomicBlockMetrics( payload );

				if ( metrics.isLineBreak ) {
					maxWidth = std::max( maxWidth, curX );
					if ( !result.lines.back().spans.empty() )
						result.lines.push_back( RichText::RenderParagraph() );
					curX = 0;
					continue;
				}

				Float startSpacing = 0.f;
				Float endSpacing = 0.f;
				if ( payload.type == RichText::RenderSpan::Type::AtomicBox ) {
					startSpacing = inlineStartSpacing( payload, inlineItems );
					endSpacing = inlineEndSpacing( payload, inlineItems );
				}
				bool hadLineContentBeforeSpacing = !result.lines.back().spans.empty();
				addInlineSpacingToCurrentLine( result, curX, startSpacing );

				if ( lineWrap && maxLayoutWidth > 0 &&
					 ( curX + metrics.size.getWidth() >= maxLayoutWidth ||
					   curX >= maxLayoutWidth ) &&
					 curX > 0 && hadLineContentBeforeSpacing ) {
					maxWidth = std::max( maxWidth, curX );
					result.lines.push_back( RichText::RenderParagraph() );
					curX = 0;
					addInlineSpacingToCurrentLine( result, curX, startSpacing );
				}

				appendAtomicRenderSpan( result.lines.back(), payload, metrics, curX, curCharIdx );
				addInlineSpacingToCurrentLine( result, curX, endSpacing );

				if ( lineWrap && maxLayoutWidth > 0 && curX >= maxLayoutWidth ) {
					maxWidth = std::max( maxWidth, curX );
					result.lines.push_back( RichText::RenderParagraph() );
					curX = 0;
				}
			}
		}

		maxWidth = std::max( maxWidth, curX );

		if ( !result.lines.empty() && result.lines.back().spans.empty() &&
			 result.lines.size() > 1 ) {
			result.lines.pop_back();
		}

		Float curY = 0;
		for ( auto& line : result.lines ) {
			line.y = curY;
			Float xOffset = horizontalAlignmentOffset( line, maxLayoutWidth, align );
			alignLineSpans( line, xOffset, defaultStyle, forcedLineHeight, false, inlineItems );
			curY += line.height;
		}

		result.size = Sizef( maxWidth, curY );
		result.totalCharacterCount = curCharIdx;
		return result;
	}

	static LayoutResult
	layoutWithFloats( const std::vector<RichText::InlineItem>& inlineItems, Float maxLayoutWidth,
					  Float textIndent, Uint32 align, Float forcedLineHeight,
					  const FontStyleConfig& defaultStyle,
					  const std::vector<RichText::FloatExclusion>& externalFloatExclusions,
					  std::vector<RichText::FloatExclusion>& localFloatExclusions,
					  bool lineWrap, RichText::WhiteSpaceWrapMode whiteSpaceWrapMode ) {
		LayoutResult result;
		result.lines.push_back( RichText::RenderParagraph() );

		Float curX = textIndent;
		Float maxWidth = 0;
		Int64 curCharIdx = 0;
		SmallVector<Rectf, 4> leftFloats;
		SmallVector<Rectf, 4> rightFloats;
		Float curY = 0;
		SmallVector<InlineLayoutRun, 32> runs = buildLayoutRuns( inlineItems );

		for ( const auto& exclusion : externalFloatExclusions ) {
			if ( exclusion.type == RichText::InlineFloat::Left )
				leftFloats.push_back( exclusion.rect );
			else if ( exclusion.type == RichText::InlineFloat::Right )
				rightFloats.push_back( exclusion.rect );
		}
		size_t externalLeftFloatCount = leftFloats.size();
		size_t externalRightFloatCount = rightFloats.size();

		auto floatLeftEdge = [&]( Float y ) -> Float {
			Float l = 0;
			for ( auto& f : leftFloats ) {
				if ( y >= f.Top && y < f.Bottom )
					l = std::max( l, f.Right );
			}
			return l;
		};

		auto floatRightEdge = [&]( Float y ) -> Float {
			Float r = maxLayoutWidth > 0 ? maxLayoutWidth : 1e9f;
			for ( auto& f : rightFloats ) {
				if ( y >= f.Top && y < f.Bottom )
					r = std::min( r, f.Left );
			}
			return r;
		};

		auto effectiveMaxWidthAt = [&]( Float y ) -> Float {
			return floatRightEdge( y ) - floatLeftEdge( y );
		};

		auto activeFloatBottom = [&]( Float y ) -> Float {
			Float bottom = y;
			for ( const auto& f : leftFloats ) {
				if ( y >= f.Top && y < f.Bottom )
					bottom = std::max( bottom, f.Bottom );
			}
			for ( const auto& f : rightFloats ) {
				if ( y >= f.Top && y < f.Bottom )
					bottom = std::max( bottom, f.Bottom );
			}
			return bottom;
		};

		auto clearFloats = [&]( RichText::InlineClear clearType ) -> bool {
			bool advanced = false;
			if ( clearType == RichText::InlineClear::Left ||
				 clearType == RichText::InlineClear::Both ) {
				for ( size_t i = externalLeftFloatCount; i < leftFloats.size(); ++i ) {
					auto& f = leftFloats[i];
					if ( f.Bottom > curY ) {
						curY = f.Bottom;
						advanced = true;
					}
				}
			}
			if ( clearType == RichText::InlineClear::Right ||
				 clearType == RichText::InlineClear::Both ) {
				for ( size_t i = externalRightFloatCount; i < rightFloats.size(); ++i ) {
					auto& f = rightFloats[i];
					if ( f.Bottom > curY ) {
						curY = f.Bottom;
						advanced = true;
					}
				}
			}
			return advanced;
		};

		size_t finalizedLineCount = 0;
		Float finalizedLinesBottom = 0.f;
		// Propagated float groups need final line metrics before translating their exclusions.
		// Keep a monotonic cursor so each preceding line is finalized at most once here.
		auto finalizeLinesThrough = [&]( size_t end ) {
			for ( ; finalizedLineCount < end; ++finalizedLineCount ) {
				auto& line = result.lines[finalizedLineCount];
				alignLineSpans( line, 0.f, defaultStyle, forcedLineHeight, true, inlineItems );
				finalizedLinesBottom =
					eemax( finalizedLinesBottom, line.y + line.height );
			}
		};

		auto advanceInFlowLine = [&]() {
			finalizeLinesThrough( result.lines.size() );
			curY = eemax( curY, finalizedLinesBottom );
			result.lines.push_back( RichText::RenderParagraph() );
			result.lines.back().y = curY;
			curX = 0;
		};

		for ( const auto& run : runs ) {
			const auto& payload = run.payload;
			if ( payload.type == RichText::RenderSpan::Type::Text ) {
				auto& span = payload.text;
				if ( !span )
					continue;

				if ( span->getString().empty() ) {
					Float startSpacing = inlineStartSpacing( payload, inlineItems );
					Float endSpacing = inlineEndSpacing( payload, inlineItems );
					if ( startSpacing <= 0 && endSpacing <= 0 )
						continue;
					addInlineSpacingToCurrentLine( result, curX, startSpacing + endSpacing );
					continue;
				}

				auto& fontStyle = span->getFontStyleConfig();
				if ( !fontStyle.Font )
					continue;

				addInlineSpacingToCurrentLine( result, curX,
											   inlineStartSpacing( payload, inlineItems ) );

				Float le = floatLeftEdge( curY );
				if ( curX < le )
					curX = le;

				Float effW = effectiveMaxWidthAt( curY );
				if ( maxLayoutWidth > 0 && maxLayoutWidth < effW )
					effW = maxLayoutWidth;

				LineWrapInfoEx wrapInfo = computeTextWraps(
					payload, fontStyle, effW, lineWrap && effW > 0, curX, whiteSpaceWrapMode );

				for ( size_t i = 0; i < wrapInfo.wraps.size() - 1; ++i ) {
					size_t startIdx = wrapInfo.wraps[i];
					size_t endIdx = wrapInfo.wraps[i + 1];
					bool startsWithNewline =
						startIdx < endIdx && span->getString()[startIdx] == '\n';
					bool isNewline = startsWithNewline && endIdx - startIdx == 1;
					if ( startsWithNewline && !isNewline ) {
						curCharIdx++;
						startIdx++;
					}

					if ( !isNewline ) {
						appendTextRenderSpan( result.lines.back(), payload, fontStyle, startIdx,
											  endIdx, curX, curCharIdx, inlineItems );
					}

					if ( i == wrapInfo.wraps.size() - 2 && !isNewline ) {
						addInlineSpacingToCurrentLine( result, curX,
													   inlineEndSpacing( payload, inlineItems ) );
						if ( lineWrap && effW > 0 && effW < 1e9f && curX > effW ) {
							maxWidth = std::max( maxWidth, curX );
							advanceInFlowLine();
							continue;
						}
					}

					bool isLastTextSegment = i + 2 == wrapInfo.wraps.size();
					bool trailingNewlineAlreadyAdvanced =
						isNewline && isLastTextSegment && result.lines.back().spans.empty() &&
						i > 0 && wrapInfo.wraps[i - 1] != wrapInfo.wraps[i];
					if ( i < wrapInfo.wraps.size() - 2 || isNewline ) {
						if ( isNewline ) {
							curCharIdx++;
							if ( isLastTextSegment ) {
								addInlineSpacingToCurrentLine(
									result, curX, inlineEndSpacing( payload, inlineItems ) );
							}
						}
						if ( !trailingNewlineAlreadyAdvanced ) {
							maxWidth = std::max( maxWidth, curX );
							advanceInFlowLine();
						}
					}
				}
			} else {
				AtomicBlockMetrics metrics = getAtomicBlockMetrics( payload );

				if ( metrics.isLineBreak ) {
					maxWidth = std::max( maxWidth, curX );
					if ( !result.lines.back().spans.empty() &&
						 lineHasInFlowContent( result.lines.back() ) ) {
						curY += result.lines.back().height;
						result.lines.push_back( RichText::RenderParagraph() );
						result.lines.back().y = curY;
					}
					curX = 0;
					continue;
				}

				if ( metrics.clearType != RichText::InlineClear::None ) {
					if ( clearFloats( metrics.clearType ) ) {
						maxWidth = std::max( maxWidth, curX );
						result.lines.push_back( RichText::RenderParagraph() );
						result.lines.back().y = curY;
						curX = 0;
					}
				}

				Float le = floatLeftEdge( curY );

				if ( metrics.floatType != RichText::InlineFloat::None ) {
					Float posX;
					if ( metrics.floatType == RichText::InlineFloat::Left ) {
						posX = le;
						Float availW = floatRightEdge( curY );
						if ( availW > 0 && availW < 1e9f &&
							 posX + metrics.size.getWidth() > availW + 0.01f ) {
							Float maxBottom = curY;
							for ( auto& f : leftFloats )
								maxBottom = std::max( maxBottom, f.Bottom );
							for ( auto& f : rightFloats )
								maxBottom = std::max( maxBottom, f.Bottom );
							if ( maxBottom > curY ) {
								maxWidth = std::max( maxWidth, curX );
								result.lines.push_back( RichText::RenderParagraph() );
								curX = 0;
								curY = maxBottom;
								result.lines.back().y = curY;
								posX = floatLeftEdge( curY );
							}
						}
					} else {
						Float re = floatRightEdge( curY );
						posX = re - metrics.size.getWidth();
						if ( metrics.size.getWidth() > re - le + 0.01f ) {
							Float maxBottom = curY;
							for ( auto& f : leftFloats )
								maxBottom = std::max( maxBottom, f.Bottom );
							for ( auto& f : rightFloats )
								maxBottom = std::max( maxBottom, f.Bottom );
							if ( maxBottom > curY ) {
								maxWidth = std::max( maxWidth, curX );
								result.lines.push_back( RichText::RenderParagraph() );
								curX = 0;
								curY = maxBottom;
								result.lines.back().y = curY;
								re = floatRightEdge( curY );
								le = floatLeftEdge( curY );
								posX = re - metrics.size.getWidth();
							}
						}
						if ( posX < le )
							posX = le;
					}

					appendPositionedAtomicRenderSpan( result.lines.back(), payload, metrics.size,
													  { posX, 0 }, curCharIdx );

					Rectf fr( posX, curY, posX + metrics.size.getWidth(),
							  curY + metrics.size.getHeight() );
					if ( metrics.floatType == RichText::InlineFloat::Left )
						leftFloats.push_back( fr );
					else
						rightFloats.push_back( fr );
				} else {
					if ( !metrics.isBlock && curX < le )
						curX = le;

					Float startSpacing = 0.f;
					Float endSpacing = 0.f;
					if ( payload.type == RichText::RenderSpan::Type::AtomicBox ) {
						startSpacing = inlineStartSpacing( payload, inlineItems );
						endSpacing = inlineEndSpacing( payload, inlineItems );
					}
					bool hadLineContentBeforeSpacing = !result.lines.back().spans.empty();
					addInlineSpacingToCurrentLine( result, curX, startSpacing );

					Float effW = metrics.isBlock ? maxLayoutWidth : effectiveMaxWidthAt( curY );
					if ( metrics.isBlockFormattingContext && metrics.isBlock ) {
						le = floatLeftEdge( curY );
						Float re = floatRightEdge( curY );
						Float availableWidth = re - le;
						if ( availableWidth > 0 && availableWidth < 1e9f ) {
							if ( metrics.size.getWidth() > availableWidth + 0.01f )
								metrics.size.setWidth( availableWidth );
							curX = le;
							effW = availableWidth;
						}
					}

					if ( lineWrap && !metrics.isBlock && effW > 0 && effW < 1e9f &&
						 metrics.size.getWidth() > effW + 0.01f ) {
						Float placedY = curY;
						Float placedWidth = effW;
						while ( metrics.size.getWidth() > placedWidth + 0.01f ) {
							Float nextY = activeFloatBottom( placedY );
							if ( nextY <= placedY )
								break;
							placedY = nextY;
							placedWidth = effectiveMaxWidthAt( placedY );
						}
						if ( placedY > curY ) {
							maxWidth = std::max( maxWidth, curX );
							if ( !result.lines.back().spans.empty() )
								result.lines.push_back( RichText::RenderParagraph() );
							curX = 0;
							curY = placedY;
							result.lines.back().y = curY;
							le = floatLeftEdge( curY );
							effW = placedWidth;
						}
					}

					if ( lineWrap && !metrics.isBlock && effW > 0 && effW < 1e9f &&
						 ( curX + metrics.size.getWidth() >= effW || curX >= effW ) && curX > 0 &&
						 hadLineContentBeforeSpacing ) {
						maxWidth = std::max( maxWidth, curX );
						advanceInFlowLine();
						if ( hadLineContentBeforeSpacing )
							addInlineSpacingToCurrentLine( result, curX, startSpacing );
					}

					if ( payload.propagatedFloats ) {
						finalizeLinesThrough( result.lines.size() - 1 );
						if ( finalizedLinesBottom > curY ) {
							curY = finalizedLinesBottom;
							result.lines.back().y = curY;
						}
						Float placedY = curY;
						while ( true ) {
							Float nextPlacedY = placedY;
							for ( const auto& propagated : *payload.propagatedFloats ) {
								Rectf rect = propagated.rect;
								rect.move( { curX, placedY } );
								auto avoidActiveFloat = [&]( const Rectf& active ) {
									if ( rect.intersect( active ) )
										nextPlacedY = std::max(
											nextPlacedY, placedY + active.Bottom - rect.Top );
								};
								for ( const auto& active : leftFloats )
									avoidActiveFloat( active );
								for ( const auto& active : rightFloats )
									avoidActiveFloat( active );
							}
							if ( nextPlacedY <= placedY + 0.01f )
								break;
							placedY = nextPlacedY;
						}
						if ( placedY > curY ) {
							curY = placedY;
							result.lines.back().y = curY;
						}
					}

					appendAtomicRenderSpan( result.lines.back(), payload, metrics, curX,
											curCharIdx );
					if ( payload.propagatedFloats ) {
						const Vector2f atomicPosition = result.lines.back().spans.back().position;
						for ( const auto& propagated : *payload.propagatedFloats ) {
							Rectf rect = propagated.rect;
							rect.move( { atomicPosition.x, curY } );
							if ( propagated.type == RichText::InlineFloat::Left )
								leftFloats.push_back( rect );
							else if ( propagated.type == RichText::InlineFloat::Right )
								rightFloats.push_back( rect );
						}
					}
					addInlineSpacingToCurrentLine( result, curX, endSpacing );

					if ( lineWrap && effW > 0 && effW < 1e9f && curX >= effW ) {
						maxWidth = std::max( maxWidth, curX );
						advanceInFlowLine();
					}
				}
			}
		}

		maxWidth = std::max( maxWidth, curX );

		if ( !result.lines.empty() && result.lines.back().spans.empty() &&
			 result.lines.size() > 1 ) {
			result.lines.pop_back();
		}

		Float accumY = 0;
		for ( auto& line : result.lines ) {
			if ( line.y > accumY )
				accumY = line.y;
			line.y = accumY;

			Float xOffset = horizontalAlignmentOffset( line, maxLayoutWidth, align );
			alignLineSpans( line, xOffset, defaultStyle, forcedLineHeight, true, inlineItems );
			accumY += line.height;
		}

		Float floatBoundsBottom = 0;
		Float floatBoundsRight = 0;
		for ( size_t i = externalLeftFloatCount; i < leftFloats.size(); ++i ) {
			floatBoundsBottom = std::max( floatBoundsBottom, leftFloats[i].Bottom );
			floatBoundsRight = std::max( floatBoundsRight, leftFloats[i].Right );
		}
		for ( size_t i = externalRightFloatCount; i < rightFloats.size(); ++i ) {
			floatBoundsBottom = std::max( floatBoundsBottom, rightFloats[i].Bottom );
			floatBoundsRight = std::max( floatBoundsRight, rightFloats[i].Right );
		}
		// This is persistent RichText storage: clear without discarding capacity so repeated
		// viewport-driven layouts do not allocate a fresh exclusion vector every time.
		localFloatExclusions.clear();
		localFloatExclusions.reserve( leftFloats.size() + rightFloats.size() -
									  externalLeftFloatCount - externalRightFloatCount );
		for ( size_t i = externalLeftFloatCount; i < leftFloats.size(); ++i )
			localFloatExclusions.push_back( { leftFloats[i], RichText::InlineFloat::Left } );
		for ( size_t i = externalRightFloatCount; i < rightFloats.size(); ++i )
			localFloatExclusions.push_back(
				{ rightFloats[i], RichText::InlineFloat::Right } );

		result.size =
			Sizef( std::max( maxWidth, floatBoundsRight ), std::max( accumY, floatBoundsBottom ) );
		result.totalCharacterCount = curCharIdx;
		return result;
	}

	// Resolve a render span to its source InlineItem leaf node.  The fast
	// path uses the sequential _leafIndex assigned during buildLayoutRuns:
	// that index directly addresses the leaves vector built by collectLeaves
	// because both walk the InlineItem tree in the same depth-first order.
	// The fallback via path-based linear search handles legacy spans that
	// lack a leaf index.
	static const InlineLeafRef* resolveLeaf( const RichText::RenderSpan& span,
											 const SmallVector<InlineLeafRef, 32>& leaves,
											 RichText::InlineFragment::Type type,
											 size_t& resolvedLeafIndex, size_t& leafIndex ) {
		if ( span._leafIndex >= 0 && static_cast<size_t>( span._leafIndex ) < leaves.size() ) {
			resolvedLeafIndex = static_cast<size_t>( span._leafIndex );
			const auto& leaf = leaves[resolvedLeafIndex];
			if ( leaf.type == type )
				return &leaf;
		}
		const InlineLeafRef* leaf =
			findLeafByPath( leaves, span.inlinePath, type, resolvedLeafIndex );
		if ( leaf == nullptr && span.inlinePath.empty() ) {
			resolvedLeafIndex = leafIndex;
			leaf = findNextLeaf( leaves, resolvedLeafIndex, type );
		}
		return leaf;
	}

	static std::vector<RichText::InlineFragment>
	rebuildFragments( const std::vector<RichText::InlineItem>& inlineItems,
					  const std::vector<RichText::RenderParagraph>& lines ) {
		if ( inlineItems.empty() || lines.empty() )
			return {};

		SmallVector<InlineLeafRef, 32> leaves;
		RichText::RenderSpan::InlinePath path;
		SmallVector<InlineAncestorRef, 4> ancestors;
		collectLeaves( inlineItems, path, ancestors, leaves );
		if ( leaves.empty() )
			return {};

		std::vector<RichText::InlineFragment> fragments;
		fragments.reserve( leaves.size() );
		SmallVector<InlineBoxFragmentAccumulator, 16> boxFragments;

		auto addBoxFragments = [&]( const InlineLeafRef& leaf, size_t leafIndex, size_t lineIndex,
									const Rectf& bounds, bool startsLeafFragment,
									bool endsLeafFragment ) {
			for ( const auto& ancestor : leaf.ancestors ) {
				Rectf ancestorBounds = expandInlineBoundsForLineHeight( bounds, *ancestor.box );
				bool startsInlineBox =
					startsLeafFragment &&
					isFirstInlineLeafInBox( *ancestor.box, ancestor.path, leaf.path );
				bool endsInlineBox =
					endsLeafFragment &&
					isLastInlineLeafInBox( *ancestor.box, ancestor.path, leaf.path );
				auto it =
					std::find_if( boxFragments.begin(), boxFragments.end(), [&]( const auto& f ) {
						return f.lineIndex == lineIndex && f.box == ancestor.box;
					} );
				if ( it == boxFragments.end() ) {
					InlineBoxFragmentAccumulator acc;
					acc.path = ancestor.path;
					acc.lineIndex = lineIndex;
					acc.box = ancestor.box;
					acc.firstLeafIndex = leafIndex;
					acc.lastLeafIndex = leafIndex;
					acc.startsInlineBox = startsInlineBox;
					acc.endsInlineBox = endsInlineBox;
					expandInlineBounds( acc.bounds, acc.valid, ancestorBounds );
					expandInlineBounds( acc.contentBounds, acc.contentValid, bounds );
					boxFragments.push_back( std::move( acc ) );
				} else {
					it->firstLeafIndex = std::min( it->firstLeafIndex, leafIndex );
					it->lastLeafIndex = std::max( it->lastLeafIndex, leafIndex );
					it->startsInlineBox = it->startsInlineBox || startsInlineBox;
					it->endsInlineBox = it->endsInlineBox || endsInlineBox;
					expandInlineBounds( it->bounds, it->valid, ancestorBounds );
					expandInlineBounds( it->contentBounds, it->contentValid, bounds );
				}
			}
		};

		size_t leafIndex = 0;
		SmallVector<size_t, 32> leafTextOffsets( leaves.size(), 0 );
		for ( size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex ) {
			const auto& line = lines[lineIndex];
			for ( const auto& span : line.spans ) {
				Rectf bounds( { span.position.x, line.y + span.position.y }, span.size );
				if ( span.type == RichText::RenderSpan::Type::Text ) {
					Int64 spanChars = span.endCharIndex - span.startCharIndex;
					if ( spanChars <= 0 )
						continue;

					size_t resolvedLeafIndex = 0;
					const InlineLeafRef* leaf =
						resolveLeaf( span, leaves, RichText::InlineFragment::Type::TextRun,
									 resolvedLeafIndex, leafIndex );
					if ( leaf == nullptr )
						continue;

					RichText::InlineFragment fragment;
					fragment.type = RichText::InlineFragment::Type::TextRun;
					fragment.itemPath = leaf->path;
					fragment.lineIndex = lineIndex;
					fragment.bounds = bounds;
					fragment.startCharIndex = span.startCharIndex;
					fragment.endCharIndex = span.endCharIndex;
					fragment.text = span.text;
					fragment.baselineAlign =
						effectiveInlineBaselineAlign( inlineItems, leaf->path, span.baselineAlign );
					fragment.source = leaf->source;
					fragment.textDecoration =
						( ( span.text ? span.text->getStyle() : 0 ) |
						  inlineAncestorTextDecoration( inlineItems, leaf->path ) ) &
						( Text::Underlined | Text::StrikeThrough );
					fragments.push_back( std::move( fragment ) );

					size_t sourceChars = leaf->text ? leaf->text->getString().size() : 0;
					bool startsLeafFragment = leafTextOffsets[resolvedLeafIndex] == 0;
					leafTextOffsets[resolvedLeafIndex] += static_cast<size_t>( spanChars );
					bool endsLeafFragment =
						sourceChars == 0 || leafTextOffsets[resolvedLeafIndex] >= sourceChars;
					if ( sourceChars == 0 || leafTextOffsets[resolvedLeafIndex] >= sourceChars ) {
						leafIndex = std::max( leafIndex, resolvedLeafIndex + 1 );
					} else {
						leafIndex = std::max( leafIndex, resolvedLeafIndex );
					}
					addBoxFragments( *leaf, resolvedLeafIndex, lineIndex, bounds,
									 startsLeafFragment, endsLeafFragment );
				} else {
					if ( span.isLineBreak )
						continue;

					size_t resolvedLeafIndex = 0;
					const InlineLeafRef* leaf =
						resolveLeaf( span, leaves, RichText::InlineFragment::Type::AtomicBox,
									 resolvedLeafIndex, leafIndex );
					if ( leaf == nullptr )
						continue;

					RichText::InlineFragment fragment;
					fragment.type = RichText::InlineFragment::Type::AtomicBox;
					fragment.itemPath = leaf->path;
					fragment.lineIndex = lineIndex;
					fragment.bounds = bounds;
					fragment.startCharIndex = span.startCharIndex;
					fragment.endCharIndex = span.endCharIndex;
					fragment.baselineAlign = effectiveInlineBaselineAlign(
						inlineItems, leaf->path,
						span.baselineAlign.type == RichText::BaselineAlignment::Baseline
							? leaf->baselineAlign
							: span.baselineAlign );
					fragment.source = leaf->source;
					fragment.textDecoration =
						inlineAncestorTextDecoration( inlineItems, leaf->path );
					fragments.push_back( std::move( fragment ) );
					addBoxFragments( *leaf, resolvedLeafIndex, lineIndex, bounds, true, true );
					leafIndex = std::max( leafIndex, resolvedLeafIndex + 1 );
				}
			}
		}

		appendBoxFragments( fragments, boxFragments, inlineItems );
		return fragments;
	}

  private:
	struct AtomicBlockMetrics {
		Sizef size;
		Float baseline{ 0.f };
		bool isLineBreak{ false };
		bool isBlock{ false };
		bool isBlockFormattingContext{ false };
		RichText::InlineFloat floatType{ RichText::InlineFloat::None };
		RichText::InlineClear clearType{ RichText::InlineClear::None };
	};

	static SmallVector<InlineLayoutRun, 32>
	buildLayoutRuns( const std::vector<RichText::InlineItem>& inlineItems ) {
		SmallVector<InlineLayoutRun, 32> runs;
		RichText::RenderSpan::InlinePath path;
		Int64 nextLeafIndex = 0;
		appendLayoutRuns( inlineItems, path, runs, nextLeafIndex );
		return runs;
	}

	static void appendLayoutRuns( const std::vector<RichText::InlineItem>& items,
								  RichText::RenderSpan::InlinePath& path,
								  SmallVector<InlineLayoutRun, 32>& runs, Int64& nextLeafIndex ) {
		for ( size_t i = 0; i < items.size(); ++i ) {
			path.push_back( i );
			const auto& item = items[i];
			if ( item.isBox() ) {
				const auto& box = item.asBox();
				if ( box.children.empty() )
					appendEmptyBoxLayoutRun( path, runs );
				else
					appendLayoutRuns( box.children, path, runs, nextLeafIndex );
			} else if ( item.isTextRun() ) {
				appendTextLayoutRun( item.asTextRun(), path, runs, nextLeafIndex );
			} else {
				appendAtomicLayoutRun( item.asAtomicBox(), path, runs, nextLeafIndex );
			}
			path.pop_back();
		}
	}

	static void appendEmptyBoxLayoutRun( const RichText::RenderSpan::InlinePath& path,
										 SmallVector<InlineLayoutRun, 32>& runs ) {
		InlineLayoutRun run;
		run.payload.type = RichText::RenderSpan::Type::Text;
		run.payload.inlinePath = path;
		runs.push_back( std::move( run ) );
	}

	static void appendTextLayoutRun( const RichText::InlineItem::TextRun& textRun,
									 const RichText::RenderSpan::InlinePath& path,
									 SmallVector<InlineLayoutRun, 32>& runs,
									 Int64& nextLeafIndex ) {
		InlineLayoutRun run;
		run.payload.type = RichText::RenderSpan::Type::Text;
		run.payload.text = textRun.text;
		run.payload.margin = textRun.margin;
		run.payload.padding = textRun.padding;
		run.payload.lineHeight = textRun.lineHeight;
		run.payload.baselineAlign = textRun.baselineAlign;
		run.payload.suppressBackground = textRun.suppressBackground;
		run.payload.inlinePath = path;
		run.payload._leafIndex = nextLeafIndex++;
		runs.push_back( std::move( run ) );
	}

	static void appendAtomicLayoutRun( const RichText::InlineItem::AtomicBox& box,
									   const RichText::RenderSpan::InlinePath& path,
									   SmallVector<InlineLayoutRun, 32>& runs,
									   Int64& nextLeafIndex ) {
		InlineLayoutRun run;
		run.payload.type = box.drawable ? RichText::RenderSpan::Type::Drawable
										: RichText::RenderSpan::Type::AtomicBox;
		run.payload.drawable = box.drawable;
		run.payload.size = box.drawable ? box.drawable->getPixelsSize() : box.size;
		run.payload.baseline = box.drawable ? run.payload.size.getHeight() : box.baseline;
		run.payload.floatType = box.floatType;
		run.payload.clearType = box.clearType;
		run.payload.isLineBreak = box.isLineBreak;
		run.payload.isBlock = box.isBlock;
		run.payload.isBlockFormattingContext = box.isBlockFormattingContext;
		run.payload.propagatedFloats = box.propagatedFloats;
		run.payload.baselineAlign = box.baselineAlign;
		run.payload.inlinePath = path;
		run.payload._leafIndex = nextLeafIndex++;
		runs.push_back( std::move( run ) );
	}

	static Float inlineStartSpacing( const RichText::RenderSpan& span,
									 const std::vector<RichText::InlineItem>& inlineItems ) {
		return span.margin.Left + span.padding.Left +
			   inlineAncestorStartSpacing( inlineItems, span.inlinePath );
	}

	static Float inlineEndSpacing( const RichText::RenderSpan& span,
								   const std::vector<RichText::InlineItem>& inlineItems ) {
		return span.margin.Right + span.padding.Right +
			   inlineAncestorEndSpacing( inlineItems, span.inlinePath );
	}

	static void addInlineSpacingToCurrentLine( LayoutResult& result, Float& curX, Float spacing ) {
		if ( spacing == 0 )
			return;
		curX += spacing;
		if ( !result.lines.empty() )
			result.lines.back().width += spacing;
	}

	static bool isBreakSpacesWrapChar( String::StringBaseType ch ) {
		return ch == ' ' || ch == '\t';
	}

	static Float breakSpacesTextWidth( const String& string, size_t start, size_t end,
									   const FontStyleConfig& fontStyle, Uint32 tabWidth,
									   Uint32 textHints ) {
		if ( end <= start )
			return 0.f;
		return Text::getTextWidth( string.substr( start, end - start ), fontStyle, tabWidth,
								   textHints );
	}

	static LineWrapInfoEx computeBreakSpacesTextWraps( const RichText::RenderSpan& span,
													   const FontStyleConfig& fontStyle,
													   Float wrapWidth, bool shouldWrap,
													   Float curX ) {
		LineWrapInfoEx wrapInfo;
		const String& string = span.text->getString();
		wrapInfo.wraps.push_back( 0 );

		if ( string.empty() )
			return wrapInfo;

		const Uint32 tabWidth = span.text->getTabWidth();
		const Uint32 textHints = span.text->getTextHints();
		size_t lineStart = 0;
		size_t lastBreakAfter = String::InvalidPos;
		Float lineBaseX = curX;

		auto visualStart = [&string]( size_t start ) {
			return start < string.size() && string[start] == '\n' ? start + 1 : start;
		};
		auto pushWrap = [&wrapInfo]( size_t index, Float width ) {
			if ( wrapInfo.wraps.back() != static_cast<Int64>( index ) ) {
				wrapInfo.wrapsWidth.push_back( std::ceil( width ) );
				wrapInfo.wraps.push_back( index );
			}
		};

		for ( size_t idx = 0; idx < string.size(); ++idx ) {
			if ( string[idx] == '\n' ) {
				pushWrap( idx,
						  lineBaseX + breakSpacesTextWidth( string, visualStart( lineStart ), idx,
															fontStyle, tabWidth, textHints ) );
				lineStart = idx;
				lastBreakAfter = String::InvalidPos;
				lineBaseX = 0.f;
				continue;
			}

			size_t start = visualStart( lineStart );
			Float candidateWidth =
				lineBaseX +
				breakSpacesTextWidth( string, start, idx + 1, fontStyle, tabWidth, textHints );

			if ( shouldWrap && wrapWidth > 0 && candidateWidth > wrapWidth ) {
				if ( lastBreakAfter != String::InvalidPos && lastBreakAfter > lineStart ) {
					pushWrap( lastBreakAfter,
							  lineBaseX + breakSpacesTextWidth( string, start, lastBreakAfter,
																fontStyle, tabWidth, textHints ) );
					lineStart = lastBreakAfter;
					lastBreakAfter = String::InvalidPos;
					lineBaseX = 0.f;
				} else if ( isBreakSpacesWrapChar( string[idx] ) ) {
					pushWrap( idx + 1, candidateWidth );
					lineStart = idx + 1;
					lastBreakAfter = String::InvalidPos;
					lineBaseX = 0.f;
					continue;
				} else if ( idx > start ) {
					pushWrap( idx, lineBaseX + breakSpacesTextWidth( string, start, idx, fontStyle,
																	 tabWidth, textHints ) );
					lineStart = idx;
					lastBreakAfter = String::InvalidPos;
					lineBaseX = 0.f;
				}
			}

			if ( isBreakSpacesWrapChar( string[idx] ) && lineStart <= idx )
				lastBreakAfter = idx + 1;
		}

		if ( !string.empty() && string[string.size() - 1] != '\n' ) {
			wrapInfo.wrapsWidth.push_back(
				std::ceil( lineBaseX + breakSpacesTextWidth( string, visualStart( lineStart ),
															 string.size(), fontStyle, tabWidth,
															 span.text->getTextHints() ) ) );
		}

		if ( wrapInfo.wraps.back() != static_cast<Int64>( string.size() ) )
			wrapInfo.wraps.push_back( string.size() );

		return wrapInfo;
	}

	static LineWrapInfoEx computeTextWraps( const RichText::RenderSpan& span,
											const FontStyleConfig& fontStyle, Float wrapWidth,
											bool shouldWrap, Float curX,
											RichText::WhiteSpaceWrapMode whiteSpaceWrapMode ) {
		if ( whiteSpaceWrapMode == RichText::WhiteSpaceWrapMode::BreakSpaces )
			return computeBreakSpacesTextWraps( span, fontStyle, wrapWidth, shouldWrap, curX );

		LineWrapInfoEx wrapInfo = LineWrap::computeLineBreaksEx(
			span.text->getString(), fontStyle, wrapWidth > 0 ? wrapWidth : 1e9f,
			shouldWrap ? LineWrapMode::Word : LineWrapMode::NoWrap, false, span.text->getTabWidth(),
			0.f, span.text->getTextHints(), false, curX );

		if ( wrapInfo.wraps.empty() ||
			 wrapInfo.wraps.back() != (Float)span.text->getString().size() )
			wrapInfo.wraps.push_back( span.text->getString().size() );

		return wrapInfo;
	}

	static void recomputeLineMetrics( RichText::RenderParagraph& line, Float forcedLineHeight,
									  bool preserveFloatPositions,
									  const std::vector<RichText::InlineItem>& inlineItems ) {
		Float maxAscent = 0.f;
		Float maxDescent = 0.f;
		bool hasParticipatingLineHeight = forcedLineHeight > 0.f;

		for ( const auto& span : line.spans ) {
			if ( span.type == RichText::RenderSpan::Type::Text ) {
				InlineVerticalEdges edges = inlineAncestorLineHeightEdges(
					inlineItems, span.inlinePath, span.size.getHeight() );
				hasParticipatingLineHeight =
					hasParticipatingLineHeight || edges.top > 0 || edges.bottom > 0;
				Float lineHeight =
					span.lineHeight > 0.f
						? getTextRunLineHeight( span.text, span.lineHeight )
						: eemax( forcedLineHeight,
								 getTextRunLineHeight( span.text, span.lineHeight ) );
				Float baseline = getTextLineBoxBaselineForHeight( span.text, lineHeight );
				maxAscent = std::max( maxAscent, baseline + edges.top );
				maxDescent = std::max( maxDescent, lineHeight - baseline + edges.bottom );
			} else {
				bool isFloat = span.floatType != RichText::InlineFloat::None;
				Float baseline = span.baseline;
				InlineVerticalEdges edges = inlineAncestorLineHeightEdges(
					inlineItems, span.inlinePath, span.size.getHeight() );
				if ( preserveFloatPositions && isFloat )
					continue;
				hasParticipatingLineHeight =
					hasParticipatingLineHeight || edges.top > 0 || edges.bottom > 0;
				maxAscent = std::max( maxAscent, baseline + edges.top );
				maxDescent =
					std::max( maxDescent, span.size.getHeight() - baseline + edges.bottom );
			}
		}

		if ( !hasParticipatingLineHeight )
			return;

		line.maxAscent = maxAscent;
		line.height = std::max( maxAscent + maxDescent, forcedLineHeight );
	}

	static void appendTextRenderSpan( RichText::RenderParagraph& line,
									  const RichText::RenderSpan& payload,
									  const FontStyleConfig& fontStyle, size_t startIdx,
									  size_t endIdx, Float& curX, Int64& curCharIdx,
									  const std::vector<RichText::InlineItem>& inlineItems ) {
		std::shared_ptr<Text> renderSpanText = std::make_shared<Text>();
		renderSpanText->setString(
			payload.text->getString().substr( startIdx, endIdx - startIdx ) );
		FontStyleConfig renderStyle = fontStyle;
		renderStyle.Style |= inlineAncestorTextDecoration( inlineItems, payload.inlinePath );
		renderSpanText->setStyleConfig( renderStyle );
		renderSpanText->setTabWidth( payload.text->getTabWidth() );

		Float height = getTextVisualLineHeight( payload.text, payload.lineHeight );
		Float lineHeight = getTextRunLineHeight( payload.text, payload.lineHeight );
		Float ascent = getTextLineBoxBaseline( payload.text, payload.lineHeight );
		Float spanWidth = renderSpanText->getTextWidth();

		RichText::RenderSpan renderSpan;
		renderSpan.type = RichText::RenderSpan::Type::Text;
		renderSpan.text = renderSpanText;
		renderSpan.margin = payload.margin;
		renderSpan.padding = payload.padding;
		renderSpan.lineHeight = payload.lineHeight;
		renderSpan.baselineAlign = payload.baselineAlign;
		renderSpan.suppressBackground = payload.suppressBackground;
		renderSpan.inlinePath = payload.inlinePath;
		renderSpan._leafIndex = payload._leafIndex;
		renderSpan.position = { curX, 0 };
		renderSpan.size = Sizef( spanWidth, height );
		renderSpan.startCharIndex = curCharIdx;
		renderSpan.endCharIndex = static_cast<Int64>( curCharIdx + ( endIdx - startIdx ) );
		curCharIdx = renderSpan.endCharIndex;

		line.spans.push_back( std::move( renderSpan ) );
		line.maxAscent = std::max( line.maxAscent, ascent );
		line.height = std::max( line.height, lineHeight );

		curX += spanWidth;
		line.width += spanWidth;
	}

	static AtomicBlockMetrics getAtomicBlockMetrics( const RichText::RenderSpan& payload ) {
		AtomicBlockMetrics metrics;
		if ( payload.type == RichText::RenderSpan::Type::Drawable ) {
			metrics.size = payload.drawable ? payload.drawable->getPixelsSize() : Sizef();
			metrics.baseline = metrics.size.getHeight();
		} else {
			metrics.size = payload.size;
			metrics.baseline = payload.baseline;
			metrics.isLineBreak = payload.isLineBreak;
			metrics.isBlock = payload.isBlock;
			metrics.isBlockFormattingContext = payload.isBlockFormattingContext;
			metrics.floatType = payload.floatType;
			metrics.clearType = payload.clearType;
		}
		return metrics;
	}

	static void appendPositionedAtomicRenderSpan( RichText::RenderParagraph& line,
												  const RichText::RenderSpan& payload,
												  const Sizef& size, const Vector2f& position,
												  Int64& curCharIdx ) {
		RichText::RenderSpan renderSpan = payload;
		renderSpan.position = position;
		renderSpan.size = size;
		renderSpan.startCharIndex = curCharIdx;
		renderSpan.endCharIndex = curCharIdx + 1;
		curCharIdx = renderSpan.endCharIndex;
		line.spans.push_back( std::move( renderSpan ) );
	}

	static void appendAtomicRenderSpan( RichText::RenderParagraph& line,
										const RichText::RenderSpan& payload,
										const AtomicBlockMetrics& metrics, Float& curX,
										Int64& curCharIdx ) {
		appendPositionedAtomicRenderSpan( line, payload, metrics.size, { curX, 0 }, curCharIdx );
		line.maxAscent = std::max( line.maxAscent, metrics.baseline );
		line.height = std::max( line.height, metrics.size.getHeight() );

		curX += metrics.size.getWidth();
		line.width += metrics.size.getWidth();
	}

	static bool lineHasInFlowContent( const RichText::RenderParagraph& line ) {
		for ( const auto& span : line.spans ) {
			if ( span.floatType == RichText::InlineFloat::None )
				return true;
		}
		return false;
	}

	static Float horizontalAlignmentOffset( const RichText::RenderParagraph& line,
											Float maxLayoutWidth, Uint32 align ) {
		if ( maxLayoutWidth <= 0 || align == 0 )
			return 0;

		Uint32 hAlign = Font::getHorizontalAlign( align );
		if ( hAlign == TEXT_ALIGN_CENTER )
			return ( maxLayoutWidth - line.width ) * 0.5f;
		if ( hAlign == TEXT_ALIGN_RIGHT )
			return maxLayoutWidth - line.width;
		return 0;
	}

	static void collectLeaves( const std::vector<RichText::InlineItem>& items,
							   RichText::RenderSpan::InlinePath& path,
							   SmallVector<InlineAncestorRef, 4>& ancestors,
							   SmallVector<InlineLeafRef, 32>& leaves ) {
		for ( size_t i = 0; i < items.size(); ++i ) {
			path.push_back( i );
			const auto& item = items[i];
			if ( item.isBox() ) {
				const auto& box = item.asBox();
				ancestors.push_back( InlineAncestorRef{ path, &box } );
				collectLeaves( box.children, path, ancestors, leaves );
				ancestors.pop_back();
			} else {
				InlineLeafRef leaf;
				leaf.path = path;
				leaf.ancestors = ancestors;
				if ( item.isTextRun() ) {
					leaf.type = RichText::InlineFragment::Type::TextRun;
					leaf.text = item.asTextRun().text;
					leaf.source = item.asTextRun().source;
				} else {
					leaf.type = RichText::InlineFragment::Type::AtomicBox;
					leaf.source = item.asAtomicBox().source;
					leaf.isLineBreak = item.asAtomicBox().isLineBreak;
					leaf.baselineAlign = item.asAtomicBox().baselineAlign;
				}
				leaves.push_back( std::move( leaf ) );
			}
			path.pop_back();
		}
	}

	static const InlineLeafRef* findNextLeaf( const SmallVector<InlineLeafRef, 32>& leaves,
											  size_t& leafIndex,
											  RichText::InlineFragment::Type type ) {
		while ( leafIndex < leaves.size() &&
				( leaves[leafIndex].type != type ||
				  ( type == RichText::InlineFragment::Type::AtomicBox &&
					leaves[leafIndex].isLineBreak ) ) )
			leafIndex++;
		return leafIndex < leaves.size() ? &leaves[leafIndex] : nullptr;
	}

	static const InlineLeafRef* findLeafByPath( const SmallVector<InlineLeafRef, 32>& leaves,
												const RichText::RenderSpan::InlinePath& path,
												RichText::InlineFragment::Type type,
												size_t& leafIndex ) {
		if ( path.empty() )
			return nullptr;

		for ( size_t i = 0; i < leaves.size(); ++i ) {
			if ( leaves[i].type == type && sameInlinePath( leaves[i].path, path ) ) {
				leafIndex = i;
				return &leaves[i];
			}
		}
		return nullptr;
	}

	static void
	appendBoxFragments( std::vector<RichText::InlineFragment>& fragments,
						const SmallVector<InlineBoxFragmentAccumulator, 16>& boxFragments,
						const std::vector<RichText::InlineItem>& inlineItems ) {
		for ( const auto& acc : boxFragments ) {
			if ( !acc.valid || acc.box == nullptr )
				continue;

			Rectf bounds = acc.bounds;
			Rectf paintBounds = acc.contentValid ? acc.contentBounds : acc.bounds;
			const Rectf edges(
				acc.box->margin.Left + acc.box->padding.Left + acc.box->borderWidth,
				acc.box->margin.Top + acc.box->padding.Top + acc.box->borderWidth,
				acc.box->margin.Right + acc.box->padding.Right + acc.box->borderWidth,
				acc.box->margin.Bottom + acc.box->padding.Bottom + acc.box->borderWidth );
			if ( acc.startsInlineBox )
				bounds.Left -= edges.Left;
			if ( acc.endsInlineBox )
				bounds.Right += edges.Right;
			bounds.Top -= edges.Top;
			bounds.Bottom += edges.Bottom;

			if ( acc.startsInlineBox )
				paintBounds.Left -= acc.box->padding.Left + acc.box->borderWidth;
			if ( acc.endsInlineBox )
				paintBounds.Right += acc.box->padding.Right + acc.box->borderWidth;
			paintBounds.Top -= acc.box->padding.Top + acc.box->borderWidth;
			paintBounds.Bottom += acc.box->padding.Bottom + acc.box->borderWidth;

			RichText::InlineFragment fragment;
			fragment.type = RichText::InlineFragment::Type::Box;
			fragment.itemPath = acc.path;
			fragment.lineIndex = acc.lineIndex;
			fragment.bounds = bounds;
			fragment.paintBounds = paintBounds;
			fragment.baselineAlign = acc.box->baselineAlign;
			fragment.source = acc.box->source;
			fragment.startsInlineBox = acc.startsInlineBox;
			fragment.endsInlineBox = acc.endsInlineBox;
			fragment.backgroundColor = acc.box->backgroundColor;
			fragment.borderWidth = acc.box->borderWidth;
			fragment.borderColor = acc.box->borderColor;
			fragment.backgroundColorDrawable = acc.box->backgroundColorDrawable;
			fragment.backgroundDrawable = acc.box->backgroundDrawable;
			fragment.borderDrawable = acc.box->borderDrawable;
			fragment.backgroundDrawableUsesFragmentColor =
				acc.box->backgroundDrawableUsesFragmentColor;
			fragment.textDecoration = ( acc.box->textDecoration |
										inlineAncestorTextDecoration( inlineItems, acc.path ) ) &
									  ( Text::Underlined | Text::StrikeThrough );
			fragments.push_back( std::move( fragment ) );
		}
	}
};

} // namespace

template <typename TextType> static void setTextString( Text& text, TextType&& string ) {
	text.setString( std::forward<TextType>( string ) );
}

template <typename TextType>
void RichText::addSpanImpl( TextType&& text, const FontStyleConfig& style, const Rectf& margin,
							const Rectf& padding, Float lineHeight,
							const BaselineAlignValue& baselineAlign, InlineSource source ) {
	if ( text.empty() && margin == Rectf::Zero && padding == Rectf::Zero && lineHeight == 0 )
		return;

	const bool hasText = !text.empty();
	auto span = std::make_shared<Text>();
	setTextString( *span, std::forward<TextType>( text ) );
	span->setStyleConfig( style );
	span->setTabWidth( mTabWidth );

	InlineItem item;
	InlineItem::Box box;
	box.margin = margin;
	box.padding = padding;
	box.lineHeight = lineHeight;
	box.baselineAlign = baselineAlign;
	box.participatesInLineMetrics = false;
	box.contributesInlineSpacing = false;
	if ( hasText || margin != Rectf::Zero || padding != Rectf::Zero || lineHeight > 0 ) {
		InlineItem textItem;
		auto& run = textItem.asTextRun();
		run.text = span;
		run.source = source;
		run.margin = margin;
		run.padding = padding;
		run.lineHeight = lineHeight;
		run.baselineAlign = baselineAlign;
		box.children.push_back( std::move( textItem ) );
	}
	item.data = std::move( box );
	resolveInlinePath( mInlineItems, mInlinePath )->push_back( std::move( item ) );

	invalidateLayout();
}

void RichText::addSpan( const String& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign, InlineSource source ) {
	addSpanImpl( text, style, margin, padding, lineHeight, baselineAlign, source );
}

void RichText::addSpan( const String::View& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign, InlineSource source ) {
	addSpanImpl( text, style, margin, padding, lineHeight, baselineAlign, source );
}

void RichText::addSpan( String&& text, const FontStyleConfig& style, const Rectf& margin,
						const Rectf& padding, Float lineHeight,
						const BaselineAlignValue& baselineAlign, InlineSource source ) {
	addSpanImpl( std::move( text ), style, margin, padding, lineHeight, baselineAlign, source );
}

void RichText::addDrawable( std::shared_ptr<Drawable> drawable ) {
	if ( !drawable )
		return;
	InlineItem item;
	InlineItem::AtomicBox box;
	box.drawable = drawable;
	box.size = drawable->getPixelsSize();
	box.baseline = box.size.getHeight();
	item.data = std::move( box );
	resolveInlinePath( mInlineItems, mInlinePath )->push_back( std::move( item ) );
	invalidateLayout();
}

void RichText::addCustomSize(
	const Sizef& size, InlineFloat floatType, InlineClear clearType, Float baseline,
	const BaselineAlignValue& baselineAlign, InlineSource source, bool isBlock,
	bool isBlockFormattingContext,
	std::shared_ptr<const std::vector<FloatExclusion>> propagatedFloats ) {
	Float usedBaseline = baseline >= 0.f ? baseline : size.getHeight();

	InlineItem item;
	InlineItem::AtomicBox box;
	box.source = source;
	box.size = size;
	box.baseline = usedBaseline;
	box.floatType = floatType;
	box.clearType = clearType;
	box.isBlock = isBlock;
	box.isBlockFormattingContext = isBlockFormattingContext;
	box.propagatedFloats = std::move( propagatedFloats );
	box.baselineAlign = baselineAlign;
	item.data = std::move( box );
	resolveInlinePath( mInlineItems, mInlinePath )->push_back( std::move( item ) );

	invalidateLayout();
}

void RichText::addLineBreak() {
	InlineItem item;
	InlineItem::AtomicBox box;
	box.isLineBreak = true;
	item.data = std::move( box );
	resolveInlinePath( mInlineItems, mInlinePath )->push_back( std::move( item ) );

	invalidateLayout();
}

void RichText::pushInlineBox( const Rectf& margin, const Rectf& padding, Float lineHeight,
							  const BaselineAlignValue& baselineAlign, const Color& backgroundColor,
							  Float borderWidth, const Color& borderColor, Uint32 textDecoration,
							  InlineSource source, Drawable* backgroundColorDrawable,
							  Drawable* backgroundDrawable, Drawable* borderDrawable,
							  bool backgroundDrawableUsesFragmentColor ) {
	InlineItem item;
	InlineItem::Box box;
	box.source = source;
	box.margin = margin;
	box.padding = padding;
	box.lineHeight = lineHeight;
	box.baselineAlign = baselineAlign;
	box.backgroundColor = backgroundColor;
	box.borderWidth = borderWidth;
	box.borderColor = borderColor;
	box.backgroundColorDrawable = backgroundColorDrawable;
	box.backgroundDrawable = backgroundDrawable;
	box.borderDrawable = borderDrawable;
	box.backgroundDrawableUsesFragmentColor = backgroundDrawableUsesFragmentColor;
	box.textDecoration = textDecoration & ( Text::Underlined | Text::StrikeThrough );
	item.data = std::move( box );

	auto* target = resolveInlinePath( mInlineItems, mInlinePath );
	target->push_back( std::move( item ) );
	mInlinePath.push_back( target->size() - 1 );
}

void RichText::popInlineBox() {
	if ( !mInlinePath.empty() )
		mInlinePath.pop_back();
}

template <typename TextType>
void RichText::addInlineTextImpl( TextType&& text, const FontStyleConfig& style,
								  const Rectf& margin, const Rectf& padding, Float lineHeight,
								  const BaselineAlignValue& baselineAlign, InlineSource source ) {
	if ( text.empty() && margin == Rectf::Zero && padding == Rectf::Zero && lineHeight == 0 )
		return;

	InlineItem textItem;
	auto& run = textItem.asTextRun();
	run.text = std::make_shared<Text>();
	setTextString( *run.text, std::forward<TextType>( text ) );
	run.text->setStyleConfig( style );
	run.text->setTabWidth( mTabWidth );
	run.source = source;
	run.margin = margin;
	run.padding = padding;
	run.lineHeight = lineHeight;
	run.baselineAlign = baselineAlign;
	run.suppressBackground = true;

	resolveInlinePath( mInlineItems, mInlinePath )->push_back( std::move( textItem ) );
	invalidateLayout();
}

void RichText::addInlineText( const String& text, const FontStyleConfig& style, const Rectf& margin,
							  const Rectf& padding, Float lineHeight,
							  const BaselineAlignValue& baselineAlign, InlineSource source ) {
	addInlineTextImpl( text, style, margin, padding, lineHeight, baselineAlign, source );
}

void RichText::addInlineText( const String::View& text, const FontStyleConfig& style,
							  const Rectf& margin, const Rectf& padding, Float lineHeight,
							  const BaselineAlignValue& baselineAlign, InlineSource source ) {
	addInlineTextImpl( text, style, margin, padding, lineHeight, baselineAlign, source );
}

void RichText::addInlineText( String&& text, const FontStyleConfig& style, const Rectf& margin,
							  const Rectf& padding, Float lineHeight,
							  const BaselineAlignValue& baselineAlign, InlineSource source ) {
	addInlineTextImpl( std::move( text ), style, margin, padding, lineHeight, baselineAlign,
					   source );
}

void RichText::addInlineAtomicBox( const Sizef& size, InlineFloat floatType, InlineClear clearType,
								   Float baseline, bool isLineBreak,
								   const BaselineAlignValue& baselineAlign, InlineSource source ) {
	InlineItem item;
	InlineItem::AtomicBox box;
	box.source = source;
	box.size = size;
	box.baseline = baseline >= 0.f ? baseline : size.getHeight();
	box.floatType = floatType;
	box.clearType = clearType;
	box.isLineBreak = isLineBreak;
	box.baselineAlign = baselineAlign;
	item.data = std::move( box );

	resolveInlinePath( mInlineItems, mInlinePath )->push_back( std::move( item ) );
	invalidateLayout();
}

void RichText::addSpan( const String& text, const FontStyleConfig& style ) {
	addSpan( text, style, Rectf::Zero, Rectf::Zero );
}

void RichText::addSpan( const String::View& text, const FontStyleConfig& style ) {
	addSpan( text, style, Rectf::Zero, Rectf::Zero );
}

void RichText::addSpan( String&& text, const FontStyleConfig& style ) {
	addSpan( std::move( text ), style, Rectf::Zero, Rectf::Zero );
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
	mInlineItems.clear();
	mInlinePath.clear();
	mInlineFragments.clear();
	mLines.clear();
	mSelection = { 0, 0 };
	invalidateLayout();
}

void RichText::rebuildInlineFragments() {
	mInlineFragments = RichTextInlineLayouter::rebuildFragments( mInlineItems, mLines );
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

void RichText::setLineWrap( bool lineWrap ) {
	if ( mLineWrap != lineWrap ) {
		mLineWrap = lineWrap;
		invalidateLayout();
	}
}

void RichText::setWhiteSpaceWrapMode( WhiteSpaceWrapMode mode ) {
	if ( mWhiteSpaceWrapMode != mode ) {
		mWhiteSpaceWrapMode = mode;
		invalidateLayout();
	}
}

void RichText::setTabWidth( Uint32 tabWidth ) {
	tabWidth = eemax<Uint32>( 1, tabWidth );
	if ( mTabWidth != tabWidth ) {
		mTabWidth = tabWidth;
		setInlineItemTextTabWidth( mInlineItems, mTabWidth );
		invalidateLayout();
	}
}

bool RichText::setExternalFloatExclusions( const std::vector<FloatExclusion>& exclusions ) {
	if ( mExternalFloatExclusions == exclusions )
		return false;

	mExternalFloatExclusions = exclusions;
	invalidateLayout();
	return true;
}

void RichText::invalidate() {
	invalidateLayout();
	invalidateInlineItemText( mInlineItems );
}

Float RichText::getMinIntrinsicWidth() {
	return RichTextInlineLayouter::getMinIntrinsicWidth( mInlineItems, mWhiteSpaceWrapMode );
}

Float RichText::getMaxIntrinsicWidth() {
	return RichTextInlineLayouter::getMaxIntrinsicWidth( mInlineItems );
}

void RichText::updateLayout() {
	if ( !mNeedsLayoutUpdate )
		return;

	bool hasFloats = !mExternalFloatExclusions.empty() ||
					 RichTextInlineLayouter::needsFloatAwareLayout( mInlineItems );

	// ─── Inline layouter fast path: no floats or clears ─────────────
	if ( !hasFloats ) {
		auto result = RichTextInlineLayouter::layoutNoFloats( mInlineItems, mMaxWidth, mTextIndent,
															  mAlign, mLineHeight, mDefaultStyle,
															  mLineWrap, mWhiteSpaceWrapMode );
		mLines = std::move( result.lines );
		mLocalFloatExclusions.clear();
		mSize = result.size;
		mTotalCharacterCount = result.totalCharacterCount;
		rebuildInlineFragments();
		mNeedsLayoutUpdate = false;
		return;
	}

	auto result = RichTextInlineLayouter::layoutWithFloats(
		mInlineItems, mMaxWidth, mTextIndent, mAlign, mLineHeight, mDefaultStyle,
		mExternalFloatExclusions, mLocalFloatExclusions, mLineWrap, mWhiteSpaceWrapMode );
	mLines = std::move( result.lines );
	mSize = result.size;
	mTotalCharacterCount = result.totalCharacterCount;
	rebuildInlineFragments();
	mNeedsLayoutUpdate = false;
}

const std::vector<RichText::FloatExclusion>& RichText::getLocalFloatExclusions() {
	updateLayout();
	return mLocalFloatExclusions;
}

Sizef RichText::getSize() {
	updateLayout();
	return mSize;
}

void RichText::invalidateLayout() {
	mNeedsLayoutUpdate = true;
}

}} // namespace EE::Graphics
