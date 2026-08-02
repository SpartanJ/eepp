#include <algorithm>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/flexlayouter.hpp>
#include <eepp/ui/gridlayouter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uistyle.hpp>

namespace EE { namespace UI {

static bool isDataPropertyName( std::string_view name ) {
	return String::istartsWith( String::trim( name ), "data-" );
}

static bool isNormalizedDataPropertyName( std::string_view name ) {
	if ( !String::startsWith( name, "data-" ) || String::trim( name ).size() != name.size() )
		return false;

	for ( auto c : name ) {
		if ( c >= 'A' && c <= 'Z' )
			return false;
	}

	return true;
}

static std::string normalizeDataPropertyName( std::string_view name ) {
	auto trimmedName = String::trim( name );
	std::string normalizedName( trimmedName );
	String::toLowerInPlace( normalizedName );
	return normalizedName;
}

static bool isAtomicInlineAutoDisplay( CSSDisplay display ) {
	return display == CSSDisplay::InlineBlock || display == CSSDisplay::InlineFlex ||
		   display == CSSDisplay::InlineGrid;
}

static UIWidget* getHTMLContainingBlockParent( const UIWidget* widget ) {
	Node* parent = widget->getParent();
	while ( parent && parent->isWidget() && parent->isType( UI_TYPE_HTML_WIDGET ) &&
			static_cast<UIHTMLWidget*>( parent )->isInline() )
		parent = parent->getParent();
	return parent && parent->isWidget() ? parent->asType<UIWidget>() : nullptr;
}

static bool hasDefiniteCSSHeight( UIWidget* widget ) {
	if ( !widget || widget->getLayoutHeightPolicy() != SizePolicy::Fixed )
		return false;

	auto* style = widget->getUIStyle();
	const auto* height = style ? style->getProperty( PropertyId::Height ) : nullptr;
	return !( height && StyleSheetLength::isPercentage( height->value() ) );
}

static CSSBaselineAlignValue parseBaselineAlign( UIHTMLWidget* widget,
												 const StyleSheetProperty& property ) {
	std::string_view val = property.value();
	auto isSpace = []( char c ) {
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
	};
	while ( !val.empty() && isSpace( val.front() ) )
		val.remove_prefix( 1 );
	while ( !val.empty() && isSpace( val.back() ) )
		val.remove_suffix( 1 );
	if ( val.empty() )
		return {};

	if ( CSS::StyleSheetLength::isPercentage( val ) ) {
		CSSBaselineAlignValue out;
		out.type = CSSBaselineAlignment::Percentage;
		out.value = CSS::StyleSheetLength::fromString( std::string( val ) ).getValue();
		return out;
	}

	if ( CSS::StyleSheetLength::isLength( val ) ) {
		CSSBaselineAlignValue out;
		out.type = CSSBaselineAlignment::Length;
		out.value =
			widget->lengthFromValue( std::string( val ), CSS::PropertyRelativeTarget::None );
		return out;
	}

	return CSSBaselineAlignmentHelper::fromKeyword( val );
}

UIHTMLWidget* UIHTMLWidget::New() {
	return eeNew( UIHTMLWidget, () );
}

bool UIHTMLWidget::resolvePercentageSize( UIWidget* widget ) {
	if ( widget == nullptr || !( widget->getFlags() & UI_HTML_ELEMENT ) ||
		 widget->getUIStyle() == nullptr )
		return false;

	const auto* width = widget->getUIStyle()->getProperty( PropertyId::Width );
	const auto* height = widget->getUIStyle()->getProperty( PropertyId::Height );
	const bool percentageWidth = width && StyleSheetLength::isPercentage( width->value() );
	const bool percentageHeight = height && StyleSheetLength::isPercentage( height->value() );
	if ( !percentageWidth && !percentageHeight )
		return false;

	UIWidget* containingBlock = getHTMLContainingBlockParent( widget );
	if ( containingBlock == nullptr )
		return false;

	const Rectf contentOffset = containingBlock->getPixelsContentOffset();
	const Sizef containingSize = containingBlock->getPixelsSize();
	const Float contentWidth =
		eemax( 0.f, containingSize.getWidth() - contentOffset.Left - contentOffset.Right );
	const Float contentHeight =
		eemax( 0.f, containingSize.getHeight() - contentOffset.Top - contentOffset.Bottom );
	Sizef size = widget->getPixelsSize();
	bool changed = false;

	if ( percentageWidth ) {
		if ( widget->getLayoutWidthPolicy() != SizePolicy::Fixed ) {
			widget->setLayoutWidthPolicy( SizePolicy::Fixed );
			changed = true;
		}
		Float resolved = widget->cssResolvedLengthToBorderBoxWidth(
			widget->convertLength( width->asStyleSheetLength(), contentWidth ) );
		if ( size.getWidth() != resolved ) {
			size.setWidth( resolved );
			changed = true;
		}
	}

	if ( percentageHeight ) {
		if ( hasDefiniteCSSHeight( containingBlock ) ) {
			if ( widget->getLayoutHeightPolicy() != SizePolicy::Fixed ) {
				widget->setLayoutHeightPolicy( SizePolicy::Fixed );
				changed = true;
			}
			Float resolved = widget->cssResolvedLengthToBorderBoxHeight(
				widget->convertLength( height->asStyleSheetLength(), contentHeight ) );
			if ( size.getHeight() != resolved ) {
				size.setHeight( resolved );
				changed = true;
			}
		} else if ( widget->getLayoutHeightPolicy() != SizePolicy::WrapContent ) {
			widget->setLayoutHeightPolicy( SizePolicy::WrapContent );
			changed = true;
		}
	}

	if ( size != widget->getPixelsSize() ) {
		if ( widget->isType( UI_TYPE_HTML_WIDGET ) )
			widget->asType<UIHTMLWidget>()->setInternalPixelsSize( size );
		else
			widget->setPixelsSize( size );
	}

	return changed;
}

UIHTMLWidget::UIHTMLWidget( const std::string& tag ) : UILayout( tag ) {
	mFlags |= UI_HTML_ELEMENT;
}

UIHTMLWidget::~UIHTMLWidget() {
	if ( mScrollTarget && mScrollCb )
		mScrollTarget->removeEventListener( mScrollCb );
	eeSAFE_DELETE( mLayouter );
	eeSAFE_DELETE( mFlexState );
	eeSAFE_DELETE( mGridState );
}

UILayouter* UIHTMLWidget::getLayouter() {
	if ( nullptr == mLayouter )
		mLayouter = UILayouterManager::create( mDisplay, this );
	return mLayouter;
}

bool UIHTMLWidget::isPacking() const {
	UILayouter* layouter = const_cast<UIHTMLWidget*>( this )->getLayouter();
	if ( layouter )
		return layouter->isPacking();
	return UILayout::isPacking();
}

void UIHTMLWidget::onDisplayChange() {
	eeSAFE_DELETE( mLayouter );
	getLayouter();
	notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
}

void UIHTMLWidget::setDisplay( CSSDisplay display ) {
	if ( mDisplay != display ) {
		auto oldDisplay = mDisplay;
		mDisplay = display;
		mNodeFlags |= NODE_FLAG_OVER_FIND_ALLOWED;

		if ( mDisplay == CSSDisplay::InlineBlock || mDisplay == CSSDisplay::Inline ||
			 mDisplay == CSSDisplay::InlineFlex || mDisplay == CSSDisplay::InlineGrid ) {
			if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
				setLayoutWidthPolicy( SizePolicy::WrapContent );
		} else if ( mDisplay == CSSDisplay::Block || mDisplay == CSSDisplay::ListItem ||
					mDisplay == CSSDisplay::Flex || mDisplay == CSSDisplay::Grid ) {
			if ( getLayoutWidthPolicy() == SizePolicy::WrapContent &&
				 mPosition != CSSPosition::Absolute && mPosition != CSSPosition::Fixed )
				setLayoutWidthPolicy( SizePolicy::MatchParent );
		} else if ( mDisplay == CSSDisplay::None ) {
			mNodeFlags &= ~NODE_FLAG_OVER_FIND_ALLOWED;
			setVisible( false );
		}

		if ( oldDisplay == CSSDisplay::None )
			setVisible( true );

		if ( isAtomicInlineAutoDisplay( mDisplay ) ) {
			Sizef size( getPixelsSize() );
			bool resetSize = false;
			if ( getLayoutWidthPolicy() == SizePolicy::WrapContent && size.getWidth() != 0.f ) {
				size.setWidth( 0.f );
				resetSize = true;
			}
			if ( getLayoutHeightPolicy() == SizePolicy::WrapContent && size.getHeight() != 0.f ) {
				size.setHeight( 0.f );
				resetSize = true;
			}
			if ( resetSize )
				setInternalPixelsSize( size );
		}

		onDisplayChange();
	}
}

Float UIHTMLWidget::getBaseline() const {
	if ( mLayouter ) {
		if ( isFlex() ) {
			auto* flex = reinterpret_cast<FlexLayouter*>( mLayouter );
			return flex->getBaseline();
		}
		if ( isGrid() ) {
			auto* grid = reinterpret_cast<GridLayouter*>( mLayouter );
			return grid->getBaseline();
		}
	}
	if ( mBaselineAlign.type == CSSBaselineAlignment::Length )
		return mBaselineAlign.value;
	return 0.f;
}

Float UIHTMLWidget::getContainingBlockContentWidth() const {
	UIWidget* parent = getHTMLContainingBlockParent( this );
	if ( !parent )
		return 0.f;

	Float width = parent->getPixelsSize().getWidth();
	Rectf contentOffset = parent->getPixelsContentOffset();
	width -= contentOffset.Left + contentOffset.Right;
	return eemax( 0.f, width );
}

Float UIHTMLWidget::getContainingBlockContentHeight() const {
	UIWidget* parent = getHTMLContainingBlockParent( this );
	if ( !parent )
		return 0.f;

	Float height = parent->getPixelsSize().getHeight();
	Rectf contentOffset = parent->getPixelsContentOffset();
	height -= contentOffset.Top + contentOffset.Bottom;
	return eemax( 0.f, height );
}

Float UIHTMLWidget::lengthFromValueForCSS( const StyleSheetProperty& property,
										   const Float& defaultValue ) const {
	if ( property.getPropertyDefinition() ) {
		switch ( property.getPropertyDefinition()->getRelativeTarget() ) {
			case PropertyRelativeTarget::ContainingBlockWidth:
				return convertLength(
					StyleSheetLength::fromString( property.getValue(), defaultValue ),
					getContainingBlockContentWidth() );
			case PropertyRelativeTarget::ContainingBlockHeight:
				return convertLength(
					StyleSheetLength::fromString( property.getValue(), defaultValue ),
					getContainingBlockContentHeight() );
			default:
				break;
		}
	}
	return lengthFromValue( property, defaultValue );
}

Float UIHTMLWidget::cssResolvedLengthToBorderBoxWidth( const Float& resolvedLength ) const {
	if ( mBoxSizing == CSSBoxSizing::BorderBox )
		return resolvedLength;
	Rectf contentOffset = getPixelsContentOffset();
	return resolvedLength + contentOffset.Left + contentOffset.Right;
}

Float UIHTMLWidget::cssResolvedLengthToBorderBoxHeight( const Float& resolvedLength ) const {
	if ( mBoxSizing == CSSBoxSizing::BorderBox )
		return resolvedLength;
	Rectf contentOffset = getPixelsContentOffset();
	return resolvedLength + contentOffset.Top + contentOffset.Bottom;
}

Float UIHTMLWidget::cssWidthPropertyToBorderBoxWidth( const StyleSheetProperty& property ) const {
	return cssResolvedLengthToBorderBoxWidth( lengthFromValueForCSS( property ) );
}

Float UIHTMLWidget::cssHeightPropertyToBorderBoxHeight( const StyleSheetProperty& property ) const {
	return cssResolvedLengthToBorderBoxHeight( lengthFromValueForCSS( property ) );
}

void UIHTMLWidget::updateCSSContentBoxFixedSize() {
	if ( getUIStyle() == nullptr )
		return;

	Sizef size( getPixelsSize() );
	bool changed = false;

	if ( getLayoutWidthPolicy() == SizePolicy::Fixed ) {
		const auto* width = getUIStyle()->getProperty( PropertyId::Width );
		if ( width && width->value() != "auto" ) {
			size.setWidth( cssWidthPropertyToBorderBoxWidth( *width ) );
			changed = true;
		}
	}

	if ( getLayoutHeightPolicy() == SizePolicy::Fixed ) {
		const auto* height = getUIStyle()->getProperty( PropertyId::Height );
		if ( height && height->value() != "auto" ) {
			size.setHeight( cssHeightPropertyToBorderBoxHeight( *height ) );
			changed = true;
		}
	}

	if ( changed )
		setPixelsSize( size );
}

void UIHTMLWidget::setBoxSizing( CSSBoxSizing boxSizing ) {
	if ( mBoxSizing != boxSizing ) {
		mBoxSizing = boxSizing;
		updateCSSContentBoxFixedSize();
		notifyLayoutAttrChange( LayoutInvalidation::Self );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
	}
}

void UIHTMLWidget::setVisibility( CSSVisibility val ) {
	if ( mVisibility != val ) {
		mVisibility = val;
		setVisible( val != CSSVisibility::Hidden );
	}
}

void UIHTMLWidget::setCSSPosition( CSSPosition position ) {
	if ( mPosition != position ) {
		mPosition = position;
		if ( position == CSSPosition::Absolute || position == CSSPosition::Fixed ) {
			// Out-of-flow elements should not stretch to their containing block
			// until updateOutOfFlowPosition() computes the correct size from CSS
			// insets (top/left/right/bottom). Switch both width and height policies
			// to WrapContent to prevent intermediate MatchParent growth.
			if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
				setLayoutWidthPolicy( SizePolicy::WrapContent );
			if ( getLayoutHeightPolicy() == SizePolicy::MatchParent )
				setLayoutHeightPolicy( SizePolicy::WrapContent );
		}
		updateScrollListeners();
		onPositionChange();
	}
}

void UIHTMLWidget::setCSSFloat( CSSFloat cssFloat ) {
	if ( mFloat != cssFloat ) {
		mFloat = cssFloat;
		// A width:auto block normally fills its containing block, while a float uses the CSS
		// shrink-to-fit width. Represent that used-width distinction with WrapContent so the
		// floated box's own layouter cannot stretch it back after its parent measured it.
		if ( mFloat != CSSFloat::None && getLayoutWidthPolicy() == SizePolicy::MatchParent )
			setLayoutWidthPolicy( SizePolicy::WrapContent );
		else if ( mFloat == CSSFloat::None && getLayoutWidthPolicy() == SizePolicy::WrapContent &&
				  ( mDisplay == CSSDisplay::Block || mDisplay == CSSDisplay::ListItem ) &&
				  ( getUIStyle() == nullptr ||
					getUIStyle()->getProperty( PropertyId::Width ) == nullptr ) &&
				  mPosition != CSSPosition::Absolute && mPosition != CSSPosition::Fixed )
			setLayoutWidthPolicy( SizePolicy::MatchParent );

		// Float changes the used display type (CSS 2.1 section 9.7), so an inline element must
		// exchange InlineLayouter for BlockLayouter and vice versa when float is toggled.
		eeSAFE_DELETE( mLayouter );
		getLayouter();
		notifyLayoutAttrChange(
			toLayoutInvalidationFlags( LayoutInvalidationReason::Style ) |
			toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext ) |
			toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
}

void UIHTMLWidget::setCSSClear( CSSClear cssClear ) {
	if ( mClear != cssClear ) {
		mClear = cssClear;
		notifyLayoutAttrChange(
			toLayoutInvalidationFlags( LayoutInvalidationReason::Style ) |
			toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext ) |
			toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
}

Rectf UIHTMLWidget::getNormalFlowLayoutPixelsMargin() const {
	Rectf margin = getLayoutPixelsMargin();
	if ( hasLayoutMarginTopAuto() )
		margin.Top = 0.f;
	if ( hasLayoutMarginBottomAuto() )
		margin.Bottom = 0.f;
	return margin;
}

Rectf UIHTMLWidget::getFormattingContextLayoutPixelsMargin( UIWidget* widget ) {
	bool resolveHorizontalAutoMargins = widget->getLayoutWidthPolicy() == SizePolicy::MatchParent;
	if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
		auto* htmlWidget = widget->asType<UIHTMLWidget>();
		resolveHorizontalAutoMargins = !htmlWidget->isOutOfFlow() &&
									   htmlWidget->getCSSFloat() == CSSFloat::None &&
									   !widget->isInlineDisplay();
	}

	if ( resolveHorizontalAutoMargins && widget->hasLayoutMarginAuto() )
		widget->updateLayoutMarginAuto();

	Rectf margin = widget->isType( UI_TYPE_HTML_WIDGET )
					   ? widget->asType<UIHTMLWidget>()->getNormalFlowLayoutPixelsMargin()
					   : widget->getLayoutPixelsMargin();
	if ( !resolveHorizontalAutoMargins ) {
		if ( widget->hasLayoutMarginLeftAuto() )
			margin.Left = 0.f;
		if ( widget->hasLayoutMarginRightAuto() )
			margin.Right = 0.f;
		if ( widget->hasLayoutMarginTopAuto() )
			margin.Top = 0.f;
		if ( widget->hasLayoutMarginBottomAuto() )
			margin.Bottom = 0.f;
	}
	return margin;
}

void UIHTMLWidget::setBaselineAlign( const CSSBaselineAlignValue& baselineAlign ) {
	if ( mBaselineAlign != baselineAlign ) {
		mBaselineAlign = baselineAlign;
		notifyLayoutAttrChange(
			toLayoutInvalidationFlags( LayoutInvalidationReason::Style ) |
			toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) );
	}
}

void UIHTMLWidget::setOffsets( const Rectf& offsets ) {
	if ( mOffsets != offsets ) {
		mOffsets = offsets;
		mTopEq = String::fromFloat( offsets.Top, "dp" );
		mLeftEq = String::fromFloat( offsets.Left, "dp" );
		mRightEq = String::fromFloat( offsets.Right, "dp" );
		mBottomEq = String::fromFloat( offsets.Bottom, "dp" );
		notifyLayoutAttrChange( LayoutInvalidation::OutOfFlow );
	}
}

void UIHTMLWidget::setZIndex( int zIndex ) {
	mZIndex = zIndex;
	Node* p = getParent();
	if ( p && p->isType( UI_TYPE_HTML_WIDGET ) )
		p->asType<UIHTMLWidget>()->updateZIndexSortFlag();
}

void UIHTMLWidget::setNeedsOrderSort( bool val ) {
	mNeedsOrderSort = val;
}

void UIHTMLWidget::updateZIndexSortFlag() {
	bool needs = false;
	for ( Node* child = getFirstChild(); child; child = child->getNextNode() ) {
		if ( child->isType( UI_TYPE_HTML_WIDGET ) &&
			 child->asType<UIHTMLWidget>()->getZIndex() != 0 ) {
			needs = true;
			break;
		}
	}
	mNeedsZIndexSort = needs;
}

void UIHTMLWidget::onChildCountChange( Node* child, const bool& removed ) {
	UILayout::onChildCountChange( child, removed );

	if ( !removed )
		updateZIndexSortFlag();
	else if ( child->isType( UI_TYPE_HTML_WIDGET ) &&
			  child->asType<UIHTMLWidget>()->getZIndex() != 0 )
		updateZIndexSortFlag();
}

void UIHTMLWidget::buildDrawOrderVector( SmallVector<Node*, 127>& out ) const {
	bool flexSort = false;
	bool directionReverse = false;

	if ( isFlex() ) {
		CSSFlexDirection dir = getFlexDirection();
		directionReverse =
			dir == CSSFlexDirection::RowReverse || dir == CSSFlexDirection::ColumnReverse;
		flexSort = mNeedsOrderSort || directionReverse;
	}

	for ( Node* child = getFirstChild(); child; child = child->getNextNode() )
		out.push_back( child );

	if ( flexSort && mNeedsOrderSort ) {
		std::stable_sort( out.begin(), out.end(), []( Node* a, Node* b ) {
			int aOrder = ( a->isWidget() && a->isType( UI_TYPE_HTML_WIDGET ) )
							 ? a->asType<UIHTMLWidget>()->getOrder()
							 : 0;
			int bOrder = ( b->isWidget() && b->isType( UI_TYPE_HTML_WIDGET ) )
							 ? b->asType<UIHTMLWidget>()->getOrder()
							 : 0;
			return aOrder < bOrder;
		} );
	}

	if ( flexSort && directionReverse )
		std::reverse( out.begin(), out.end() );

	if ( mNeedsZIndexSort ) {
		std::stable_sort( out.begin(), out.end(), []( Node* a, Node* b ) {
			int aZ =
				( a->isType( UI_TYPE_HTML_WIDGET ) ) ? a->asType<UIHTMLWidget>()->getZIndex() : 0;
			int bZ =
				( b->isType( UI_TYPE_HTML_WIDGET ) ) ? b->asType<UIHTMLWidget>()->getZIndex() : 0;
			return aZ < bZ;
		} );
	}
}

void UIHTMLWidget::drawChildren() {
	bool needsSort = mNeedsOrderSort || mNeedsZIndexSort;

	if ( isFlex() ) {
		CSSFlexDirection dir = getFlexDirection();
		if ( dir == CSSFlexDirection::RowReverse || dir == CSSFlexDirection::ColumnReverse )
			needsSort = true;
	}

	if ( !needsSort ) {
		UILayout::drawChildren();
		return;
	}

	SmallVector<Node*, 127> sortedChildren;
	buildDrawOrderVector( sortedChildren );

	for ( auto* child : sortedChildren ) {
		if ( child->isVisible() )
			child->nodeDraw();
	}
}

Node* UIHTMLWidget::overFind( const Vector2f& point ) {
	if ( !mNeedsOrderSort && !mNeedsZIndexSort )
		return UILayout::overFind( point );

	Node* pOver = nullptr;

	if ( ( mNodeFlags & NODE_FLAG_OVER_FIND_ALLOWED ) && mEnabled && mVisible ) {
		updateWorldPolygon();

		if ( mWorldBounds.contains( point ) && mPoly.pointInside( point ) ) {
			writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
			mSceneNode->addMouseOverNode( this );

			SmallVector<Node*, 127> sortedChildren;
			buildDrawOrderVector( sortedChildren );

			// Iterate last-to-first: highest z-index = topmost = hit first
			for ( auto it = sortedChildren.rbegin(); it != sortedChildren.rend(); ++it ) {
				Node* childOver = ( *it )->overFind( point );
				if ( childOver ) {
					pOver = childOver;
					break;
				}
			}

			if ( !pOver )
				pOver = this;
		}
	}

	return pOver;
}

void UIHTMLWidget::setFlexDirection( CSSFlexDirection val ) {
	auto* fs = ensureFlexState();
	if ( fs->direction != val ) {
		fs->direction = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setFlexWrap( CSSFlexWrap val ) {
	auto* fs = ensureFlexState();
	if ( fs->wrap != val ) {
		fs->wrap = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setJustifyContent( CSSJustifyContent val ) {
	auto* fs = ensureFlexState();
	if ( fs->justifyContent != val ) {
		fs->justifyContent = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setAlignItems( CSSAlignItems val ) {
	auto* fs = ensureFlexState();
	if ( fs->alignItems != val ) {
		fs->alignItems = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setAlignContent( CSSAlignContent val ) {
	auto* fs = ensureFlexState();
	if ( fs->alignContent != val ) {
		fs->alignContent = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setAlignSelf( CSSAlignSelf val ) {
	auto* fs = ensureFlexState();
	if ( fs->alignSelf != val ) {
		fs->alignSelf = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setFlexGrow( Float val ) {
	auto* fs = ensureFlexState();
	if ( fs->flexGrow != val ) {
		fs->flexGrow = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setFlexShrink( Float val ) {
	auto* fs = ensureFlexState();
	if ( fs->flexShrink != val ) {
		fs->flexShrink = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setFlexBasis( const std::string& val ) {
	auto* fs = ensureFlexState();
	if ( fs->flexBasis != val ) {
		fs->flexBasis = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setOrder( int val ) {
	auto* fs = ensureFlexState();
	if ( fs->order != val ) {
		fs->order = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setRowGap( const std::string& val ) {
	auto* fs = ensureFlexState();
	if ( fs->rowGap != val ) {
		fs->rowGap = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setColumnGap( const std::string& val ) {
	auto* fs = ensureFlexState();
	if ( fs->columnGap != val ) {
		fs->columnGap = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridTemplateRows( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->templateRows != val ) {
		gs->templateRows = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridTemplateColumns( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->templateColumns != val ) {
		gs->templateColumns = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridTemplateAreas( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->templateAreas != val ) {
		gs->templateAreas = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridAutoRows( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->autoRows != val ) {
		gs->autoRows = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridAutoColumns( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->autoColumns != val ) {
		gs->autoColumns = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridAutoFlow( CSSGridAutoFlow val ) {
	auto* gs = ensureGridState();
	if ( gs->autoFlow != val ) {
		gs->autoFlow = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridAutoFlowDense( bool val ) {
	auto* gs = ensureGridState();
	if ( gs->autoFlowDense != val ) {
		gs->autoFlowDense = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridRowStart( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->rowStart != val ) {
		gs->rowStart = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridRowEnd( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->rowEnd != val ) {
		gs->rowEnd = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridColumnStart( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->columnStart != val ) {
		gs->columnStart = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridColumnEnd( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->columnEnd != val ) {
		gs->columnEnd = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setGridArea( const std::string& val ) {
	auto* gs = ensureGridState();
	if ( gs->area != val ) {
		gs->area = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setJustifyItems( CSSJustifyItems val ) {
	auto* gs = ensureGridState();
	if ( gs->justifyItems != val ) {
		gs->justifyItems = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

void UIHTMLWidget::setJustifySelf( CSSJustifySelf val ) {
	auto* gs = ensureGridState();
	if ( gs->justifySelf != val ) {
		gs->justifySelf = val;
		notifyLayoutAttrChange( LayoutInvalidation::ContainerLayout );
	}
}

std::vector<PropertyId> UIHTMLWidget::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::Display,
				   PropertyId::BoxSizing,
				   PropertyId::Position,
				   PropertyId::Float,
				   PropertyId::Clear,
				   PropertyId::Top,
				   PropertyId::Right,
				   PropertyId::Bottom,
				   PropertyId::Left,
				   PropertyId::ZIndex,
				   PropertyId::AlignmentBaseline,
				   PropertyId::FlexDirection,
				   PropertyId::FlexWrap,
				   PropertyId::FlexFlow,
				   PropertyId::JustifyContent,
				   PropertyId::AlignItems,
				   PropertyId::AlignContent,
				   PropertyId::AlignSelf,
				   PropertyId::FlexGrow,
				   PropertyId::FlexShrink,
				   PropertyId::FlexBasis,
				   PropertyId::Flex,
				   PropertyId::Order,
				   PropertyId::ColumnGap,
				   PropertyId::RowGap,
				   PropertyId::Gap,
				   PropertyId::GridTemplateRows,
				   PropertyId::GridTemplateColumns,
				   PropertyId::GridTemplateAreas,
				   PropertyId::GridTemplate,
				   PropertyId::GridAutoRows,
				   PropertyId::GridAutoColumns,
				   PropertyId::GridAutoFlow,
				   PropertyId::Grid,
				   PropertyId::GridRowStart,
				   PropertyId::GridRowEnd,
				   PropertyId::GridColumnStart,
				   PropertyId::GridColumnEnd,
				   PropertyId::GridRow,
				   PropertyId::GridColumn,
				   PropertyId::GridArea,
				   PropertyId::JustifyItems,
				   PropertyId::JustifySelf };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

std::string UIHTMLWidget::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& state ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Display:
			return CSSDisplayHelper::toString( mDisplay );
		case PropertyId::BoxSizing:
			return CSSBoxSizingHelper::toString( mBoxSizing );
		case PropertyId::Position:
			return CSSPositionHelper::toString( mPosition );
		case PropertyId::Float:
			return CSSFloatHelper::toString( mFloat );
		case PropertyId::Clear:
			return CSSClearHelper::toString( mClear );
		case PropertyId::Top:
			return mTopEq;
		case PropertyId::Right:
			return mRightEq;
		case PropertyId::Bottom:
			return mBottomEq;
		case PropertyId::Left:
			return mLeftEq;
		case PropertyId::ZIndex:
			return String::toString( mZIndex );
		case PropertyId::AlignmentBaseline:
			return std::string( CSSBaselineAlignmentHelper::toString( mBaselineAlign ) );
		case PropertyId::FlexDirection:
			return CSSFlexDirectionHelper::toString( getFlexDirection() );
		case PropertyId::FlexWrap:
			return CSSFlexWrapHelper::toString( getFlexWrap() );
		case PropertyId::JustifyContent:
			return CSSJustifyContentHelper::toString( getJustifyContent() );
		case PropertyId::AlignItems:
			return CSSAlignItemsHelper::toString( getAlignItems() );
		case PropertyId::AlignContent:
			return CSSAlignContentHelper::toString( getAlignContent() );
		case PropertyId::AlignSelf:
			return CSSAlignSelfHelper::toString( getAlignSelf() );
		case PropertyId::FlexGrow:
			return String::toString( getFlexGrow() );
		case PropertyId::FlexShrink:
			return String::toString( getFlexShrink() );
		case PropertyId::FlexBasis:
			return getFlexBasis();
		case PropertyId::Order:
			return String::toString( getOrder() );
		case PropertyId::RowGap:
			return getRowGap();
		case PropertyId::ColumnGap:
			return getColumnGap();
		case PropertyId::GridTemplateRows:
			return getGridTemplateRows();
		case PropertyId::GridTemplateColumns:
			return getGridTemplateColumns();
		case PropertyId::GridTemplateAreas:
			return getGridTemplateAreas();
		case PropertyId::GridAutoRows:
			return getGridAutoRows();
		case PropertyId::GridAutoColumns:
			return getGridAutoColumns();
		case PropertyId::GridAutoFlow:
			return CSSGridAutoFlowHelper::toString( getGridAutoFlow() );
		case PropertyId::GridRowStart:
			return getGridRowStart();
		case PropertyId::GridRowEnd:
			return getGridRowEnd();
		case PropertyId::GridColumnStart:
			return getGridColumnStart();
		case PropertyId::GridColumnEnd:
			return getGridColumnEnd();
		case PropertyId::GridArea:
			return getGridArea();
		case PropertyId::JustifyItems:
			return CSSJustifyItemsHelper::toString( getJustifyItems() );
		case PropertyId::JustifySelf:
			return CSSJustifySelfHelper::toString( getJustifySelf() );
		default:
			return UILayout::getPropertyString( propertyDef );
	}
}

bool UIHTMLWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	auto applyGridLineShorthand = []( const std::string& value, auto setStart, auto setEnd ) {
		size_t slash = value.find( '/' );
		if ( slash == std::string::npos ) {
			setStart( String::trim( value ) );
			setEnd( "auto" );
		} else {
			setStart( String::trim( value.substr( 0, slash ) ) );
			setEnd( String::trim( value.substr( slash + 1 ) ) );
		}
	};

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Display: {
			setDisplay( CSSDisplayHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::BoxSizing: {
			setBoxSizing( CSSBoxSizingHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Position: {
			setCSSPosition( CSSPositionHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Float: {
			setCSSFloat( CSSFloatHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Clear: {
			setCSSClear( CSSClearHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Visibility: {
			setVisibility( CSSVisibilityHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::Overflow:
			mOverflowCreatesBlockFormattingContext =
				!String::iequals( attribute.getValue(), "visible" );
			return UILayout::applyProperty( attribute );
		case PropertyId::MarginTop:
		case PropertyId::MarginRight:
		case PropertyId::MarginBottom:
		case PropertyId::MarginLeft: {
			Uint8 marginBit = 0;
			switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
				case PropertyId::MarginTop:
					marginBit = 1 << 0;
					break;
				case PropertyId::MarginRight:
					marginBit = 1 << 1;
					break;
				case PropertyId::MarginBottom:
					marginBit = 1 << 2;
					break;
				case PropertyId::MarginLeft:
					marginBit = 1 << 3;
					break;
				default:
					break;
			}
			if ( StyleSheetLength::isPercentage( attribute.value() ) )
				mPercentageMargins |= marginBit;
			else
				mPercentageMargins &= ~marginBit;
			return UILayout::applyProperty( attribute );
		}
		case PropertyId::Width:
		case PropertyId::Height:
		case PropertyId::PaddingLeft:
		case PropertyId::PaddingRight:
		case PropertyId::PaddingTop:
		case PropertyId::PaddingBottom:
		case PropertyId::BorderLeftWidth:
		case PropertyId::BorderRightWidth:
		case PropertyId::BorderTopWidth:
		case PropertyId::BorderBottomWidth: {
			bool applied = UILayout::applyProperty( attribute );
			updateCSSContentBoxFixedSize();
			return applied;
		}
		case PropertyId::ZIndex: {
			setZIndex( attribute.asInt() );
			return true;
		}
		case PropertyId::Top: {
			mTopEq = attribute.asString();
			notifyLayoutAttrChange( LayoutInvalidation::Self );
			return true;
		}
		case PropertyId::Right: {
			mRightEq = attribute.asString();
			notifyLayoutAttrChange( LayoutInvalidation::Self );
			return true;
		}
		case PropertyId::Bottom: {
			mBottomEq = attribute.asString();
			notifyLayoutAttrChange( LayoutInvalidation::Self );
			return true;
		}
		case PropertyId::Left: {
			mLeftEq = attribute.asString();
			notifyLayoutAttrChange( LayoutInvalidation::Self );
			return true;
		}
		case PropertyId::AlignmentBaseline: {
			setBaselineAlign( parseBaselineAlign( this, attribute ) );
			return true;
		}
		case PropertyId::FlexDirection: {
			setFlexDirection( CSSFlexDirectionHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::FlexWrap: {
			setFlexWrap( CSSFlexWrapHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::JustifyContent: {
			setJustifyContent( CSSJustifyContentHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::AlignItems: {
			setAlignItems( CSSAlignItemsHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::AlignContent: {
			setAlignContent( CSSAlignContentHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::AlignSelf: {
			setAlignSelf( CSSAlignSelfHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::FlexGrow: {
			setFlexGrow( attribute.asFloat() );
			return true;
		}
		case PropertyId::FlexShrink: {
			setFlexShrink( attribute.asFloat() );
			return true;
		}
		case PropertyId::FlexBasis: {
			setFlexBasis( attribute.asString() );
			return true;
		}
		case PropertyId::Order: {
			setOrder( (int)attribute.asFloat() );
			return true;
		}
		case PropertyId::RowGap: {
			setRowGap( attribute.asString() );
			return true;
		}
		case PropertyId::ColumnGap: {
			setColumnGap( attribute.asString() );
			return true;
		}
		case PropertyId::GridTemplateRows: {
			setGridTemplateRows( attribute.asString() );
			return true;
		}
		case PropertyId::GridTemplateColumns: {
			setGridTemplateColumns( attribute.asString() );
			return true;
		}
		case PropertyId::GridTemplateAreas: {
			setGridTemplateAreas( attribute.asString() );
			return true;
		}
		case PropertyId::GridAutoRows: {
			setGridAutoRows( attribute.asString() );
			return true;
		}
		case PropertyId::GridAutoColumns: {
			setGridAutoColumns( attribute.asString() );
			return true;
		}
		case PropertyId::GridAutoFlow: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );
			setGridAutoFlowDense( val.find( "dense" ) != std::string::npos );
			if ( val.find( "column" ) != std::string::npos )
				setGridAutoFlow( CSSGridAutoFlow::Column );
			else
				setGridAutoFlow( CSSGridAutoFlow::Row );
			return true;
		}
		case PropertyId::GridRowStart: {
			setGridRowStart( attribute.asString() );
			return true;
		}
		case PropertyId::GridRowEnd: {
			setGridRowEnd( attribute.asString() );
			return true;
		}
		case PropertyId::GridColumnStart: {
			setGridColumnStart( attribute.asString() );
			return true;
		}
		case PropertyId::GridColumnEnd: {
			setGridColumnEnd( attribute.asString() );
			return true;
		}
		case PropertyId::GridRow: {
			applyGridLineShorthand(
				attribute.asString(),
				[this]( const std::string& value ) { setGridRowStart( value ); },
				[this]( const std::string& value ) { setGridRowEnd( value ); } );
			return true;
		}
		case PropertyId::GridColumn: {
			applyGridLineShorthand(
				attribute.asString(),
				[this]( const std::string& value ) { setGridColumnStart( value ); },
				[this]( const std::string& value ) { setGridColumnEnd( value ); } );
			return true;
		}
		case PropertyId::GridArea: {
			setGridArea( attribute.asString() );
			return true;
		}
		case PropertyId::JustifyItems: {
			setJustifyItems( CSSJustifyItemsHelper::fromString( attribute.asString() ) );
			return true;
		}
		case PropertyId::JustifySelf: {
			setJustifySelf( CSSJustifySelfHelper::fromString( attribute.asString() ) );
			return true;
		}
		default:
			break;
	}

	return UILayout::applyProperty( attribute );
}

void UIHTMLWidget::updateLayout() {
	if ( getLayouter() )
		getLayouter()->updateLayout();
	else
		UILayout::updateLayout();

	positionOutOfFlowChildren();

	// The layouter (BlockLayouter) above sets size from content, but for
	// out-of-flow elements the size should be determined by CSS insets
	// (top/left/right/bottom). Re-apply the correct out-of-flow size after
	// the layouter has finished. Re-entrancy is prevented because
	// UIRichText::onSizeChange() skips tryUpdateLayout() when isOutOfFlow().
	if ( isOutOfFlow() )
		updateOutOfFlowPosition();

	mDirtyLayout = false;
}

UIWidget* UIHTMLWidget::getContainingBlock() {
	if ( mPosition == CSSPosition::Fixed ) {
		Node* parent = getParent();
		UIWidget* cb = parent && parent->isWidget() ? parent->asType<UIWidget>() : nullptr;
		while ( parent ) {
			if ( parent->isType( UI_TYPE_SCROLLVIEW ) ) {
				cb = parent->asType<UIWidget>();
				break;
			}
			if ( parent->isWidget() )
				cb = parent->asType<UIWidget>();
			parent = parent->getParent();
		}
		return cb;
	}

	Node* parent = getParent();
	UIWidget* lastWidget = nullptr;
	UIWidget* htmlWidget = nullptr;
	while ( parent ) {
		if ( parent->isWidget() ) {
			lastWidget = parent->asType<UIWidget>();
			if ( lastWidget->isType( UI_TYPE_HTML_WIDGET ) ) {
				if ( lastWidget->asType<UIHTMLWidget>()->getCSSPosition() != CSSPosition::Static ) {
					return lastWidget;
				}
				if ( lastWidget->isType( UI_TYPE_HTML_HTML ) ) {
					htmlWidget = lastWidget;
				}
			}
		}
		parent = parent->getParent();
	}
	return htmlWidget ? htmlWidget : lastWidget;
}

void UIHTMLWidget::positionOutOfFlowChildren() {
	Node* child = mChild;
	while ( child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_HTML_WIDGET ) ) {
			UIHTMLWidget* htmlChild = child->asType<UIHTMLWidget>();
			CSSPosition pos = htmlChild->getCSSPosition();
			if ( pos == CSSPosition::Absolute || pos == CSSPosition::Fixed ) {
				htmlChild->updateOutOfFlowPosition();
			}
		}
		child = child->getNextNode();
	}
}

void UIHTMLWidget::updateOutOfFlowPosition() {
	UIWidget* cb = getContainingBlock();
	if ( !cb )
		return;

	Rectf cbContentOffset = cb->getPixelsContentOffset();
	Float cbContentWidth =
		cb->getPixelsSize().getWidth() - cbContentOffset.Left - cbContentOffset.Right;
	Float cbContentHeight =
		cb->getPixelsSize().getHeight() - cbContentOffset.Top - cbContentOffset.Bottom;

	Rectf margin = getLayoutPixelsMargin();
	Float childWidth = getPixelsSize().getWidth();
	Float childHeight = getPixelsSize().getHeight();

	Float top = 0;
	Float left = 0;
	Float right = 0;
	Float bottom = 0;

	bool useTop = mTopEq != "auto";
	bool useBottom = mBottomEq != "auto";
	bool useLeft = mLeftEq != "auto";
	bool useRight = mRightEq != "auto";
	auto resetAutoMarginUnlessBothInsetsApply = [this]( Float& marginValue, PropertyId property,
														bool bothInsetsApply ) {
		if ( bothInsetsApply || !getUIStyle() )
			return;
		const auto* marginProperty = getUIStyle()->getProperty( property );
		if ( marginProperty && marginProperty->value() == "auto" )
			marginValue = 0;
	};
	// CSS Positioned Layout: an auto margin on an axis only absorbs free space when both
	// opposing insets participate in that axis's constraint equation. Otherwise it is zero.
	resetAutoMarginUnlessBothInsetsApply( margin.Top, PropertyId::MarginTop, useTop && useBottom );
	resetAutoMarginUnlessBothInsetsApply( margin.Bottom, PropertyId::MarginBottom,
										  useTop && useBottom );
	resetAutoMarginUnlessBothInsetsApply( margin.Left, PropertyId::MarginLeft,
										  useLeft && useRight );
	resetAutoMarginUnlessBothInsetsApply( margin.Right, PropertyId::MarginRight,
										  useLeft && useRight );

	// Per CSS §10.1: for absolutely positioned elements, percentage top/bottom
	// resolves against the containing block's height. If the containing block
	// does not have a definite height, the percentage computes to auto to
	// prevent circular dependencies.
	auto cbHasDefiniteHeight = [&]() {
		if ( cb->isType( UI_TYPE_HTML_HTML ) && cb->getUISceneNode() &&
			 cb->getUISceneNode()->getLayoutViewportPixelsSize().getHeight() > 0 )
			return true;
		if ( !cb->isLayout() )
			return true;
		auto* cbLayout = cb->asType<UILayout>();
		if ( cbLayout->getLayoutHeightPolicy() != SizePolicy::Fixed )
			return false;
		if ( cb->getUIStyle() ) {
			const auto* hprop = cb->getUIStyle()->getProperty( PropertyId::Height );
			if ( hprop && StyleSheetLength::isPercentage( hprop->value() ) )
				return false;
		}
		return true;
	};

	if ( useTop && StyleSheetLength::isPercentage( mTopEq ) && !cbHasDefiniteHeight() )
		useTop = false;
	if ( useBottom && StyleSheetLength::isPercentage( mBottomEq ) && !cbHasDefiniteHeight() )
		useBottom = false;

	if ( useLeft )
		left = lengthFromValue( mLeftEq, CSS::PropertyRelativeTarget::ContainingBlockWidth, 0 );
	if ( useRight )
		right = lengthFromValue( mRightEq, CSS::PropertyRelativeTarget::ContainingBlockWidth, 0 );

	if ( useTop )
		top = lengthFromValue( mTopEq, CSS::PropertyRelativeTarget::ContainingBlockHeight, 0 );
	if ( useBottom )
		bottom =
			lengthFromValue( mBottomEq, CSS::PropertyRelativeTarget::ContainingBlockHeight, 0 );

	Float finalWidth = childWidth;
	Float finalHeight = childHeight;

	if ( useLeft && useRight && getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		Float stretched = cbContentWidth - left - right - margin.Left - margin.Right;
		if ( stretched >= 0 )
			finalWidth = stretched;
	}

	if ( useTop && useBottom && getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
		Float stretched = cbContentHeight - top - bottom - margin.Top - margin.Bottom;
		if ( stretched >= 0 )
			finalHeight = stretched;
	}

	if ( finalWidth != childWidth || finalHeight != childHeight ) {
		setPixelsSize( finalWidth, finalHeight );
		childWidth = finalWidth;
		childHeight = finalHeight;
	}

	if ( !useLeft && useRight )
		left = cbContentWidth - childWidth - margin.Left - margin.Right - right;

	if ( !useTop && useBottom )
		top = cbContentHeight - childHeight - margin.Top - margin.Bottom - bottom;

	top += margin.Top;
	left += margin.Left;

	Vector2f cbPos( cbContentOffset.Left, cbContentOffset.Top );
	cbPos.x += left;
	cbPos.y += top;

	Vector2f worldPos = cb->convertToWorldSpace( cbPos );
	Vector2f localPos = getParent()->convertToNodeSpace( worldPos );
	setPixelsPosition( localPos );
}

void UIHTMLWidget::updateStickyPosition() {
	if ( !mScrollTarget )
		return;

	UIWidget* cb = getContainingBlock();
	if ( !cb )
		return;

	Vector2f baseWorldPos = getParent()->convertToWorldSpace( mStickyBasePos );

	Node* viewport = mScrollTarget->getParent();
	if ( !viewport )
		return;

	Vector2f posInViewport = viewport->convertToNodeSpace( baseWorldPos );

	Float topOffset = 0;
	bool useTop = mTopEq != "auto";
	if ( useTop )
		topOffset =
			lengthFromValue( mTopEq, CSS::PropertyRelativeTarget::ContainingBlockHeight, 0 );

	Float bottomOffset = 0;
	bool useBottom = mBottomEq != "auto";
	if ( useBottom )
		bottomOffset =
			lengthFromValue( mBottomEq, CSS::PropertyRelativeTarget::ContainingBlockHeight, 0 );

	Vector2f newPosInViewport = posInViewport;

	if ( useTop ) {
		if ( posInViewport.y < topOffset ) {
			newPosInViewport.y = topOffset;
		}
	}

	if ( useBottom ) {
		Float viewportHeight = viewport->getSize().getHeight();
		if ( posInViewport.y + getPixelsSize().getHeight() > viewportHeight - bottomOffset ) {
			newPosInViewport.y = viewportHeight - bottomOffset - getPixelsSize().getHeight();
		}
	}

	Vector2f cbWorldPos = cb->convertToWorldSpace( Vector2f( 0, 0 ) );
	Vector2f cbInViewport = viewport->convertToNodeSpace( cbWorldPos );
	Float cbBottomInViewport =
		cbInViewport.y + cb->getPixelsSize().getHeight() - cb->getPixelsPadding().Bottom;

	if ( newPosInViewport.y + getPixelsSize().getHeight() > cbBottomInViewport ) {
		newPosInViewport.y = cbBottomInViewport - getPixelsSize().getHeight();
	}

	if ( newPosInViewport.y < cbInViewport.y + cb->getPixelsPadding().Top ) {
		newPosInViewport.y = cbInViewport.y + cb->getPixelsPadding().Top;
	}

	if ( newPosInViewport != posInViewport ) {
		Vector2f newWorldPos = viewport->convertToWorldSpace( newPosInViewport );
		Vector2f newLocalPos = getParent()->convertToNodeSpace( newWorldPos );

		mIsUpdatingScroll = true;
		setPixelsPosition( newLocalPos );
		mIsUpdatingScroll = false;
	} else {
		mIsUpdatingScroll = true;
		setPixelsPosition( mStickyBasePos );
		mIsUpdatingScroll = false;
	}
}

void UIHTMLWidget::updateScrollListeners() {
	if ( mScrollTarget ) {
		if ( mScrollCb ) {
			mScrollTarget->removeEventListener( mScrollCb );
			mScrollCb = 0;
		}
		mScrollTarget = nullptr;
	}

	if ( mPosition == CSSPosition::Fixed || mPosition == CSSPosition::Sticky ) {
		Node* parent = getParent();
		while ( parent ) {
			if ( parent->isType( UI_TYPE_SCROLLVIEW ) ) {
				mScrollTarget = parent->asType<UIScrollView>()->getScrollView();
				break;
			}
			parent = parent->getParent();
		}

		if ( mScrollTarget ) {
			mScrollCb = mScrollTarget->on( Event::OnPositionChange, [this]( const Event* ) {
				onScrollTargetPositionChange();
			} );
		}
	}
}

void UIHTMLWidget::onParentChange() {
	UILayout::onParentChange();
	updateScrollListeners();
}

void UIHTMLWidget::onParentSizeChange( const Vector2f& sizeChange ) {
	UILayout::onParentSizeChange( sizeChange );

	if ( mPercentageMargins == 0 || !getUIStyle() )
		return;

	static constexpr PropertyId MarginProperties[] = {
		PropertyId::MarginTop,
		PropertyId::MarginRight,
		PropertyId::MarginBottom,
		PropertyId::MarginLeft,
	};
	for ( Uint32 i = 0; i < 4; ++i ) {
		if ( !( mPercentageMargins & ( 1 << i ) ) )
			continue;
		PropertyId propertyId = MarginProperties[i];
		const StyleSheetProperty* property = getUIStyle()->getProperty( propertyId );
		if ( property )
			applyProperty( *property );
	}
}

void UIHTMLWidget::onPositionChange() {
	UILayout::onPositionChange();
	if ( mPosition == CSSPosition::Sticky && !mIsUpdatingScroll ) {
		mStickyBasePos = getPixelsPosition();
		updateStickyPosition();
	}
}

void UIHTMLWidget::onScrollTargetPositionChange() {
	if ( mPosition == CSSPosition::Fixed ) {
		updateOutOfFlowPosition();
	} else if ( mPosition == CSSPosition::Sticky ) {
		updateStickyPosition();
	}
}

void UIHTMLWidget::invalidateIntrinsicSize() {
	if ( mLayouter )
		mLayouter->invalidateIntrinsicWidths();
	UIWidget::invalidateIntrinsicSize();
}

bool UIHTMLWidget::establishesBlockFormattingContext() const {
	if ( mFloat != CSSFloat::None || isOutOfFlow() || mDisplay == CSSDisplay::InlineBlock )
		return true;

	return mOverflowCreatesBlockFormattingContext;
}

bool UIHTMLWidget::hasDataProperty( const std::string& name ) const {
	if ( isNormalizedDataPropertyName( name ) )
		return mDataProperties.find( name ) != mDataProperties.end();
	return mDataProperties.find( normalizeDataPropertyName( name ) ) != mDataProperties.end();
}

const StyleSheetProperty* UIHTMLWidget::getDataProperty( const std::string& name ) const {
	auto it = isNormalizedDataPropertyName( name )
				  ? mDataProperties.find( name )
				  : mDataProperties.find( normalizeDataPropertyName( name ) );
	return it != mDataProperties.end() ? &it->second : nullptr;
}

std::string UIHTMLWidget::getDataPropertyString( const std::string& name,
												 const std::string& defaultValue ) const {
	const StyleSheetProperty* property = getDataProperty( name );
	return property ? property->value() : defaultValue;
}

void UIHTMLWidget::setDataProperty( const StyleSheetProperty& property ) {
	const auto& name = property.getName();
	if ( isDataPropertyName( name ) )
		mDataProperties[isNormalizedDataPropertyName( name ) ? name
															 : normalizeDataPropertyName( name )] =
			property;
}

void UIHTMLWidget::setDataProperty( const std::string& name, const std::string& value ) {
	if ( !isDataPropertyName( name ) )
		return;

	std::string normalizedName =
		isNormalizedDataPropertyName( name ) ? name : normalizeDataPropertyName( name );
	mDataProperties[normalizedName] = StyleSheetProperty( normalizedName, value, false );
}

void UIHTMLWidget::removeDataProperty( const std::string& name ) {
	if ( isNormalizedDataPropertyName( name ) ) {
		mDataProperties.erase( name );
		return;
	}
	mDataProperties.erase( normalizeDataPropertyName( name ) );
}

}} // namespace EE::UI
