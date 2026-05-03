#include <eepp/graphics/richtext.hpp>
#include <eepp/ui/blocklayouter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>

namespace EE { namespace UI {

Float BlockLayouter::getMinIntrinsicWidth() {
	computeIntrinsicWidths();
	return mMinIntrinsicWidth;
}

Float BlockLayouter::getMaxIntrinsicWidth() {
	computeIntrinsicWidths();
	return mMaxIntrinsicWidth;
}

void BlockLayouter::computeIntrinsicWidths() {
	if ( !mContainer->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	auto* widget = mContainer->asType<UIHTMLWidget>();
	auto* rt = widget->getRichTextPtr();
	if ( rt == nullptr )
		return;

	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::Fixed ) {
		// Do nothing here, UIWidget handles fixed width.
		return;
	}

	if ( mIntrinsicWidthsDirty ) {
		RichText tmpRt( *rt );
		UIRichText::rebuildRichText( widget, tmpRt, UIRichText::IntrinsicMode::Min );
		mMinIntrinsicWidth = tmpRt.getMinIntrinsicWidth() +
							 mContainer->getPixelsContentOffset().Left +
							 mContainer->getPixelsContentOffset().Right;
		UIRichText::rebuildRichText( widget, tmpRt, UIRichText::IntrinsicMode::Max );
		mMaxIntrinsicWidth = tmpRt.getMaxIntrinsicWidth() +
							 mContainer->getPixelsContentOffset().Left +
							 mContainer->getPixelsContentOffset().Right;
		mIntrinsicWidthsDirty = false;
	}
}

void BlockLayouter::updateLayout() {
	if ( !mContainer->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	auto* widget = mContainer->asType<UIHTMLWidget>();
	auto* rt = widget->getRichTextPtr();
	if ( rt == nullptr || mPacking )
		return;
	mResizedCount = 0;
	mPacking = true;

	mContainer->beginAttributesTransaction();

	setMatchParentIfNeededVerticalGrowth();

	const StyleSheetProperty* prop = nullptr;
	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::Fixed && mContainer->getUIStyle() &&
		 ( prop = mContainer->getUIStyle()->getProperty( PropertyId::Width ) ) ) {
		mContainer->setInternalPixelsSize(
			{ mContainer->lengthFromValue( *prop ), mContainer->getPixelsSize().getHeight() } );
	}

	if ( mContainer->getLayoutHeightPolicy() == SizePolicy::Fixed && mContainer->getUIStyle() &&
		 ( prop = mContainer->getUIStyle()->getProperty( PropertyId::Height ) ) ) {
		mContainer->setInternalPixelsSize(
			{ mContainer->getPixelsSize().getWidth(), mContainer->lengthFromValue( *prop ) } );
	}

	UIRichText::rebuildRichText( widget, *rt );

	rt->updateLayout();

	positionRichTextChildren( rt );

	Float totW = mContainer->getPixelsSize().getWidth();
	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		totW = rt->getSize().getWidth() + mContainer->getPixelsContentOffset().Left +
			   mContainer->getPixelsContentOffset().Right;
		if ( !mContainer->getMaxWidthEq().empty() && totW > mContainer->getMaxSizePx().getWidth() )
			mContainer->setClipType( ClipType::ContentBox );
	} else if ( mContainer->getLayoutWidthPolicy() == SizePolicy::Fixed &&
				mContainer->getUIStyle() ) {
		const StyleSheetProperty* wprop =
			mContainer->getUIStyle()->getProperty( PropertyId::Width );
		if ( wprop && StyleSheetLength::isPercentage( wprop->value() ) && mContainer->getParent() &&
			 mContainer->getParent()->isWidget() &&
			 mContainer->getParent()->asType<UIWidget>()->getLayoutWidthPolicy() ==
				 SizePolicy::WrapContent ) {
			totW = rt->getSize().getWidth() + mContainer->getPixelsContentOffset().Left +
				   mContainer->getPixelsContentOffset().Right;
			if ( !mContainer->getMaxWidthEq().empty() &&
				 totW > mContainer->getMaxSizePx().getWidth() )
				mContainer->setClipType( ClipType::ContentBox );
		}
	}

	if ( totW != mContainer->getPixelsSize().getWidth() ||
		 mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent )
		mContainer->setInternalPixelsWidth( totW );

	Float totH = mContainer->getPixelsSize().getHeight();
	if ( mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		totH = rt->getSize().getHeight() + mContainer->getPixelsContentOffset().Top +
			   mContainer->getPixelsContentOffset().Bottom;
		if ( !mContainer->getMaxHeightEq().empty() &&
			 totH > mContainer->getMaxSizePx().getHeight() )
			mContainer->setClipType( ClipType::ContentBox );
	} else if ( mContainer->getLayoutHeightPolicy() == SizePolicy::Fixed &&
				mContainer->getUIStyle() ) {
		const StyleSheetProperty* hprop =
			mContainer->getUIStyle()->getProperty( PropertyId::Height );
		if ( hprop && StyleSheetLength::isPercentage( hprop->value() ) && mContainer->getParent() &&
			 mContainer->getParent()->isWidget() &&
			 mContainer->getParent()->asType<UIWidget>()->getLayoutHeightPolicy() ==
				 SizePolicy::WrapContent ) {
			totH = rt->getSize().getHeight() + mContainer->getPixelsContentOffset().Top +
				   mContainer->getPixelsContentOffset().Bottom;
			if ( !mContainer->getMaxHeightEq().empty() &&
				 totH > mContainer->getMaxSizePx().getHeight() )
				mContainer->setClipType( ClipType::ContentBox );
		}
	}

	if ( totH != mContainer->getPixelsSize().getHeight() ||
		 mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent )
		mContainer->setInternalPixelsHeight( totH );

	mContainer->endAttributesTransaction();

	if ( mResizedCount > 0 )
		positionRichTextChildren( rt );

	mPacking = false;
	mResizedCount = 0;
}

void BlockLayouter::positionRichTextChildren( Graphics::RichText* rt ) {
	const auto& lines = rt->getLines();
	Node* child = mContainer->getFirstChild();

	size_t currentLine = 0;
	size_t currentSpan = 0;

	auto getNextCustomSpan = [&]() -> const RichText::RenderSpan* {
		while ( currentLine < lines.size() ) {
			const auto& line = lines[currentLine];
			while ( currentSpan < line.spans.size() ) {
				const auto& span = line.spans[currentSpan];
				currentSpan++;
				if ( std::holds_alternative<RichText::CustomBlock>( span.block ) )
					return &span;
			}
			currentSpan = 0;
			currentLine++;
		}
		return nullptr;
	};

	Int64 curCharIdx = 0;

	auto processNode = [&]( Node* node, auto& processNodeRef ) -> Rectf {
		constexpr Float maxF = std::numeric_limits<Float>::max();
		constexpr Float lowF = std::numeric_limits<Float>::lowest();
		Rectf bounds( maxF, maxF, lowF, lowF );

		if ( !node->isVisible() )
			return bounds;

		// UITextNode is a logical marker; its text is rendered by the
		// RichText engine — just advance the character index and return
		// empty bounds so it does not affect any widget's geometry.
		if ( node->isTextNode() ) {
			curCharIdx += static_cast<UITextNode*>( node )->getText().length();
			return bounds;
		}

		if ( !node->isWidget() )
			return bounds;

		UIWidget* widget = node->asType<UIWidget>();

		// Accumulate ancestor positions so the widget can be placed
		// relative to the container (mContainer).
		Vector2f offset;
		Node* p = widget->getParent();
		while ( p && p != mContainer ) {
			offset += p->isWidget() ? p->asType<UIWidget>()->getPixelsPosition() : p->getPosition();
			p = p->getParent();
		}

		if ( widget->isType( UI_TYPE_HTML_WIDGET ) &&
			 widget->asType<UIHTMLWidget>()->isMergeable() ) {
			UITextSpan* textSpan = widget->asType<UITextSpan>();
			Int64 startChar = curCharIdx;
			Int64 endChar = curCharIdx;
			if ( !textSpan->getText().empty() ) {
				endChar += textSpan->getText().length();
				curCharIdx = endChar;
			}

			auto& hitBoxes = textSpan->getHitBoxes();
			hitBoxes.clear();

			if ( startChar < endChar ) {
				for ( const auto& line : lines ) {
					bool passedText = false;
					for ( const auto& rspan : line.spans ) {
						if ( rspan.startCharIndex >= startChar && rspan.endCharIndex <= endChar ) {
							Rectf hb( mContainer->getPixelsContentOffset().Left + rspan.position.x,
									  mContainer->getPixelsContentOffset().Top + line.y +
										  rspan.position.y,
									  mContainer->getPixelsContentOffset().Left + rspan.position.x +
										  rspan.size.getWidth(),
									  mContainer->getPixelsContentOffset().Top + line.y +
										  rspan.position.y + rspan.size.getHeight() );

							hitBoxes.push_back( hb );
							bounds.expand( hb );
						} else if ( rspan.startCharIndex > endChar ) {
							passedText = true;
							break;
						}
					}
					if ( passedText )
						break;
				}
			}

			// Recurse into children.  UITextNode children advance
			// curCharIdx but contribute no geometry (they are logical
			// markers only).  Widget children get their own position
			// and hit-boxes.
			Node* spanChild = widget->getFirstChild();
			while ( spanChild != NULL ) {
				if ( spanChild->isWidget() )
					bounds.expand( processNodeRef( spanChild, processNodeRef ) );
				spanChild = spanChild->getNextNode();
			}

			if ( bounds.Left <= bounds.Right && bounds.Top <= bounds.Bottom ) {
				Vector2f boundsPos = bounds.getPosition();

				widget->setPixelsPosition( boundsPos - offset );
				if ( bounds.getSize() != widget->getPixelsSize() ) {
					widget->setPixelsSize( bounds.getSize() );
					mResizedCount++;
				}

				for ( auto& hb : hitBoxes )
					hb.move( -boundsPos );

			} else {
				hitBoxes.clear();
			}

		} else if ( widget->isType( UI_TYPE_BR ) ) {
			curCharIdx += 1;
			Vector2f pos;
			if ( widget->getPrevNode() && widget->getPrevNode()->isWidget() ) {
				pos = widget->getPrevNode()->asType<UIWidget>()->getPixelsPosition();
				pos.y += widget->getPrevNode()->getPixelsSize().getHeight();
			}
			widget->setPixelsPosition( pos );
			widget->setPixelsSize( { eemax( 0.f, mContainer->getPixelsSize().getWidth() -
													 mContainer->getPixelsContentOffset().Left -
													 mContainer->getPixelsContentOffset().Right ),
									 0 } );
		} else {
			curCharIdx += 1;
			const auto* span = getNextCustomSpan();
			if ( span ) {
				size_t lineIdx = currentSpan > 0 ? currentLine : currentLine - 1;
				Float lineY = lines[lineIdx].y;
				Rectf margin = widget->getLayoutPixelsMargin();

				Vector2f targetPos( mContainer->getPixelsContentOffset().Left + span->position.x +
										margin.Left,
									mContainer->getPixelsContentOffset().Top + lineY +
										span->position.y + margin.Top );

				widget->setPixelsPosition( targetPos - offset );

				bounds = Rectf( targetPos, span->size );
			}
		}
		return bounds;
	};

	child = mContainer->getFirstChild();
	while ( NULL != child ) {
		bool isOutOfFlow =
			child->isType( UI_TYPE_HTML_WIDGET ) && child->asType<UIHTMLWidget>()->isOutOfFlow();
		if ( !isOutOfFlow )
			processNode( child, processNode );
		child = child->getNextNode();
	}
}

}} // namespace EE::UI
