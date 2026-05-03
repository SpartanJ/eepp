#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <eepp/ui/uiscrollview.hpp>

namespace EE { namespace UI {

UIHTMLWidget* UIHTMLWidget::New() {
	return eeNew( UIHTMLWidget, () );
}

UIHTMLWidget::UIHTMLWidget( const std::string& tag ) : UILayout( tag ) {}

UIHTMLWidget::~UIHTMLWidget() {
	if ( mScrollTarget && mScrollCb )
		mScrollTarget->removeEventListener( mScrollCb );
	eeSAFE_DELETE( mLayouter );
}

UILayouter* UIHTMLWidget::getLayouter() {
	if ( !mLayouter ) {
		mLayouter = UILayouterManager::create( mDisplay, this );
	}
	return mLayouter;
}

Uint32 UIHTMLWidget::getType() const {
	return UI_TYPE_HTML_WIDGET;
}

bool UIHTMLWidget::isType( const Uint32& type ) const {
	return UIHTMLWidget::getType() == type ? true : UILayout::isType( type );
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
	notifyLayoutAttrChange();
}

void UIHTMLWidget::setDisplay( CSSDisplay display ) {
	if ( mDisplay != display ) {
		mDisplay = display;
		if ( mDisplay == CSSDisplay::InlineBlock || mDisplay == CSSDisplay::Inline ) {
			if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
				setLayoutWidthPolicy( SizePolicy::WrapContent );
		} else if ( mDisplay == CSSDisplay::Block || mDisplay == CSSDisplay::ListItem ) {
			if ( getLayoutWidthPolicy() == SizePolicy::WrapContent &&
				 mPosition != CSSPosition::Absolute && mPosition != CSSPosition::Fixed )
				setLayoutWidthPolicy( SizePolicy::MatchParent );
		}
		onDisplayChange();
	}
}

void UIHTMLWidget::setCSSPosition( CSSPosition position ) {
	if ( mPosition != position ) {
		mPosition = position;
		if ( position == CSSPosition::Absolute || position == CSSPosition::Fixed ) {
			if ( getLayoutWidthPolicy() == SizePolicy::MatchParent )
				setLayoutWidthPolicy( SizePolicy::WrapContent );
		}
		updateScrollListeners();
		onPositionChange();
	}
}

void UIHTMLWidget::setCSSFloat( CSSFloat cssFloat ) {
	if ( mFloat != cssFloat ) {
		mFloat = cssFloat;
		notifyLayoutAttrChange();
	}
}

void UIHTMLWidget::setCSSClear( CSSClear cssClear ) {
	if ( mClear != cssClear ) {
		mClear = cssClear;
		notifyLayoutAttrChange();
	}
}

void UIHTMLWidget::setOffsets( const Rectf& offsets ) {
	if ( mOffsets != offsets ) {
		mOffsets = offsets;
		mTopEq = String::fromFloat( offsets.Top, "dp" );
		mLeftEq = String::fromFloat( offsets.Left, "dp" );
		mRightEq = String::fromFloat( offsets.Right, "dp" );
		mBottomEq = String::fromFloat( offsets.Bottom, "dp" );
		notifyLayoutAttrChange();
	}
}

void UIHTMLWidget::setZIndex( int zIndex ) {
	mZIndex = zIndex;
}

std::vector<PropertyId> UIHTMLWidget::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = { PropertyId::Display, PropertyId::Position, PropertyId::Float,
				   PropertyId::Clear,	PropertyId::Top,	  PropertyId::Right,
				   PropertyId::Bottom,	PropertyId::Left,	  PropertyId::ZIndex };
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
		default:
			return UILayout::getPropertyString( propertyDef );
	}
}

bool UIHTMLWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Display: {
			setDisplay( CSSDisplayHelper::fromString( attribute.asString() ) );
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
		case PropertyId::ZIndex: {
			setZIndex( attribute.asInt() );
			return true;
		}
		case PropertyId::Top: {
			mTopEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Right: {
			mRightEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Bottom: {
			mBottomEq = attribute.asString();
			notifyLayoutAttrChange();
			return true;
		}
		case PropertyId::Left: {
			mLeftEq = attribute.asString();
			notifyLayoutAttrChange();
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
			UIHTMLWidget* htmlChild = static_cast<UIHTMLWidget*>( child );
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

bool UIHTMLWidget::isOutOfFlow() const {
	return mPosition == CSSPosition::Absolute || mPosition == CSSPosition::Fixed;
}

}} // namespace EE::UI
