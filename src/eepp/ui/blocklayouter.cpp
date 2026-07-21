#include <eepp/core/containers.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/ui/blocklayouter.hpp>
#include <eepp/ui/uihtmltable.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>

namespace EE { namespace UI {

static bool isStretchedFlexItem( UIHTMLWidget* widget ) {
	Node* parent = widget->getParent();
	if ( !parent || !parent->isWidget() || !parent->isType( UI_TYPE_HTML_WIDGET ) )
		return false;
	UIHTMLWidget* parentHtml = parent->asType<UIHTMLWidget>();
	if ( !parentHtml->isFlex() )
		return false;

	CSSAlignSelf alignSelf = widget->getAlignSelf();
	if ( alignSelf == CSSAlignSelf::Stretch )
		return true;

	if ( alignSelf == CSSAlignSelf::Auto )
		return parentHtml->getAlignItems() == CSSAlignItems::Stretch;

	return false;
}

static bool isTableCellInTableRow( UIWidget* widget ) {
	return widget && widget->isType( UI_TYPE_HTML_TABLE_CELL ) && widget->getParent() &&
		   widget->getParent()->isType( UI_TYPE_HTML_TABLE_ROW );
}

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

	Node* parentNode = widget->getParent();
	bool parentIsFlex = parentNode && parentNode->isType( UI_TYPE_HTML_WIDGET ) &&
						parentNode->asType<UIHTMLWidget>()->isFlex();
	bool parentIsGrid = parentNode && parentNode->isType( UI_TYPE_HTML_WIDGET ) &&
						parentNode->asType<UIHTMLWidget>()->isGrid();

	// Inline elements normally do not run their own block layout; the nearest parent RichText
	// stream formats and paints them. Flex/grid containers are the exception: CSS blockification
	// turns each child into an independent item, so an inline UITextSpan inside any flex/grid
	// parent, including inline-flex/inline-grid, still needs this layouter to build its own
	// RichText, measure its height, and position text. This is broader than the render ownership
	// guard in UIRichText/UITextSpan, which only applies to block-level flex/grid containers.
	if ( widget->isInline() && !parentIsFlex && !parentIsGrid )
		return;

	mResizedCount = 0;
	mPacking = true;

	mContainer->beginAttributesTransaction();

	bool preserveFloatConstrainedBFCWidth =
		widget->establishesBlockFormattingContext() && !rt->getExternalFloatExclusions().empty();
	bool preserveShrinkToFitFloatWidth = widget->getCSSFloat() != CSSFloat::None &&
										 widget->getLayoutWidthPolicy() == SizePolicy::MatchParent;
	if ( !preserveFloatConstrainedBFCWidth && !preserveShrinkToFitFloatWidth && !parentIsFlex )
		setMatchParentIfNeededVerticalGrowth();

	const StyleSheetProperty* prop = nullptr;
	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::Fixed && mContainer->getUIStyle() &&
		 ( prop = mContainer->getUIStyle()->getProperty( PropertyId::Width ) ) ) {
		mContainer->setInternalPixelsSize( { mContainer->cssWidthPropertyToBorderBoxWidth( *prop ),
											 mContainer->getPixelsSize().getHeight() } );
	}

	if ( !isTableCellInTableRow( mContainer ) &&
		 mContainer->getLayoutHeightPolicy() == SizePolicy::Fixed && mContainer->getUIStyle() &&
		 ( prop = mContainer->getUIStyle()->getProperty( PropertyId::Height ) ) ) {
		mContainer->setInternalPixelsSize(
			{ mContainer->getPixelsSize().getWidth(),
			  mContainer->cssHeightPropertyToBorderBoxHeight( *prop ) } );
	}

	UIRichText::rebuildRichText( widget, *rt );

	rt->updateLayout();

	positionRichTextChildren( rt );

	Sizef contentSize = rt->getSize();
	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent ||
		 mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		// RichText computes float placement from custom-block sizes captured before the
		// corresponding widgets are positioned and may be resized by nested layout. For the
		// intentional HTML-layer hit-test compatibility behavior, wrap-content parents must grow to
		// the final floated child bounds, including the parent's padding/content offset.
		const Rectf contentOffset = mContainer->getPixelsContentOffset();
		Node* child = mContainer->getFirstChild();
		while ( child != nullptr ) {
			if ( child->isWidget() && child->isType( UI_TYPE_HTML_WIDGET ) ) {
				auto* childWidget = child->asType<UIHTMLWidget>();
				if ( childWidget->isVisible() && !childWidget->isOutOfFlow() &&
					 childWidget->getCSSFloat() != CSSFloat::None ) {
					const Rectf margin = childWidget->getNormalFlowLayoutPixelsMargin();
					const Vector2f pos = childWidget->getPixelsPosition();
					const Sizef size = childWidget->getPixelsSize();
					contentSize.setWidth(
						std::max( contentSize.getWidth(), pos.x - margin.Left + size.getWidth() +
															  margin.Right - contentOffset.Left ) );
					contentSize.setHeight( std::max( contentSize.getHeight(),
													 pos.y - margin.Top + size.getHeight() +
														 margin.Bottom - contentOffset.Top ) );
				}
			}
			child = child->getNextNode();
		}
	}

	Float totW = mContainer->getPixelsSize().getWidth();
	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		totW = contentSize.getWidth() + mContainer->getPixelsContentOffset().Left +
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
			const Rectf contentOffset = mContainer->getPixelsContentOffset();
			totW = rt->getSize().getWidth() + contentOffset.Left + contentOffset.Right;
			if ( !mContainer->getMaxWidthEq().empty() &&
				 totW > mContainer->getMaxSizePx().getWidth() )
				mContainer->setClipType( ClipType::ContentBox );
		}
	}

	if ( totW != mContainer->getPixelsSize().getWidth() ||
		 ( mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent &&
		   !isStretchedFlexItem( widget ) ) )
		mContainer->setInternalPixelsWidth( totW );

	Float totH = mContainer->getPixelsSize().getHeight();
	if ( mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		totH = contentSize.getHeight() + mContainer->getPixelsContentOffset().Top +
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
			const Rectf contentOffset = mContainer->getPixelsContentOffset();
			totH = rt->getSize().getHeight() + contentOffset.Top + contentOffset.Bottom;
			if ( !mContainer->getMaxHeightEq().empty() &&
				 totH > mContainer->getMaxSizePx().getHeight() )
				mContainer->setClipType( ClipType::ContentBox );
		}
	}

	if ( totH != mContainer->getPixelsSize().getHeight() ||
		 ( mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent &&
		   !isStretchedFlexItem( widget ) ) )
		mContainer->setInternalPixelsHeight( totH );

	mContainer->endAttributesTransaction();

	if ( mResizedCount > 0 )
		positionRichTextChildren( rt );

	mPacking = false;
	mResizedCount = 0;
}

void BlockLayouter::positionRichTextChildren( Graphics::RichText* rt ) {
	const auto& lines = rt->getLines();
	const auto& fragments = rt->getInlineFragments();
	Node* child = mContainer->getFirstChild();
	const bool hasStructuredFragments = !fragments.empty();

	size_t currentLine = 0;
	size_t currentSpan = 0;

	auto getNextCustomSpan = [&]() -> const RichText::RenderSpan* {
		while ( currentLine < lines.size() ) {
			const auto& line = lines[currentLine];
			while ( currentSpan < line.spans.size() ) {
				const auto& span = line.spans[currentSpan];
				currentSpan++;
				if ( span.type != RichText::RenderSpan::Type::Text && !span.isLineBreak )
					return &span;
			}
			currentSpan = 0;
			currentLine++;
		}
		return nullptr;
	};

	Int64 curCharIdx = 0;
	const Rectf contentOffset = mContainer->getPixelsContentOffset();

	for ( auto& bucket : mTextNodeFragments )
		bucket.second.clear();
	for ( auto& bucket : mWidgetFragments )
		bucket.second.clear();
	mTextNodeFragments.clear();
	mWidgetFragments.clear();
	mTextNodeFragments.reserve( fragments.size() );
	mWidgetFragments.reserve( fragments.size() );
	for ( const auto& fragment : fragments ) {
		if ( fragment.source.ptr == nullptr )
			continue;

		auto* bucket = fragment.source.type == RichText::InlineSourceType::TextNode
						   ? &mTextNodeFragments[fragment.source.ptr]
					   : fragment.source.type == RichText::InlineSourceType::Widget
						   ? &mWidgetFragments[fragment.source.ptr]
						   : nullptr;
		if ( bucket == nullptr )
			continue;

		switch ( fragment.type ) {
			case RichText::InlineFragment::Type::TextRun:
				bucket->textRuns.push_back( &fragment );
				break;
			case RichText::InlineFragment::Type::Box:
				bucket->boxes.push_back( &fragment );
				break;
			case RichText::InlineFragment::Type::AtomicBox:
				bucket->atomicBoxes.push_back( &fragment );
				break;
		}
	}

	auto toContainerBounds = [&]( const Rectf& bounds ) {
		return Rectf( contentOffset.Left + bounds.Left, contentOffset.Top + bounds.Top,
					  contentOffset.Left + bounds.Right, contentOffset.Top + bounds.Bottom );
	};

	auto hasValidBounds = []( const Rectf& bounds ) {
		return bounds.Left <= bounds.Right && bounds.Top <= bounds.Bottom;
	};

	auto isFloatingWidget = []( UIWidget* widget ) {
		return widget->isType( UI_TYPE_HTML_WIDGET ) &&
			   widget->asType<UIHTMLWidget>()->getCSSFloat() != CSSFloat::None;
	};

	auto establishesBlockFormattingContext = []( UIWidget* widget ) {
		return widget->isType( UI_TYPE_HTML_WIDGET ) &&
			   widget->asType<UIHTMLWidget>()->establishesBlockFormattingContext();
	};

	auto toRichTextFloat = []( CSSFloat val ) {
		switch ( val ) {
			case CSSFloat::Left:
				return RichText::InlineFloat::Left;
			case CSSFloat::Right:
				return RichText::InlineFloat::Right;
			case CSSFloat::None:
			default:
				return RichText::InlineFloat::None;
		}
	};

	auto buildExternalFloatExclusions = [&]( UIWidget* widget, const Vector2f& targetPos ) {
		std::vector<RichText::FloatExclusion> exclusions;
		Rectf childContentOffset = widget->getPixelsContentOffset();
		Vector2f richTextOrigin( targetPos.x + childContentOffset.Left,
								 targetPos.y + childContentOffset.Top );
		Float childContentHeight = widget->getPixelsSize().getHeight() - childContentOffset.Top -
								   childContentOffset.Bottom;
		Float childContentWidth =
			widget->getPixelsSize().getWidth() - childContentOffset.Left - childContentOffset.Right;
		bool filtersHorizontally =
			widget->getLayoutWidthPolicy() != SizePolicy::MatchParent && childContentWidth > 0;

		auto addExclusionIfRelevant = [&]( Rectf rect, RichText::InlineFloat type ) {
			if ( rect.Bottom <= 0 )
				return;
			if ( childContentHeight > 0 && rect.Top >= childContentHeight )
				return;
			if ( filtersHorizontally && ( rect.Right <= 0 || rect.Left >= childContentWidth ) )
				return;
			exclusions.push_back( { rect, type } );
		};

		if ( mContainer->isType( UI_TYPE_HTML_WIDGET ) ) {
			if ( RichText* containerRt = mContainer->asType<UIHTMLWidget>()->getRichTextPtr() ) {
				for ( const auto& inherited : containerRt->getExternalFloatExclusions() ) {
					Rectf rect = inherited.rect;
					rect.move( { contentOffset.Left, contentOffset.Top } );
					rect.move( -richTextOrigin );
					addExclusionIfRelevant( rect, inherited.type );
				}
			}
		}

		for ( const auto& bucket : mWidgetFragments ) {
			if ( bucket.first == widget )
				continue;

			auto* floatWidget = static_cast<UIWidget*>( bucket.first );
			if ( floatWidget == nullptr || !floatWidget->isType( UI_TYPE_HTML_WIDGET ) )
				continue;

			CSSFloat floatType = floatWidget->asType<UIHTMLWidget>()->getCSSFloat();
			if ( floatType == CSSFloat::None )
				continue;

			for ( const auto* fragment : bucket.second.atomicBoxes ) {
				Rectf rect = toContainerBounds( fragment->bounds );
				rect.move( -richTextOrigin );
				addExclusionIfRelevant( rect, toRichTextFloat( floatType ) );
			}
		}

		return exclusions;
	};

	auto applyExternalFloatExclusions = [&]( UIWidget* widget, const Vector2f& targetPos ) {
		if ( !widget->isType( UI_TYPE_HTML_WIDGET ) )
			return;

		auto* htmlWidget = widget->asType<UIHTMLWidget>();
		RichText* childRt = htmlWidget->getRichTextPtr();
		if ( childRt == nullptr )
			return;

		std::vector<RichText::FloatExclusion> exclusions;
		if ( htmlWidget->getCSSFloat() == CSSFloat::None && !htmlWidget->isOutOfFlow() )
			exclusions = buildExternalFloatExclusions( widget, targetPos );

		if ( childRt->setExternalFloatExclusions( exclusions ) ) {
			if ( htmlWidget->getLayouter() )
				htmlWidget->getLayouter()->updateLayout();
			mResizedCount++;
		}
	};

	auto expandBounds = [&]( Rectf& bounds, bool& valid, const Rectf& rect ) {
		if ( !valid ) {
			bounds = rect;
			valid = true;
		} else {
			bounds.expand( rect );
		}
	};

	auto getTextNodeFragmentBounds = [&]( UITextNode* textNode, Rectf& outBounds ) {
		auto it = mTextNodeFragments.find( textNode );
		if ( it == mTextNodeFragments.end() )
			return false;

		bool valid = false;
		for ( const auto* fragment : it->second.textRuns )
			expandBounds( outBounds, valid, toContainerBounds( fragment->bounds ) );
		return valid;
	};

	auto getWidgetFragmentBounds = [&]( UIWidget* widget, Rectf& outBounds,
										SpanHitBoxes* hitBoxes ) {
		auto it = mWidgetFragments.find( widget );
		if ( it == mWidgetFragments.end() )
			return false;

		bool valid = false;
		for ( const auto* fragment : it->second.boxes ) {
			Rectf hb = toContainerBounds( fragment->bounds );
			if ( hitBoxes )
				hitBoxes->push_back( hb );
			expandBounds( outBounds, valid, hb );
		}
		return valid;
	};

	auto getAtomicWidgetFragmentBounds = [&]( UIWidget* widget, Rectf& outBounds ) {
		auto it = mWidgetFragments.find( widget );
		if ( it == mWidgetFragments.end() )
			return false;

		bool valid = false;
		for ( const auto* fragment : it->second.atomicBoxes )
			expandBounds( outBounds, valid, toContainerBounds( fragment->bounds ) );
		return valid;
	};

	auto processNode = [&]( Node* node, auto& processNodeRef ) -> Rectf {
		constexpr Float maxF = std::numeric_limits<Float>::max();
		constexpr Float lowF = std::numeric_limits<Float>::lowest();
		Rectf bounds( maxF, maxF, lowF, lowF );

		if ( !node->isVisible() )
			return bounds;

		if ( node->isTextNode() ) {
			auto* tn = static_cast<UITextNode*>( node );
			Int64 startChar = curCharIdx;
			curCharIdx += tn->getLayoutCharCount();
			Int64 endChar = curCharIdx;

			if ( startChar < endChar && !lines.empty() ) {
				Rectf textBounds( maxF, maxF, lowF, lowF );

				Vector2f offset;
				Node* p = node->getParent();
				while ( p && p != mContainer ) {
					offset += p->isWidget() ? p->asType<UIWidget>()->getPixelsPosition()
											: p->getPosition();
					p = p->getParent();
				}

				bool hasFragments = getTextNodeFragmentBounds( tn, textBounds );
				if ( !hasFragments && !hasStructuredFragments ) {
					for ( const auto& line : lines ) {
						bool passedText = false;
						for ( const auto& rspan : line.spans ) {
							if ( rspan.startCharIndex >= startChar &&
								 rspan.endCharIndex <= endChar ) {
								Rectf hb( contentOffset.Left + rspan.position.x,
										  contentOffset.Top + line.y + rspan.position.y,
										  contentOffset.Left + rspan.position.x +
											  rspan.size.getWidth(),
										  contentOffset.Top + line.y + rspan.position.y +
											  rspan.size.getHeight() );
								textBounds.expand( hb );
							} else if ( rspan.startCharIndex >= endChar ) {
								passedText = true;
								break;
							}
						}
						if ( passedText )
							break;
					}
				}

				if ( hasValidBounds( textBounds ) ) {
					tn->setPixelsPosition( textBounds.getPosition() - offset );
					tn->setPixelsSize( textBounds.getSize() );
					return textBounds;
				}
			}

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

		bool handled = false;

		if ( widget->isType( UI_TYPE_HTML_WIDGET ) && widget->asType<UIHTMLWidget>()->isInline() ) {
			UITextSpan* textSpan = widget->asType<UITextSpan>();
			Int64 startChar = curCharIdx;
			Int64 endChar = curCharIdx;

			Int64 layoutCount = textSpan->getLayoutCharCount();
			if ( layoutCount > 0 ) {
				endChar += layoutCount;
				curCharIdx = endChar;
			}

			auto& hitBoxes = textSpan->getHitBoxes();
			hitBoxes.clear();

			bool hasWidgetFragments = getWidgetFragmentBounds( widget, bounds, &hitBoxes );

			if ( !hasWidgetFragments && !hasStructuredFragments && startChar < endChar ) {
				for ( const auto& line : lines ) {
					bool passedText = false;
					for ( const auto& rspan : line.spans ) {
						if ( rspan.startCharIndex >= startChar && rspan.endCharIndex <= endChar ) {
							Rectf hb( contentOffset.Left + rspan.position.x,
									  contentOffset.Top + line.y + rspan.position.y,
									  contentOffset.Left + rspan.position.x + rspan.size.getWidth(),
									  contentOffset.Top + line.y + rspan.position.y +
										  rspan.size.getHeight() );

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

			Node* spanChild = widget->getFirstChild();
			while ( spanChild != NULL ) {
				if ( spanChild->isWidget() )
					bounds.expand( processNodeRef( spanChild, processNodeRef ) );
				spanChild = spanChild->getNextNode();
			}

			if ( textSpan->isInlineBlock() ) {
				Rectf pad = textSpan->getPixelsPadding();
				bounds.Left -= pad.Left;
				bounds.Top -= pad.Top;
				bounds.Right += pad.Right;
				bounds.Bottom += pad.Bottom;
				for ( auto& hb : hitBoxes ) {
					hb.Left -= pad.Left;
					hb.Top -= pad.Top;
					hb.Right += pad.Right;
					hb.Bottom += pad.Bottom;
				}
			}

			if ( hasValidBounds( bounds ) ) {
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

			handled = true;
		}

		if ( !handled ) {
			if ( widget->isType( UI_TYPE_BR ) ) {
				curCharIdx += 1;
				Vector2f pos;
				if ( widget->getPrevNode() && widget->getPrevNode()->isWidget() ) {
					pos = widget->getPrevNode()->asType<UIWidget>()->getPixelsPosition();
					pos.y += widget->getPrevNode()->getPixelsSize().getHeight();
				}
				widget->setPixelsPosition( pos );
				widget->setPixelsSize(
					{ eemax( 0.f, mContainer->getPixelsSize().getWidth() -
									  mContainer->getPixelsContentOffset().Left -
									  mContainer->getPixelsContentOffset().Right ),
					  0 } );
			} else {
				curCharIdx += 1;
				Rectf atomicBounds( maxF, maxF, lowF, lowF );
				if ( getAtomicWidgetFragmentBounds( widget, atomicBounds ) ) {
					if ( widget->hasLayoutMarginAuto() )
						widget->updateLayoutMarginAuto();
					Rectf margin =
						widget->isType( UI_TYPE_HTML_WIDGET )
							? widget->asType<UIHTMLWidget>()->getNormalFlowLayoutPixelsMargin()
							: widget->getLayoutPixelsMargin();
					Vector2f targetPos( atomicBounds.Left + margin.Left,
										atomicBounds.Top + margin.Top );

					widget->setPixelsPosition( targetPos - offset );
					if ( establishesBlockFormattingContext( widget ) &&
						 widget->getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
						Float contentWidth =
							eemax( 0.f, atomicBounds.getWidth() - margin.Left - margin.Right );
						if ( contentWidth + 0.01f < widget->getPixelsSize().getWidth() ) {
							widget->setPixelsSize( contentWidth,
												   widget->getPixelsSize().getHeight() );
							mResizedCount++;
						}
					}
					applyExternalFloatExclusions( widget, targetPos );
					if ( !isFloatingWidget( widget ) &&
						 widget->getLayoutWidthPolicy() == SizePolicy::MatchParent &&
						 mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
						Float contentWidth = eemax(
							0.f, mContainer->getPixelsSize().getWidth() - contentOffset.Left -
									 contentOffset.Right - margin.Left - margin.Right );
						if ( widget->getPixelsSize().getWidth() == 0 && contentWidth > 0 ) {
							widget->setPixelsSize( contentWidth,
												   widget->getPixelsSize().getHeight() );
							mResizedCount++;
						}
					}
					bounds = Rectf( targetPos, atomicBounds.getSize() );
				} else if ( !hasStructuredFragments ) {
					const auto* span = getNextCustomSpan();
					if ( span == nullptr )
						return bounds;

					size_t lineIdx = currentSpan > 0 ? currentLine : currentLine - 1;
					Float lineY = lines[lineIdx].y;
					if ( widget->hasLayoutMarginAuto() )
						widget->updateLayoutMarginAuto();
					Rectf margin =
						widget->isType( UI_TYPE_HTML_WIDGET )
							? widget->asType<UIHTMLWidget>()->getNormalFlowLayoutPixelsMargin()
							: widget->getLayoutPixelsMargin();

					Vector2f targetPos( contentOffset.Left + span->position.x + margin.Left,
										contentOffset.Top + lineY + span->position.y + margin.Top );

					widget->setPixelsPosition( targetPos - offset );
					if ( establishesBlockFormattingContext( widget ) &&
						 widget->getLayoutWidthPolicy() == SizePolicy::MatchParent ) {
						Float contentWidth =
							eemax( 0.f, span->size.getWidth() - margin.Left - margin.Right );
						if ( contentWidth + 0.01f < widget->getPixelsSize().getWidth() ) {
							widget->setPixelsSize( contentWidth,
												   widget->getPixelsSize().getHeight() );
							mResizedCount++;
						}
					}
					applyExternalFloatExclusions( widget, targetPos );
					if ( !isFloatingWidget( widget ) &&
						 widget->getLayoutWidthPolicy() == SizePolicy::MatchParent &&
						 mContainer->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
						// Stretch match-parent children only after the wrap-content parent has its
						// final used width. During RichText measurement this width may still be a
						// stale pre-style value, so using it there would poison shrink-to-fit
						// inline-block sizing.
						Float contentWidth =
							eemax( 0.f, mContainer->getPixelsSize().getWidth() -
											mContainer->getPixelsContentOffset().Left -
											mContainer->getPixelsContentOffset().Right -
											margin.Left - margin.Right );
						if ( widget->getPixelsSize().getWidth() == 0 && contentWidth > 0 ) {
							widget->setPixelsSize( contentWidth,
												   widget->getPixelsSize().getHeight() );
							mResizedCount++;
						}
					}

					bounds = Rectf( targetPos, span->size );
				}
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
