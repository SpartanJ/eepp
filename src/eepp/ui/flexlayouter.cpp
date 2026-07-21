#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/flexlayouter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

void FlexLayouter::collectFlexItems( SmallVector<FlexItem, 16>& items ) {
	items.clear();

	Node* child = mContainer->getFirstChild();
	while ( child ) {
		if ( !child->isWidget() ) {
			child = child->getNextNode();
			continue;
		}

		UIWidget* widget = child->asType<UIWidget>();

		// Check for visibility:collapse before the isVisible() test,
		// since isVisible() returns false for both hidden and collapse.
		bool isCollapsed = false;
		if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
			isCollapsed =
				widget->asType<UIHTMLWidget>()->getVisibility() == CSSVisibility::Collapse;
		} else {
			// Non-HTML widgets (e.g. UITextNode): check via style property
			if ( widget->getUIStyle() ) {
				const auto* visProp = widget->getUIStyle()->getProperty( PropertyId::Visibility );
				if ( visProp )
					isCollapsed = String::iequals( visProp->value(), "collapse" );
			}
		}

		if ( !widget->isVisible() && !isCollapsed ) {
			child = child->getNextNode();
			continue;
		}

		if ( widget->isType( UI_TYPE_TEXTNODE ) ) {
			auto* textNode = widget->asType<UITextNode>();
			if ( textNode && textNode->isWhitespaceOnly() ) {
				child = child->getNextNode();
				continue;
			}
		}

		if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
			auto* htmlWidget = widget->asType<UIHTMLWidget>();

			if ( htmlWidget->getDisplay() == CSSDisplay::None || htmlWidget->isOutOfFlow() ) {
				child = child->getNextNode();
				continue;
			}
		}

		FlexItem item;
		item.widget = widget;
		item.collapsed = isCollapsed;
		readItemStyle( widget, item );

		items.push_back( item );
		child = child->getNextNode();
	}

	std::sort( items.begin(), items.end(),
			   []( const FlexItem& a, const FlexItem& b ) { return a.order < b.order; } );
}

FlexLayouter::Axis FlexLayouter::getMainAxis( CSSFlexDirection direction ) const {
	switch ( direction ) {
		case CSSFlexDirection::Row:
			return { true, false };
		case CSSFlexDirection::RowReverse:
			return { true, true };
		case CSSFlexDirection::Column:
			return { false, false };
		case CSSFlexDirection::ColumnReverse:
			return { false, true };
	}
	return { true, false };
}

FlexLayouter::Axis FlexLayouter::getCrossAxis( CSSFlexDirection direction ) const {
	switch ( direction ) {
		case CSSFlexDirection::Row:
		case CSSFlexDirection::RowReverse:
			return { false, false };
		case CSSFlexDirection::Column:
		case CSSFlexDirection::ColumnReverse:
			return { true, false };
	}
	return { false, false };
}

namespace {

static Float getItemBaselineOffset( UIWidget* widget ) {
	if ( widget->isType( UI_TYPE_HTML_WIDGET ) )
		return widget->asType<UIHTMLWidget>()->getBaseline();
	if ( widget->isType( UI_TYPE_TEXTNODE ) )
		return widget->asType<UITextNode>()->getBaseline();
	return 0.f;
}

bool getFontStyleFromAncestor( UIWidget* widget, FontStyleConfig& outConfig ) {
	Node* n = widget;
	while ( n ) {
		if ( n->isType( UI_TYPE_RICHTEXT ) ) {
			outConfig = n->asType<UIRichText>()->getRichText().getFontStyleConfig();
			return outConfig.Font != nullptr;
		}
		n = n->getParent();
	}
	// Fallback: try the scene node's default font
	UISceneNode* sceneNode = widget->getUISceneNode();
	if ( sceneNode ) {
		Font* defaultFont = sceneNode->getUIThemeManager()->getDefaultFont();
		if ( defaultFont ) {
			outConfig = FontStyleConfig();
			outConfig.Font = defaultFont;
			outConfig.CharacterSize = 12;
			return true;
		}
	}
	return false;
}

static void updateChildLayoutIfDirty( UIWidget* widget, const UILayouter* owner ) {
	if ( !widget->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	auto* childHtml = widget->asType<UIHTMLWidget>();
	auto* layouter = childHtml->getLayouter();
	if ( layouter && layouter != owner && !layouter->isPacking() && childHtml->isLayoutDirty() )
		childHtml->updateLayout();
}

} // namespace

void FlexLayouter::readContainerStyle( CSSFlexDirection& direction, CSSFlexWrap& wrap,
									   CSSJustifyContent& justify, CSSAlignItems& alignItems,
									   CSSAlignContent& alignContent, Float& columnGap,
									   Float& rowGap ) {
	if ( !mContainer->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	auto* widget = mContainer->asType<UIHTMLWidget>();
	direction = widget->getFlexDirection();
	wrap = widget->getFlexWrap();
	justify = widget->getJustifyContent();
	alignItems = widget->getAlignItems();
	alignContent = widget->getAlignContent();

	columnGap = mContainer->lengthFromValue(
		widget->getColumnGap(), CSS::PropertyRelativeTarget::ContainingBlockWidth, 0.f );
	rowGap = mContainer->lengthFromValue( widget->getRowGap(),
										  CSS::PropertyRelativeTarget::ContainingBlockWidth, 0.f );
}

void FlexLayouter::readItemStyle( UIWidget* child, FlexItem& item ) {
	if ( !child->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	auto* widget = child->asType<UIHTMLWidget>();
	item.flexGrow = widget->getFlexGrow();
	item.flexShrink = widget->getFlexShrink();

	std::string val = widget->getFlexBasis();
	String::toLowerInPlace( val );
	item.flexBasisRaw = val;
	if ( val == "auto" || val == "content" ) {
		item.flexBasisAuto = true;
		item.flexBasisContent = ( val == "content" );
		item.flexBasisValue = 0.f;
		item.flexBasisIsPercentage = false;
	} else {
		item.flexBasisAuto = false;
		item.flexBasisIsPercentage = val.find( '%' ) != std::string::npos;
		if ( item.flexBasisIsPercentage ) {
			// Store the raw percentage factor, resolve later in
			// measureFlexItems against the flex container's inner main size.
			item.flexBasisValue = 0.f;
		} else {
			Axis mainAxis = getMainAxis( mDirection );
			Float resolved = child->lengthFromValue( val, CSS::PropertyRelativeTarget::None, 0.f );
			item.flexBasisValue = mainAxis.horizontal
									  ? child->cssResolvedLengthToBorderBoxWidth( resolved )
									  : child->cssResolvedLengthToBorderBoxHeight( resolved );
		}
	}

	item.alignSelf = widget->getAlignSelf();
	item.order = widget->getOrder();
}

Float FlexLayouter::resolveFlexBasis( UIWidget* child, CSSFlexDirection, Float flexBasisValue,
									  bool flexBasisAuto, const Axis& mainAxis,
									  bool flexBasisContent ) {
	if ( flexBasisAuto ) {
		// For text nodes (anonymous flex items), measure text content width
		if ( child->isType( UI_TYPE_TEXTNODE ) ) {
			auto* textNode = child->asType<UITextNode>();
			if ( !textNode->getText().empty() ) {
				FontStyleConfig fontConfig;
				if ( getFontStyleFromAncestor( mContainer, fontConfig ) ) {
					return Text::getTextWidth( textNode->getText(), fontConfig );
				}
			}
		}

		// Per §7.2.3: flex-basis: content always uses content-based sizing
		// and skips the explicit main size property (width/height) check.
		if ( !flexBasisContent ) {
			if ( mainAxis.horizontal && child->getLayoutWidthPolicy() == SizePolicy::Fixed &&
				 child->getUIStyle() ) {
				const auto* wprop = child->getUIStyle()->getProperty( PropertyId::Width );
				if ( wprop )
					return child->cssWidthPropertyToBorderBoxWidth( *wprop );
			}
			if ( !mainAxis.horizontal && child->getLayoutHeightPolicy() == SizePolicy::Fixed &&
				 child->getUIStyle() ) {
				const auto* hprop = child->getUIStyle()->getProperty( PropertyId::Height );
				if ( hprop )
					return child->cssHeightPropertyToBorderBoxHeight( *hprop );
			}
		}

		if ( mainAxis.horizontal )
			return child->getPixelsSize().getWidth();
		else
			return child->getPixelsSize().getHeight();
	}
	return flexBasisValue;
}

Float FlexLayouter::getItemMainSize( UIWidget* child, const Axis& mainAxis ) const {
	return mainAxis.horizontal ? child->getPixelsSize().getWidth()
							   : child->getPixelsSize().getHeight();
}

Float FlexLayouter::getItemCrossSize( UIWidget* child, const Axis& crossAxis ) const {
	return crossAxis.horizontal ? child->getPixelsSize().getWidth()
								: child->getPixelsSize().getHeight();
}

void FlexLayouter::measureFlexItems( const Axis& mainAxis, const Axis& crossAxis,
									 const Float containerCrossSize, const Float containerWidth,
									 const Float containerHeight, const Rectf& containerPadding,
									 bool indefiniteMainSize, bool indefiniteCrossSize ) {
	for ( auto& item : mItems ) {
		Float tentativeCrossSize = containerCrossSize;
		if ( indefiniteCrossSize ) {
			tentativeCrossSize = 0.f;
		} else if ( tentativeCrossSize <= 0.f && crossAxis.horizontal ) {
			tentativeCrossSize = containerWidth - containerPadding.Left - containerPadding.Right;
		} else if ( tentativeCrossSize <= 0.f && !crossAxis.horizontal ) {
			tentativeCrossSize = containerHeight - containerPadding.Top - containerPadding.Bottom;
		}
		if ( tentativeCrossSize < 0.f )
			tentativeCrossSize = 0.f;

		if ( crossAxis.horizontal && tentativeCrossSize > 0.f &&
			 item.widget->getLayoutWidthPolicy() != SizePolicy::Fixed )
			item.widget->setInternalPixelsWidth( tentativeCrossSize );
		if ( !crossAxis.horizontal && tentativeCrossSize > 0.f &&
			 item.widget->getLayoutHeightPolicy() != SizePolicy::Fixed )
			item.widget->setInternalPixelsHeight( tentativeCrossSize );

		SizePolicy oldMainPolicy = mainAxis.horizontal ? item.widget->getLayoutWidthPolicy()
													   : item.widget->getLayoutHeightPolicy();
		if ( oldMainPolicy == SizePolicy::MatchParent ) {
			if ( mainAxis.horizontal )
				item.widget->setLayoutWidthPolicy( SizePolicy::WrapContent );
			else
				item.widget->setLayoutHeightPolicy( SizePolicy::WrapContent );

			// Set the item's internal size to the container's main size,
			// so the block layouter's `else` branch (non-WrapContent)
			// wraps text at the full container width.  The flex-shrink
			// step will then proportionally reduce the item to its final
			// size, giving the expected "fill remaining space" behaviour.
			Float containerMainDim =
				mainAxis.horizontal
					? containerWidth - containerPadding.Left - containerPadding.Right
					: containerHeight - containerPadding.Top - containerPadding.Bottom;
			if ( containerMainDim > 0.f ) {
				if ( mainAxis.horizontal )
					item.widget->setInternalPixelsWidth( containerMainDim );
				else
					item.widget->setInternalPixelsHeight( containerMainDim );
			}
		}

		updateChildLayoutIfDirty( item.widget, this );

		if ( oldMainPolicy == SizePolicy::MatchParent ) {
			if ( mainAxis.horizontal )
				item.widget->setLayoutWidthPolicy( oldMainPolicy );
			else
				item.widget->setLayoutHeightPolicy( oldMainPolicy );
		}

		// Resolve margins along the flex axes.
		// Auto margins on the main axis are handled by the flex algorithm
		// (CSS Flexbox §8.1), so we treat them as 0 here.
		Rectf margin = item.widget->getLayoutPixelsMargin();

		// CSS §4.2: Percentage margins on flex items always resolve against
		// the flex container's inline size (width in horizontal writing mode).
		// The widget system may resolve top/bottom percentage margins against
		// the parent's height (ContainingBlockHeight), which is wrong for flex.
		// Re-resolve any percentage margins against the flex container's width.
		if ( item.widget->getUIStyle() ) {
			const StyleSheetProperty* props[4] = {
				item.widget->getUIStyle()->getProperty( PropertyId::MarginTop ),
				item.widget->getUIStyle()->getProperty( PropertyId::MarginRight ),
				item.widget->getUIStyle()->getProperty( PropertyId::MarginBottom ),
				item.widget->getUIStyle()->getProperty( PropertyId::MarginLeft ),
			};
			Float sides[4] = { margin.Top, margin.Right, margin.Bottom, margin.Left };
			for ( int i = 0; i < 4; i++ ) {
				if ( props[i] && StyleSheetLength::isPercentage( props[i]->value() ) ) {
					std::string val = props[i]->value();
					String::toLowerInPlace( val );
					String::replaceAll( val, "%", "" );
					Float pct = 0.f;
					String::fromString( pct, val );
					sides[i] = mContainer->getPixelsSize().getWidth() * pct / 100.f;
				}
			}
			margin.Top = sides[0];
			margin.Right = sides[1];
			margin.Bottom = sides[2];
			margin.Left = sides[3];
		}
		if ( mainAxis.horizontal ) {
			item.hasAutoMarginMainStart = item.widget->hasLayoutMarginLeftAuto();
			item.hasAutoMarginMainEnd = item.widget->hasLayoutMarginRightAuto();
			item.marginMainStart = item.hasAutoMarginMainStart ? 0.f : margin.Left;
			item.marginMainEnd = item.hasAutoMarginMainEnd ? 0.f : margin.Right;
			item.marginCrossStart = margin.Top;
			item.marginCrossEnd = margin.Bottom;
			item.hasAutoMarginCrossStart = item.widget->hasLayoutMarginTopAuto();
			item.hasAutoMarginCrossEnd = item.widget->hasLayoutMarginBottomAuto();
		} else {
			item.hasAutoMarginMainStart = item.widget->hasLayoutMarginTopAuto();
			item.hasAutoMarginMainEnd = item.widget->hasLayoutMarginBottomAuto();
			item.marginMainStart = item.hasAutoMarginMainStart ? 0.f : margin.Top;
			item.marginMainEnd = item.hasAutoMarginMainEnd ? 0.f : margin.Bottom;
			item.marginCrossStart = margin.Left;
			item.marginCrossEnd = margin.Right;
			item.hasAutoMarginCrossStart = item.widget->hasLayoutMarginLeftAuto();
			item.hasAutoMarginCrossEnd = item.widget->hasLayoutMarginRightAuto();
		}

		// Zero out cross-axis auto margins since they absorb free space during
		// cross-axis alignment and should not contribute to line cross sizing.
		if ( item.hasAutoMarginCrossStart )
			item.marginCrossStart = 0.f;
		if ( item.hasAutoMarginCrossEnd )
			item.marginCrossEnd = 0.f;

		if ( item.flexBasisIsPercentage && !item.flexBasisAuto ) {
			// CSS Flexbox §9.2: percentage flex-basis resolves against the flex
			// container's inner main size. Compute the container's inner main
			// size from the supplied dimensions and padding, then resolve the
			// percentage. If the main size is indefinite, treat as auto.
			Float containerInnerMain =
				mainAxis.horizontal
					? containerWidth - containerPadding.Left - containerPadding.Right
					: containerHeight - containerPadding.Top - containerPadding.Bottom;
			if ( indefiniteMainSize || containerInnerMain <= 0.f ) {
				item.targetMainSize = getItemMainSize( item.widget, mainAxis );
			} else {
				// Parse the percentage from the raw string value
				std::string pctStr = item.flexBasisRaw;
				String::toLowerInPlace( pctStr );
				String::replaceAll( pctStr, "%", "" );
				Float pct = 0.f;
				String::fromString( pct, pctStr );
				Float resolved = containerInnerMain * pct / 100.f;
				item.targetMainSize =
					mainAxis.horizontal
						? item.widget->cssResolvedLengthToBorderBoxWidth( resolved )
						: item.widget->cssResolvedLengthToBorderBoxHeight( resolved );
			}
		} else {
			item.targetMainSize =
				resolveFlexBasis( item.widget, mDirection, item.flexBasisValue, item.flexBasisAuto,
								  mainAxis, item.flexBasisContent );
		}

		// Per §7.2.3: flex-basis: content ignores the explicit main size property
		if ( !item.flexBasisContent ) {
			if ( mainAxis.horizontal && item.widget->getLayoutWidthPolicy() == SizePolicy::Fixed &&
				 item.widget->getUIStyle() ) {
				const auto* wprop = item.widget->getUIStyle()->getProperty( PropertyId::Width );
				if ( wprop )
					item.targetMainSize = item.widget->cssWidthPropertyToBorderBoxWidth( *wprop );
			} else if ( !mainAxis.horizontal &&
						item.widget->getLayoutHeightPolicy() == SizePolicy::Fixed &&
						item.widget->getUIStyle() ) {
				const auto* hprop = item.widget->getUIStyle()->getProperty( PropertyId::Height );
				if ( hprop )
					item.targetMainSize = item.widget->cssHeightPropertyToBorderBoxHeight( *hprop );
			}
		}

		// For text node flex items, measure text content to determine intrinsic size.
		// CSS Flexbox §4: text nodes inside a flex container become anonymous flex items
		// whose main size is based on their text content.
		if ( item.widget->isType( UI_TYPE_TEXTNODE ) ) {
			auto* textNode = item.widget->asType<UITextNode>();
			if ( !textNode->getText().empty() ) {
				FontStyleConfig fontConfig;
				if ( getFontStyleFromAncestor( mContainer, fontConfig ) ) {
					Float textWidth = Text::getTextWidth( textNode->getText(), fontConfig );
					Float textHeight =
						(Float)fontConfig.Font->getFontHeight( fontConfig.CharacterSize );
					if ( textWidth > 0.f ) {
						if ( item.targetMainSize <= 0.f )
							item.targetMainSize = textWidth;
						if ( mainAxis.horizontal )
							// cross axis is vertical for row
							item.widget->setInternalPixelsHeight( textHeight );
						else
							item.widget->setInternalPixelsWidth( textWidth );
					}
				}
			}
		}

		// Compute min/max main size (CSS §4.5: min-width/min-height default is auto for flex items)
		if ( mainAxis.horizontal ) {
			if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) &&
				 item.widget->asType<UIHTMLWidget>()->getLayouter() )
				item.minMainSize =
					item.widget->asType<UIHTMLWidget>()->getLayouter()->getMinIntrinsicWidth();
			else if ( item.widget->isType( UI_TYPE_TEXTNODE ) ) {
				// Text node with wrapping: minimum size = 0, since text can wrap
				// to any width and the cross size grows to accommodate the content.
				// Without wrapping, the minimum would be the full text width,
				// which prevents any flex-shrink.
				item.minMainSize = 0.f;
			} else
				item.minMainSize = item.widget->getPixelsSize().getWidth();
			if ( item.minMainSize < 0.f )
				item.minMainSize = 0.f;

			// Per §4.5: if the main-axis overflow is scrollable (non-visible),
			// the automatic minimum size is zero, not the content-based minimum.
			if ( item.widget->getUIStyle() ) {
				const auto* ov = item.widget->getUIStyle()->getProperty( PropertyId::Overflow );
				if ( ov ) {
					std::string val = ov->asString();
					String::toLowerInPlace( val );
					if ( val != "visible" )
						item.minMainSize = 0.f;
				}
			}

			// Clamp by explicit min-width/max-width CSS properties
			if ( item.widget->getUIStyle() ) {
				const auto* minW = item.widget->getUIStyle()->getProperty( PropertyId::MinWidth );
				if ( minW ) {
					Float explicitMin = item.widget->cssWidthPropertyToBorderBoxWidth( *minW );
					if ( explicitMin > item.minMainSize )
						item.minMainSize = explicitMin;
				}
				const auto* maxW = item.widget->getUIStyle()->getProperty( PropertyId::MaxWidth );
				if ( maxW )
					item.maxMainSize = item.widget->cssWidthPropertyToBorderBoxWidth( *maxW );
				else
					item.maxMainSize = std::numeric_limits<Float>::max();
			}
		} else {
			if ( item.widget->isType( UI_TYPE_TEXTNODE ) ) {
				FontStyleConfig fontConfig;
				if ( getFontStyleFromAncestor( mContainer, fontConfig ) ) {
					item.minMainSize =
						(Float)fontConfig.Font->getFontHeight( fontConfig.CharacterSize );
				} else {
					item.minMainSize = item.widget->getPixelsSize().getHeight();
				}
			} else {
				item.minMainSize = item.widget->getPixelsSize().getHeight();
			}
			if ( item.minMainSize < 0.f )
				item.minMainSize = 0.f;

			// Per §4.5: if the main-axis overflow is scrollable (non-visible),
			// the automatic minimum size is zero.
			if ( item.widget->getUIStyle() ) {
				const auto* ov = item.widget->getUIStyle()->getProperty( PropertyId::Overflow );
				if ( ov ) {
					std::string val = ov->asString();
					String::toLowerInPlace( val );
					if ( val != "visible" )
						item.minMainSize = 0.f;
				}
			}

			// Clamp by explicit min-height/max-height CSS properties
			if ( item.widget->getUIStyle() ) {
				const auto* minH = item.widget->getUIStyle()->getProperty( PropertyId::MinHeight );
				if ( minH ) {
					Float explicitMin = item.widget->cssHeightPropertyToBorderBoxHeight( *minH );
					if ( explicitMin > item.minMainSize )
						item.minMainSize = explicitMin;
				}
				const auto* maxH = item.widget->getUIStyle()->getProperty( PropertyId::MaxHeight );
				if ( maxH )
					item.maxMainSize = item.widget->cssHeightPropertyToBorderBoxHeight( *maxH );
				else
					item.maxMainSize = std::numeric_limits<Float>::max();
			}
		}

		// For collapsed items: zero out main-axis sizes, save cross size
		if ( item.collapsed ) {
			item.savedCrossSize = getItemCrossSize( item.widget, crossAxis );
			item.targetMainSize = 0.f;
			item.minMainSize = 0.f;
			item.maxMainSize = 0.f;
			item.flexGrow = 0.f;
			item.flexShrink = 0.f;
			item.marginMainStart = 0.f;
			item.marginMainEnd = 0.f;
			item.widget->setPixelsSize( 0, 0 );
		}
	}
}

void FlexLayouter::generateFlexLines( SmallVector<FlexLayouter::FlexLine, 8>& lines,
									  const Float containerMainSize, const Float columnGap ) {
	lines.clear();

	SmallVector<size_t, 16> remaining;
	for ( size_t i = 0; i < mItems.size(); i++ )
		remaining.push_back( i );

	while ( !remaining.empty() ) {
		FlexLine line;
		Float usedMain = 0.f;

		SmallVector<size_t, 16> nextRemaining;

		for ( size_t idx : remaining ) {
			const auto& item = mItems[idx];
			Float itemOuterSize = item.targetMainSize + item.marginMainStart + item.marginMainEnd;
			Float gap = line.itemIndices.empty() ? 0.f : columnGap;

			if ( mWrap == CSSFlexWrap::NoWrap ||
				 ( usedMain + gap + itemOuterSize <= containerMainSize &&
				   usedMain + gap + itemOuterSize >= 0.f ) ) {
				line.itemIndices.push_back( idx );
				usedMain += gap + itemOuterSize;
			} else {
				nextRemaining.push_back( idx );
			}
		}

		// Per CSS Flexbox §9.4, every flex line must contain at least one
		// flex item. If the container is narrower than any single item,
		// force the first remaining item into this line to avoid an
		// infinite loop.
		if ( line.itemIndices.empty() && !remaining.empty() ) {
			line.itemIndices.push_back( remaining.front() );
			nextRemaining.clear();
			for ( size_t i = 1; i < remaining.size(); i++ )
				nextRemaining.push_back( remaining[i] );
		}

		lines.push_back( std::move( line ) );
		remaining = std::move( nextRemaining );

		if ( mWrap == CSSFlexWrap::NoWrap )
			break;
	}

	if ( mWrap == CSSFlexWrap::WrapReverse )
		std::reverse( lines.begin(), lines.end() );
}

static CSSAlignSelf resolveAlignSelf( CSSAlignSelf alignSelf, CSSAlignItems alignItems ) {
	if ( alignSelf == CSSAlignSelf::Auto ) {
		switch ( alignItems ) {
			case CSSAlignItems::FlexStart:
				return CSSAlignSelf::FlexStart;
			case CSSAlignItems::FlexEnd:
				return CSSAlignSelf::FlexEnd;
			case CSSAlignItems::Center:
				return CSSAlignSelf::Center;
			case CSSAlignItems::Baseline:
				return CSSAlignSelf::Baseline;
			case CSSAlignItems::Stretch:
				return CSSAlignSelf::Stretch;
		}
	}
	return alignSelf;
}

void FlexLayouter::resolveFlexibleLengths( FlexLine& line, const Float containerMainSize,
										   const Float columnGap ) {
	if ( line.itemIndices.empty() )
		return;

	size_t itemCount = line.itemIndices.size();
	Float totalGaps = ( itemCount > 1 ) ? (Float)( itemCount - 1 ) * columnGap : 0.f;

	// Save original flex base sizes and compute totals
	SmallVector<Float, 16> baseSizes;
	Float totalFlexGrow = 0.f;
	Float totalFlexShrink = 0.f;
	for ( auto idx : line.itemIndices ) {
		baseSizes.push_back( mItems[idx].targetMainSize );
		totalFlexGrow += mItems[idx].flexGrow;
		totalFlexShrink += mItems[idx].flexShrink;
	}

	// Reset frozen state for all items on this line
	for ( auto idx : line.itemIndices )
		mItems[idx].frozen = false;

	// Compute initial free space using original flex base sizes
	Float totalOuter = 0.f;
	for ( size_t i = 0; i < itemCount; ++i ) {
		size_t idx = line.itemIndices[i];
		totalOuter += baseSizes[i] + mItems[idx].marginMainStart + mItems[idx].marginMainEnd;
	}
	Float freeSpace = containerMainSize - totalOuter - totalGaps;

	bool isGrow = freeSpace > 0.f && totalFlexGrow > 0.f;
	bool isShrink = freeSpace < 0.f && totalFlexShrink > 0.f;

	if ( freeSpace == 0.f || ( !isGrow && !isShrink ) ) {
		// No free space to distribute, or no flexing possible — just clamp
		for ( auto idx : line.itemIndices ) {
			auto& item = mItems[idx];
			if ( item.targetMainSize < item.minMainSize )
				item.targetMainSize = item.minMainSize;
			if ( item.targetMainSize > item.maxMainSize )
				item.targetMainSize = item.maxMainSize;
		}
		return;
	}

	// Freeze items with zero flex factor in the current direction
	for ( auto idx : line.itemIndices ) {
		if ( isGrow ? ( mItems[idx].flexGrow <= 0.f ) : ( mItems[idx].flexShrink <= 0.f ) )
			mItems[idx].frozen = true;
	}

	// Reset targetMainSize to base sizes before the first distribution
	for ( size_t i = 0; i < itemCount; ++i )
		mItems[line.itemIndices[i]].targetMainSize = baseSizes[i];

	// Helper: compute remaining free space using frozen items' clamped sizes
	// and unfrozen items' original flex base sizes.
	auto computeRemaining = [&]() -> Float {
		Float sum = totalGaps;
		for ( size_t i = 0; i < itemCount; ++i ) {
			size_t idx = line.itemIndices[i];
			const auto& item = mItems[idx];
			if ( item.frozen )
				sum += item.targetMainSize + item.marginMainStart + item.marginMainEnd;
			else
				sum += baseSizes[i] + item.marginMainStart + item.marginMainEnd;
		}
		return containerMainSize - sum;
	};

	// Iterative distribution loop (max 100 iterations as safety)
	for ( int iter = 0; iter < 100; ++iter ) {
		Float remaining = computeRemaining();
		if ( remaining == 0.f )
			break;

		if ( remaining > 0.f ) {
			// --- Grow distribution ---
			Float curGrow = 0.f;
			for ( auto idx : line.itemIndices )
				if ( !mItems[idx].frozen )
					curGrow += mItems[idx].flexGrow;
			if ( curGrow <= 0.f )
				break;

			Float used = 0.f;
			for ( size_t i = 0; i < itemCount; ++i ) {
				size_t idx = line.itemIndices[i];
				auto& item = mItems[idx];
				if ( item.frozen || item.flexGrow <= 0.f )
					continue;
				Float add = ( remaining * item.flexGrow ) / curGrow;
				item.targetMainSize = baseSizes[i] + add;
				used += add;
			}
			if ( curGrow > 0.f && ( remaining - used ) > 0.5f ) {
				for ( auto idx : line.itemIndices ) {
					if ( !mItems[idx].frozen && mItems[idx].flexGrow > 0.f ) {
						mItems[idx].targetMainSize += ( remaining - used );
						break;
					}
				}
			}
		} else {
			// --- Shrink distribution ---
			Float deficit = -remaining;
			Float totalScaledShrink = 0.f;
			for ( size_t i = 0; i < itemCount; ++i ) {
				size_t idx = line.itemIndices[i];
				if ( !mItems[idx].frozen && mItems[idx].flexShrink > 0.f )
					totalScaledShrink += mItems[idx].flexShrink * baseSizes[i];
			}
			if ( totalScaledShrink <= 0.f )
				break;

			Float used = 0.f;
			for ( size_t i = 0; i < itemCount; ++i ) {
				size_t idx = line.itemIndices[i];
				auto& item = mItems[idx];
				if ( item.frozen || item.flexShrink <= 0.f )
					continue;
				Float shrink = deficit * ( item.flexShrink * baseSizes[i] ) / totalScaledShrink;
				Float newMain = baseSizes[i] - shrink;
				if ( newMain < 0.f )
					newMain = 0.f;
				item.targetMainSize = newMain;
				used += baseSizes[i] - item.targetMainSize;
			}
			if ( ( deficit - used ) > 0.5f ) {
				for ( auto idx : line.itemIndices ) {
					if ( !mItems[idx].frozen && mItems[idx].targetMainSize > 0.f ) {
						mItems[idx].targetMainSize -= ( deficit - used );
						break;
					}
				}
			}
		}

		// Clamp each unfrozen item and freeze those that hit min/max
		bool anyViolation = false;
		for ( size_t i = 0; i < itemCount; ++i ) {
			size_t idx = line.itemIndices[i];
			auto& item = mItems[idx];
			if ( item.frozen )
				continue;
			if ( item.targetMainSize < item.minMainSize ) {
				item.targetMainSize = item.minMainSize;
				item.frozen = true;
				anyViolation = true;
			} else if ( item.targetMainSize > item.maxMainSize ) {
				item.targetMainSize = item.maxMainSize;
				item.frozen = true;
				anyViolation = true;
			}
		}

		if ( !anyViolation )
			break;
	}
}

void FlexLayouter::alignMainAxis( FlexLine& line, const Float containerMainSize,
								  const Float columnGap ) {
	if ( line.itemIndices.empty() )
		return;

	size_t itemCount = line.itemIndices.size();
	Float totalItemMain = 0.f;
	for ( size_t idx : line.itemIndices ) {
		const auto& item = mItems[idx];
		totalItemMain += item.targetMainSize + item.marginMainStart + item.marginMainEnd;
	}

	Float totalGap = ( itemCount > 1 ) ? (Float)( itemCount - 1 ) * columnGap : 0.f;
	Float freeSpaceMain = containerMainSize - totalItemMain - totalGap;

	// CSS Flexbox §8.1: Before justify-content, positive free space on the main
	// axis is first consumed by auto margins. If an item has margin-main-start: auto,
	// the free space before it is absorbed; if margin-main-end: auto, the free space
	// after it is absorbed. If both are auto, free space is split between them.
	// We track consumed space per item and adjust freeSpaceMain for justify-content.
	SmallVector<Float, 16> autoMainStart( itemCount, 0.f );
	SmallVector<Float, 16> autoMainEnd( itemCount, 0.f );
	Float remainingFree = freeSpaceMain;
	if ( remainingFree > 0.f ) {
		// Count items with main-axis auto margins
		SmallVector<size_t, 16> autoStartItems;
		SmallVector<size_t, 16> autoEndItems;
		SmallVector<size_t, 16> autoBothItems;
		for ( size_t i = 0; i < itemCount; ++i ) {
			size_t idx = line.itemIndices[i];
			const auto& item = mItems[idx];
			if ( item.hasAutoMarginMainStart && item.hasAutoMarginMainEnd )
				autoBothItems.push_back( i );
			else if ( item.hasAutoMarginMainStart )
				autoStartItems.push_back( i );
			else if ( item.hasAutoMarginMainEnd )
				autoEndItems.push_back( i );
		}

		// Distribute free space to auto margins in order:
		// 1. Items with auto on both sides split the free space equally
		// 2. Items with auto on start side get all their free space
		// 3. Items with auto on end side get all their free space
		if ( !autoBothItems.empty() ) {
			Float perItem = remainingFree / (Float)autoBothItems.size();
			for ( size_t i : autoBothItems ) {
				Float half = perItem * 0.5f;
				autoMainStart[i] = half;
				autoMainEnd[i] = half;
			}
			remainingFree = 0.f;
		} else if ( !autoStartItems.empty() || !autoEndItems.empty() ) {
			Float perStart =
				autoStartItems.empty()
					? 0.f
					: remainingFree / (Float)( autoStartItems.size() + autoEndItems.size() );
			Float perEnd = perStart;
			for ( size_t i : autoStartItems ) {
				autoMainStart[i] = perStart;
				autoMainEnd[i] = 0.f;
			}
			for ( size_t i : autoEndItems ) {
				autoMainStart[i] = 0.f;
				autoMainEnd[i] = perEnd;
			}
			remainingFree = 0.f;
		}
	}

	// Remaining free space (after auto margins) is distributed by justify-content
	Float mainOffset = 0.f;
	Float spacing = columnGap;

	switch ( mJustify ) {
		case CSSJustifyContent::FlexEnd:
			mainOffset = remainingFree;
			break;
		case CSSJustifyContent::Center:
			mainOffset = remainingFree * 0.5f;
			break;
		case CSSJustifyContent::SpaceBetween:
			if ( remainingFree > 0.f && itemCount > 1 )
				spacing = columnGap + remainingFree / (Float)( itemCount - 1 );
			else
				mainOffset = 0.f;
			break;
		case CSSJustifyContent::SpaceAround:
			if ( remainingFree > 0.f && itemCount > 0 ) {
				spacing = columnGap + remainingFree / (Float)itemCount;
				mainOffset = spacing * 0.5f;
			}
			break;
		case CSSJustifyContent::SpaceEvenly:
			if ( remainingFree > 0.f && itemCount > 0 ) {
				Float slot = columnGap + remainingFree / (Float)( itemCount + 1 );
				mainOffset = slot;
				spacing = slot;
			}
			break;
		case CSSJustifyContent::FlexStart:
		default:
			mainOffset = 0.f;
			break;
	}

	Float pos = mainOffset;
	for ( size_t i = 0; i < itemCount; ++i ) {
		size_t idx = line.itemIndices[i];
		const auto& item = mItems[idx];
		mItems[idx].mainPos = pos + autoMainStart[i] + item.marginMainStart;
		pos += autoMainStart[i] + item.targetMainSize + item.marginMainStart + item.marginMainEnd +
			   autoMainEnd[i] + spacing;
	}
}

void FlexLayouter::resolveCrossSizes( FlexLine& line, const Axis& crossAxis,
									  const Axis& mainAxis ) {
	if ( line.itemIndices.empty() ) {
		line.crossSize = 0.f;
		return;
	}

	// Set each item's main-axis size to its resolved targetMainSize so that
	// getItemCrossSize reflects the correct cross size for the final width.
	// Skip collapsed items — their targetMainSize is 0 and we use savedCrossSize.
	for ( size_t idx : line.itemIndices ) {
		auto& item = mItems[idx];
		if ( item.collapsed )
			continue;
		if ( mainAxis.horizontal )
			item.widget->setInternalPixelsWidth( item.targetMainSize );
		else
			item.widget->setInternalPixelsHeight( item.targetMainSize );
		updateChildLayoutIfDirty( item.widget, this );
	}

	// For text nodes (anonymous flex items), configure a cached Text object for
	// multi-line word wrapping. In row direction, measure wrapped text height at
	// the resolved main size and use it as the cross size.
	for ( size_t idx : line.itemIndices ) {
		auto& item = mItems[idx];
		if ( item.collapsed )
			continue;
		if ( item.widget->isType( UI_TYPE_TEXTNODE ) ) {
			auto* textNode = item.widget->asType<UITextNode>();
			if ( !textNode->getText().empty() ) {
				FontStyleConfig fontConfig;
				if ( getFontStyleFromAncestor( mContainer, fontConfig ) ) {
					Text* flexText = textNode->getFlexText();
					flexText->setStyleConfig( fontConfig );
					flexText->setString( textNode->getText() );
					flexText->setLineWrapMode( LineWrapMode::Word );
					Float maxWidth;
					if ( mainAxis.horizontal ) {
						maxWidth = item.targetMainSize;
					} else {
						// Column: wrap at tentative cross-axis width
						maxWidth = item.widget->getPixelsSize().getWidth();
					}
					if ( maxWidth <= 0.f )
						maxWidth = 10000.f;
					flexText->setMaxWrapWidth( maxWidth );
					if ( mainAxis.horizontal ) {
						Float wrappedHeight = flexText->getTextHeight();
						if ( wrappedHeight <= 0.f )
							wrappedHeight =
								(Float)fontConfig.Font->getFontHeight( fontConfig.CharacterSize );
						item.widget->setInternalPixelsHeight( wrappedHeight );
					}
				}
			}
		}
	}

	Float maxCross = 0.f;
	bool hasBaselineItem = false;
	Float maxBaselineAscent = 0.f;
	Float maxBaselineDescent = 0.f;
	for ( size_t idx : line.itemIndices ) {
		auto& item = mItems[idx];
		if ( item.collapsed ) {
			item.crossSize = item.savedCrossSize;
		} else {
			item.crossSize = getItemCrossSize( item.widget, crossAxis );
		}
		item.outerCrossSize = item.crossSize + item.marginCrossStart + item.marginCrossEnd;
		maxCross = eemax( maxCross, item.outerCrossSize );

		if ( resolveAlignSelf( item.alignSelf, mAlignItems ) == CSSAlignSelf::Baseline ) {
			hasBaselineItem = true;
			Float bl = getItemBaselineOffset( item.widget );
			Float asc = bl;
			Float desc = item.outerCrossSize - bl;
			if ( !crossAxis.horizontal ) {
				asc = bl;
				desc = item.outerCrossSize - bl;
			}
			// Baseline alignment only meaningful when cross axis is vertical
			if ( !crossAxis.horizontal && !mainAxis.horizontal ) {
				// Column direction: cross axis is horizontal, baseline is along main axis
				asc = 0.f;
				desc = 0.f;
			}
			maxBaselineAscent = eemax( maxBaselineAscent, asc );
			maxBaselineDescent = eemax( maxBaselineDescent, desc );
		}
	}
	if ( hasBaselineItem )
		line.crossSize = eemax( line.crossSize, maxBaselineAscent + maxBaselineDescent );
	else
		line.crossSize = maxCross;
}

void FlexLayouter::alignCrossAxis( const SmallVector<FlexLine, 8>& lines,
								   const Float containerCrossSize, const Float rowGap,
								   const Axis& crossAxis ) {
	if ( lines.empty() )
		return;

	Float totalLinesCross = 0.f;
	for ( const auto& line : lines )
		totalLinesCross += line.crossSize;

	size_t lineCount = lines.size();
	Float totalRowGap = ( lineCount > 1 ) ? (Float)( lineCount - 1 ) * rowGap : 0.f;
	Float freeSpaceCross = containerCrossSize - totalLinesCross - totalRowGap;

	Float crossOffset = 0.f;
	Float addPerLine = 0.f;
	Float lineGap = rowGap;

	if ( lineCount > 1 ) {
		Float fsc = freeSpaceCross;
		switch ( mAlignContent ) {
			case CSSAlignContent::FlexEnd:
				crossOffset = freeSpaceCross;
				break;
			case CSSAlignContent::Center:
				crossOffset = freeSpaceCross * 0.5f;
				break;
			case CSSAlignContent::SpaceBetween:
				if ( fsc > 0.f && lineCount > 1 )
					lineGap = fsc / (Float)( lineCount - 1 );
				break;
			case CSSAlignContent::SpaceAround:
				if ( fsc > 0.f ) {
					lineGap = fsc / (Float)lineCount;
					crossOffset = lineGap * 0.5f;
				}
				break;
			case CSSAlignContent::SpaceEvenly:
				if ( fsc > 0.f ) {
					lineGap = fsc / (Float)( lineCount + 1 );
					crossOffset = lineGap;
				}
				break;
			case CSSAlignContent::Stretch:
				if ( fsc > 0.f && lineCount > 0 )
					addPerLine = fsc / (Float)lineCount;
				break;
			case CSSAlignContent::FlexStart:
			default:
				break;
		}
	}

	Float pos = crossOffset;
	for ( size_t li = 0; li < lines.size(); li++ ) {
		FlexLine& line = const_cast<FlexLine&>( lines[li] );
		line.crossPos = pos;

		Float lineCrossSize = line.crossSize + addPerLine;
		if ( lineCrossSize < line.crossSize )
			lineCrossSize = line.crossSize;
		if ( lines.size() == 1 && containerCrossSize > 0.f )
			lineCrossSize = containerCrossSize;
		if ( lineCrossSize > containerCrossSize && containerCrossSize > 0.f )
			lineCrossSize = containerCrossSize;

		// Pre-compute max baseline offset for baseline-aligned items on this line.
		// Baseline alignment applies when the cross axis is the block axis (vertical
		// for row direction, or horizontal in column direction — spec treats inline-axis
		// parallel to cross axis differently). Here we only apply baseline alignment
		// when the cross axis is vertical (row/row-reverse direction).
		Float maxLineBaseline = 0.f;
		bool hasBaselineOnLine = false;
		if ( !crossAxis.horizontal ) {
			for ( size_t idx : line.itemIndices ) {
				auto& item = mItems[idx];
				if ( item.hasAutoMarginCrossStart || item.hasAutoMarginCrossEnd )
					continue;
				CSSAlignSelf resolved = resolveAlignSelf( item.alignSelf, mAlignItems );
				if ( resolved == CSSAlignSelf::Baseline ) {
					hasBaselineOnLine = true;
					Float bl = getItemBaselineOffset( item.widget ) + item.marginCrossStart;
					maxLineBaseline = eemax( maxLineBaseline, bl );
				}
			}
		}

		for ( size_t idx : line.itemIndices ) {
			auto& item = mItems[idx];

			// Cross-axis auto margins (§8.1): absorb free space before align-self.
			if ( item.hasAutoMarginCrossStart || item.hasAutoMarginCrossEnd ) {
				Float free = lineCrossSize - item.outerCrossSize;
				if ( item.hasAutoMarginCrossStart && item.hasAutoMarginCrossEnd ) {
					item.crossPos = pos + free * 0.5f + item.marginCrossStart;
				} else if ( item.hasAutoMarginCrossStart ) {
					item.crossPos = pos + free + item.marginCrossStart;
				} else {
					item.crossPos = pos + item.marginCrossStart;
				}
				continue;
			}

			CSSAlignSelf resolved = resolveAlignSelf( item.alignSelf, mAlignItems );

			Float itemFreeSpace = lineCrossSize - item.outerCrossSize;
			switch ( resolved ) {
				case CSSAlignSelf::FlexStart:
					item.crossPos = pos + item.marginCrossStart;
					break;
				case CSSAlignSelf::FlexEnd:
					item.crossPos = pos + itemFreeSpace + item.marginCrossStart;
					break;
				case CSSAlignSelf::Center:
					item.crossPos = pos + itemFreeSpace * 0.5f + item.marginCrossStart;
					break;
				case CSSAlignSelf::Stretch:
					item.crossPos = pos + item.marginCrossStart;
					if ( !( crossAxis.horizontal
								? ( item.widget->getLayoutWidthPolicy() == SizePolicy::Fixed )
								: ( item.widget->getLayoutHeightPolicy() == SizePolicy::Fixed ) ) )
						item.crossSize =
							lineCrossSize - item.marginCrossStart - item.marginCrossEnd;
					break;
				case CSSAlignSelf::Baseline:
					if ( hasBaselineOnLine ) {
						Float bl = getItemBaselineOffset( item.widget ) + item.marginCrossStart;
						item.crossPos = pos + ( maxLineBaseline - bl ) + item.marginCrossStart;
					} else {
						item.crossPos = pos + item.marginCrossStart;
					}
					break;
				case CSSAlignSelf::Auto:
				default:
					item.crossPos = pos + item.marginCrossStart;
					break;
			}
		}

		pos += lineCrossSize + lineGap;
	}
}

void FlexLayouter::applyLayout( const SmallVector<FlexLine, 8>& lines, const Axis& mainAxis,
								const Axis& crossAxis, const Rectf& containerPadding,
								const Float containerWidth, const Float containerHeight,
								const SizePolicy widthPolicy, const SizePolicy heightPolicy ) {
	Float contentW = 0.f;
	Float contentH = 0.f;

	for ( const auto& line : lines ) {
		for ( size_t idx : line.itemIndices ) {
			auto& item = mItems[idx];
			if ( item.collapsed ) {
				// Position collapsed items but give them zero size
				if ( mainAxis.horizontal ) {
					Float x = mainAxis.reversed
								  ? containerWidth - containerPadding.Right - item.mainPos
								  : containerPadding.Left + item.mainPos;
					Float y = containerPadding.Top + item.crossPos;
					item.widget->setPixelsPosition( x, y );
				} else {
					Float y = mainAxis.reversed
								  ? containerHeight - containerPadding.Bottom - item.mainPos
								  : containerPadding.Top + item.mainPos;
					Float x = containerPadding.Left + item.crossPos;
					item.widget->setPixelsPosition( x, y );
				}
				continue;
			}
			Float widgetX, widgetY, widgetW, widgetH;

			if ( mainAxis.horizontal ) {
				if ( mainAxis.reversed ) {
					widgetX = containerWidth - containerPadding.Right - item.mainPos -
							  item.targetMainSize;
				} else {
					widgetX = containerPadding.Left + item.mainPos;
				}
				widgetY = containerPadding.Top + item.crossPos;
				widgetW = item.targetMainSize;
				widgetH = item.crossSize;
				contentW = eemax( contentW,
								  widgetX + widgetW + item.marginMainEnd - containerPadding.Left );
				contentH = eemax( contentH,
								  widgetY + widgetH + item.marginCrossEnd - containerPadding.Top );
			} else {
				if ( mainAxis.reversed ) {
					widgetY = containerHeight - containerPadding.Bottom - item.mainPos -
							  item.targetMainSize;
				} else {
					widgetY = containerPadding.Top + item.mainPos;
				}
				widgetX = containerPadding.Left + item.crossPos;
				widgetW = item.crossSize;
				widgetH = item.targetMainSize;
				contentW = eemax( contentW,
								  widgetX + widgetW + item.marginCrossEnd - containerPadding.Left );
				contentH = eemax( contentH,
								  widgetY + widgetH + item.marginMainEnd - containerPadding.Top );
			}

			item.widget->setPixelsPosition( widgetX, widgetY );
			item.widget->setPixelsSize( widgetW, widgetH );

			updateChildLayoutIfDirty( item.widget, this );
		}
	}

	Float totW = containerWidth;
	Float totH = containerHeight;

	if ( widthPolicy == SizePolicy::WrapContent )
		totW = containerPadding.Left + contentW + containerPadding.Right;
	else if ( widthPolicy != SizePolicy::Fixed ) {
		auto* parent = mContainer->getParent();
		bool parentIsFlex = parent && parent->isType( UI_TYPE_HTML_WIDGET ) &&
							static_cast<UIHTMLWidget*>( parent )->isFlex();
		if ( parentIsFlex ) {
			totW = containerWidth;
		} else {
			totW = parent ? parent->getPixelsSize().getWidth() : totW;
		}
	}

	if ( heightPolicy == SizePolicy::WrapContent )
		totH = containerPadding.Top + contentH + containerPadding.Bottom;

	mContainer->setInternalPixelsWidth( totW );
	mContainer->setInternalPixelsHeight( totH );
}

void FlexLayouter::updateLayout() {
	if ( !mContainer->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	auto* widget = mContainer->asType<UIHTMLWidget>();
	if ( widget->isInline() || mPacking )
		return;

	RichText* richText = widget->isType( UI_TYPE_RICHTEXT )
									   ? widget->asType<UIRichText>()->getRichTextPtr()
									   : nullptr;
	bool preserveFloatConstrainedBFCWidth =
		widget->establishesBlockFormattingContext() &&
		widget->getLayoutWidthPolicy() == SizePolicy::MatchParent && richText != nullptr &&
		!richText->getExternalFloatExclusions().empty();

	mPacking = true;

	mContainer->beginAttributesTransaction();

	{
		auto* parent = mContainer->getParent();
		bool parentIsFlexContainer = parent && parent->isType( UI_TYPE_HTML_WIDGET ) &&
									 static_cast<UIHTMLWidget*>( parent )->isFlex();
		if ( !parentIsFlexContainer && !preserveFloatConstrainedBFCWidth )
			setMatchParentIfNeededVerticalGrowth();
	}

	readContainerStyle( mDirection, mWrap, mJustify, mAlignItems, mAlignContent, mColumnGap,
						mRowGap );

	const Rectf containerPadding = mContainer->getPixelsContentOffset();

	const SizePolicy widthPolicy = mContainer->getLayoutWidthPolicy();
	const SizePolicy heightPolicy = mContainer->getLayoutHeightPolicy();

	Float containerWidth = mContainer->getPixelsSize().getWidth();
	Float containerHeight = mContainer->getPixelsSize().getHeight();

	if ( widthPolicy == SizePolicy::Fixed && mContainer->getUIStyle() ) {
		const auto* wprop = mContainer->getUIStyle()->getProperty( PropertyId::Width );
		if ( wprop ) {
			Float rawWidth = widget->cssWidthPropertyToBorderBoxWidth( *wprop );
			containerWidth =
				mContainer->fitMinMaxSizePx( Sizef( rawWidth, containerHeight ) ).getWidth();
		}
	}

	if ( heightPolicy == SizePolicy::Fixed && mContainer->getUIStyle() ) {
		const auto* hprop = mContainer->getUIStyle()->getProperty( PropertyId::Height );
		if ( hprop ) {
			Float rawHeight = widget->cssHeightPropertyToBorderBoxHeight( *hprop );
			containerHeight =
				mContainer->fitMinMaxSizePx( Sizef( containerWidth, rawHeight ) ).getHeight();
		}
	}

	Axis mainAxis = getMainAxis( mDirection );
	Axis crossAxis = getCrossAxis( mDirection );

	const Float totalPaddingMain = mainAxis.horizontal
									   ? containerPadding.Left + containerPadding.Right
									   : containerPadding.Top + containerPadding.Bottom;
	const Float totalPaddingCross = crossAxis.horizontal
										? containerPadding.Left + containerPadding.Right
										: containerPadding.Top + containerPadding.Bottom;

	Float containerMainSize = mainAxis.horizontal ? containerWidth : containerHeight;
	Float containerCrossSize = crossAxis.horizontal ? containerWidth : containerHeight;

	if ( widthPolicy != SizePolicy::Fixed && mainAxis.horizontal )
		containerMainSize -= totalPaddingMain;
	if ( heightPolicy != SizePolicy::Fixed && !mainAxis.horizontal )
		containerMainSize -= totalPaddingMain;

	if ( widthPolicy != SizePolicy::Fixed && crossAxis.horizontal )
		containerCrossSize -= totalPaddingCross;
	if ( heightPolicy != SizePolicy::Fixed && !crossAxis.horizontal )
		containerCrossSize -= totalPaddingCross;

	if ( containerMainSize < 0.f )
		containerMainSize = 0.f;
	if ( containerCrossSize < 0.f )
		containerCrossSize = 0.f;

	// Detect indefinite cross size: WrapContent in the cross axis direction
	bool indefiniteCrossSize = ( crossAxis.horizontal && widthPolicy == SizePolicy::WrapContent ) ||
							   ( !crossAxis.horizontal && heightPolicy == SizePolicy::WrapContent );

	// When cross size is indefinite, zero out containerCrossSize so that
	// the flex algorithm sizes lines to content rather than using a stale
	// container size from a previous layout pass.
	if ( indefiniteCrossSize )
		containerCrossSize = 0.f;

	collectFlexItems( mItems );

	// Clear any stale RichText data from a previous block layout mode.
	// When a flex container has actual flex items, each item draws its own
	// content via UITextSpan::draw(). The parent's mRichText is stale and
	// should not be rendered, otherwise text appears twice (once from the
	// parent's RichText and once from the flex item's own draw()).
	if ( !mItems.empty() && mContainer->isType( UI_TYPE_RICHTEXT ) ) {
		mContainer->asType<UIRichText>()->getRichTextPtr()->clear();
	}

	if ( mItems.empty() ) {
		// If the container is a UITextSpan with its own text content (e.g. an <a> with
		// display:flex containing only text), process the text via RichText so the
		// container gets a proper content-based size.
		bool hasTextContent = false;
		if ( mContainer->isType( UI_TYPE_TEXTSPAN ) ) {
			auto* textSpan = mContainer->asType<UITextSpan>();
			auto* rt = textSpan->getRichTextPtr();
			if ( rt && !textSpan->getText().empty() &&
				 textSpan->getFontStyleConfig().Font != nullptr ) {
				hasTextContent = true;
				UIRichText::rebuildRichText( textSpan, *rt );
				rt->updateLayout();
				Float textW = rt->getSize().getWidth();
				Float textH = rt->getSize().getHeight();

				if ( widthPolicy == SizePolicy::WrapContent )
					mContainer->setInternalPixelsWidth( textW + containerPadding.Left +
														containerPadding.Right );
				else if ( widthPolicy == SizePolicy::Fixed )
					mContainer->setInternalPixelsWidth( containerWidth );
				else if ( preserveFloatConstrainedBFCWidth )
					mContainer->setInternalPixelsWidth( containerWidth );
				else
					mContainer->setInternalPixelsWidth(
						mContainer->getParent()
							? mContainer->getParent()->getPixelsSize().getWidth()
							: 0 );

				if ( heightPolicy == SizePolicy::WrapContent )
					mContainer->setInternalPixelsHeight( textH + containerPadding.Top +
														 containerPadding.Bottom );
				else if ( heightPolicy == SizePolicy::Fixed )
					mContainer->setInternalPixelsHeight( containerHeight );
			}
		}

		if ( !hasTextContent ) {
			Float totW = containerPadding.Left + containerPadding.Right;
			Float totH = containerPadding.Top + containerPadding.Bottom;

			if ( widthPolicy == SizePolicy::WrapContent )
				mContainer->setInternalPixelsWidth( totW );
			else if ( widthPolicy == SizePolicy::Fixed )
				mContainer->setInternalPixelsWidth( containerWidth );
			else if ( preserveFloatConstrainedBFCWidth )
				mContainer->setInternalPixelsWidth( containerWidth );
			else
				mContainer->setInternalPixelsWidth(
					mContainer->getParent() ? mContainer->getParent()->getPixelsSize().getWidth()
											: 0 );

			if ( heightPolicy == SizePolicy::WrapContent )
				mContainer->setInternalPixelsHeight( totH );
			else if ( heightPolicy == SizePolicy::Fixed )
				mContainer->setInternalPixelsHeight( containerHeight );
		}

		mContainer->endAttributesTransaction();
		mPacking = false;
		return;
	}

	// Detect indefinite main size: WrapContent in the main axis direction
	bool indefiniteMainSize = ( mainAxis.horizontal && widthPolicy == SizePolicy::WrapContent ) ||
							  ( !mainAxis.horizontal && heightPolicy == SizePolicy::WrapContent );

	measureFlexItems( mainAxis, crossAxis, containerCrossSize, containerWidth, containerHeight,
					  containerPadding, indefiniteMainSize, indefiniteCrossSize );

	SmallVector<FlexLine, 8> lines;

	if ( indefiniteMainSize ) {
		// Compute container main size from item outer sizes (CSS §9.3)
		Float naturalMainSize = 0.f;
		for ( auto& item : mItems )
			naturalMainSize += item.targetMainSize + item.marginMainStart + item.marginMainEnd;
		if ( mItems.size() > 1 )
			naturalMainSize += (Float)( mItems.size() - 1 ) * mColumnGap;
		containerMainSize = naturalMainSize;
	}

	generateFlexLines( lines, containerMainSize, mColumnGap );

	for ( auto& line : lines ) {
		if ( !indefiniteMainSize )
			resolveFlexibleLengths( line, containerMainSize, mColumnGap );
		alignMainAxis( line, containerMainSize, mColumnGap );
		resolveCrossSizes( line, crossAxis, mainAxis );
	}

	alignCrossAxis( lines, containerCrossSize, mRowGap, crossAxis );

	applyLayout( lines, mainAxis, crossAxis, containerPadding, containerWidth, containerHeight,
				 widthPolicy, heightPolicy );
	if ( preserveFloatConstrainedBFCWidth ) {
		// The parent RichText stream has already resolved this match-parent BFC against active
		// floats and placed it into a narrowed atomic fragment. Flex layout still needs to run
		// using that used width, but its normal match-parent finalization would expand back to
		// the full parent content box and overlap the float.
		mContainer->setInternalPixelsWidth( containerWidth );
	}

	// CSS Flexbox §8.5: store the container's baseline for use by outer flex containers.
	// For row/row-reverse, baseline = first flex line's baseline offset from content top.
	// For column/column-reverse, baseline = last flex line's baseline offset from content top.
	{
		mContainerBaseline = 0.f;
		if ( !lines.empty() ) {
			Axis lineCrossAxis = crossAxis;
			size_t lineIdx = ( mDirection == CSSFlexDirection::Row ||
							   mDirection == CSSFlexDirection::RowReverse )
								 ? 0
								 : lines.size() - 1;
			const FlexLine& blLine = lines[lineIdx];
			Float maxBl = 0.f;
			for ( size_t idx : blLine.itemIndices ) {
				auto& item = mItems[idx];
				if ( item.collapsed )
					continue;
				maxBl =
					eemax( maxBl, getItemBaselineOffset( item.widget ) + item.marginCrossStart );
			}
			Float absp = lineCrossAxis.horizontal ? 0.f : blLine.crossPos;
			mContainerBaseline = absp + maxBl;
		}
	}

	// CSS Flexbox §4.3: painting order follows order-modified document order.
	// Set flag so drawChildren() sorts by order when items have different order values.
	if ( mItems.size() > 1 ) {
		bool needsOrderSort = false;
		for ( size_t i = 1; i < mItems.size(); i++ ) {
			if ( mItems[i].order != mItems[0].order ) {
				needsOrderSort = true;
				break;
			}
		}
		mContainer->asType<UIHTMLWidget>()->setNeedsOrderSort( needsOrderSort );
	} else {
		mContainer->asType<UIHTMLWidget>()->setNeedsOrderSort( false );
	}

	mContainer->endAttributesTransaction();
	mPacking = false;
}

void FlexLayouter::computeIntrinsicWidths() {
	if ( !mIntrinsicWidthsDirty )
		return;

	if ( !mContainer->isType( UI_TYPE_HTML_WIDGET ) )
		return;

	if ( mContainer->getLayoutWidthPolicy() == SizePolicy::Fixed ) {
		mIntrinsicWidthsDirty = false;
		return;
	}

	readContainerStyle( mDirection, mWrap, mJustify, mAlignItems, mAlignContent, mColumnGap,
						mRowGap );

	Axis mainAxis = getMainAxis( mDirection );

	SmallVector<FlexItem, 16> items;
	collectFlexItems( items );

	if ( items.empty() ) {
		const Rectf containerPadding = mContainer->getPixelsContentOffset();
		mMaxIntrinsicWidth = containerPadding.Left + containerPadding.Right;
		mMinIntrinsicWidth = containerPadding.Left + containerPadding.Right;
		mIntrinsicWidthsDirty = false;
		return;
	}

	// Zero out collapsed items so they don't contribute to intrinsic sizing
	for ( auto& item : items ) {
		if ( item.collapsed ) {
			item.targetMainSize = 0.f;
			item.marginMainStart = 0.f;
			item.marginMainEnd = 0.f;
		}
	}

	for ( auto& item : items ) {
		if ( item.flexBasisIsPercentage && !item.flexBasisAuto ) {
			// Percentage flex-basis in intrinsic sizing: use content-based
			// size as a fallback since container size is not known here.
			item.targetMainSize = getItemMainSize( item.widget, mainAxis );
		} else {
			item.targetMainSize =
				resolveFlexBasis( item.widget, mDirection, item.flexBasisValue, item.flexBasisAuto,
								  mainAxis, item.flexBasisContent );
		}

		Rectf margin = item.widget->getLayoutPixelsMargin();

		// Re-resolve percentage margins against flex container's inline size
		if ( item.widget->getUIStyle() ) {
			const StyleSheetProperty* props[4] = {
				item.widget->getUIStyle()->getProperty( PropertyId::MarginTop ),
				item.widget->getUIStyle()->getProperty( PropertyId::MarginRight ),
				item.widget->getUIStyle()->getProperty( PropertyId::MarginBottom ),
				item.widget->getUIStyle()->getProperty( PropertyId::MarginLeft ),
			};
			Float sides[4] = { margin.Top, margin.Right, margin.Bottom, margin.Left };
			for ( int i = 0; i < 4; i++ ) {
				if ( props[i] && StyleSheetLength::isPercentage( props[i]->value() ) ) {
					std::string val = props[i]->asString();
					String::toLowerInPlace( val );
					String::replaceAll( val, "%", "" );
					Float pct = 0.f;
					String::fromString( pct, val );
					sides[i] = mContainer->getPixelsSize().getWidth() * pct / 100.f;
				}
			}
			margin.Top = sides[0];
			margin.Right = sides[1];
			margin.Bottom = sides[2];
			margin.Left = sides[3];
		}

		if ( mainAxis.horizontal ) {
			item.hasAutoMarginMainStart = item.widget->hasLayoutMarginLeftAuto();
			item.hasAutoMarginMainEnd = item.widget->hasLayoutMarginRightAuto();
			item.marginMainStart = item.hasAutoMarginMainStart ? 0.f : margin.Left;
			item.marginMainEnd = item.hasAutoMarginMainEnd ? 0.f : margin.Right;
			item.marginCrossStart = margin.Top;
			item.marginCrossEnd = margin.Bottom;
		} else {
			item.hasAutoMarginMainStart = item.widget->hasLayoutMarginTopAuto();
			item.hasAutoMarginMainEnd = item.widget->hasLayoutMarginBottomAuto();
			item.marginMainStart = item.hasAutoMarginMainStart ? 0.f : margin.Top;
			item.marginMainEnd = item.hasAutoMarginMainEnd ? 0.f : margin.Bottom;
			item.marginCrossStart = margin.Left;
			item.marginCrossEnd = margin.Right;
		}
	}

	const Rectf containerPadding = mContainer->getPixelsContentOffset();

	if ( mWrap == CSSFlexWrap::NoWrap ) {
		Float maxContentMain = 0.f;
		for ( auto& item : items )
			maxContentMain += item.targetMainSize + item.marginMainStart + item.marginMainEnd;
		if ( items.size() > 1 )
			maxContentMain += (Float)( items.size() - 1 ) * mColumnGap;

		if ( mainAxis.horizontal ) {
			mMaxIntrinsicWidth = maxContentMain + containerPadding.Left + containerPadding.Right;

			// Min-content = largest single item's min-content contribution
			Float maxMinContent = 0.f;
			for ( auto& item : items ) {
				Float minContent = item.targetMainSize;
				if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) &&
					 item.widget->asType<UIHTMLWidget>()->getLayouter() ) {
					minContent = eemax( minContent, item.widget->asType<UIHTMLWidget>()
														->getLayouter()
														->getMinIntrinsicWidth() );
				} else {
					minContent = eemax( minContent, item.widget->getPixelsSize().getWidth() );
				}
				if ( minContent < 0.f )
					minContent = 0.f;
				maxMinContent =
					eemax( maxMinContent, minContent + item.marginMainStart + item.marginMainEnd );
			}
			mMinIntrinsicWidth = maxMinContent + containerPadding.Left + containerPadding.Right;
		} else {
			Float maxItemWidth = 0.f;
			for ( auto& item : items )
				maxItemWidth = eemax( maxItemWidth, item.targetMainSize + item.marginCrossStart +
														item.marginCrossEnd );
			mMaxIntrinsicWidth = maxItemWidth + containerPadding.Left + containerPadding.Right;

			Float maxMinContent = 0.f;
			for ( auto& item : items ) {
				Float minContent = item.targetMainSize;
				if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) &&
					 item.widget->asType<UIHTMLWidget>()->getLayouter() ) {
					minContent = eemax( minContent, item.widget->asType<UIHTMLWidget>()
														->getLayouter()
														->getMinIntrinsicWidth() );
				} else {
					minContent = eemax( minContent, item.widget->getPixelsSize().getWidth() );
				}
				if ( minContent < 0.f )
					minContent = 0.f;
				maxMinContent = eemax( maxMinContent,
									   minContent + item.marginCrossStart + item.marginCrossEnd );
			}
			mMinIntrinsicWidth = maxMinContent + containerPadding.Left + containerPadding.Right;
		}
	} else {
		if ( mainAxis.horizontal ) {
			Float maxLineWidth = 0.f;
			Float lineSum = 0.f;
			for ( auto& item : items ) {
				lineSum += ( lineSum > 0.f ? mColumnGap : 0.f ) + item.targetMainSize +
						   item.marginMainStart + item.marginMainEnd;
			}
			maxLineWidth = lineSum;
			mMaxIntrinsicWidth = maxLineWidth + containerPadding.Left + containerPadding.Right;

			// Min-content width for multi-line containers = largest single item's
			// min-content contribution (each line can be as narrow as its widest item).
			Float maxMinContent = 0.f;
			for ( auto& item : items ) {
				Float minContent = item.targetMainSize;
				if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) &&
					 item.widget->asType<UIHTMLWidget>()->getLayouter() ) {
					minContent = eemax( minContent, item.widget->asType<UIHTMLWidget>()
														->getLayouter()
														->getMinIntrinsicWidth() );
				} else {
					minContent = eemax( minContent, item.widget->getPixelsSize().getWidth() );
				}
				if ( minContent < 0.f )
					minContent = 0.f;
				maxMinContent =
					eemax( maxMinContent, minContent + item.marginMainStart + item.marginMainEnd );
			}
			mMinIntrinsicWidth = maxMinContent + containerPadding.Left + containerPadding.Right;
		} else {
			Float maxItemWidth = 0.f;
			for ( auto& item : items )
				maxItemWidth = eemax( maxItemWidth, item.targetMainSize + item.marginCrossStart +
														item.marginCrossEnd );
			mMaxIntrinsicWidth = maxItemWidth + containerPadding.Left + containerPadding.Right;

			Float maxMinContent = 0.f;
			for ( auto& item : items ) {
				Float minContent = item.targetMainSize;
				if ( item.widget->isType( UI_TYPE_HTML_WIDGET ) &&
					 item.widget->asType<UIHTMLWidget>()->getLayouter() ) {
					minContent = eemax( minContent, item.widget->asType<UIHTMLWidget>()
														->getLayouter()
														->getMinIntrinsicWidth() );
				} else {
					minContent = eemax( minContent, item.widget->getPixelsSize().getWidth() );
				}
				if ( minContent < 0.f )
					minContent = 0.f;
				maxMinContent = eemax( maxMinContent,
									   minContent + item.marginCrossStart + item.marginCrossEnd );
			}
			mMinIntrinsicWidth = maxMinContent + containerPadding.Left + containerPadding.Right;
		}
	}

	mIntrinsicWidthsDirty = false;
}

Float FlexLayouter::getMinIntrinsicWidth() {
	computeIntrinsicWidths();
	return mMinIntrinsicWidth;
}

Float FlexLayouter::getMaxIntrinsicWidth() {
	computeIntrinsicWidths();
	return mMaxIntrinsicWidth;
}

}} // namespace EE::UI
