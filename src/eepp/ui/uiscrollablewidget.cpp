#include <cmath>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace UI {

UIScrollableWidget::UIScrollableWidget( const std::string& tag ) :
	UITouchDraggableWidget( tag ),
	mScrollViewType( ScrollViewType::Outside ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mVScroll( UIScrollBar::NewVertical() ),
	mHScroll( UIScrollBar::NewHorizontal() ),
	mSizeChangeCb( 0 ),
	mPosChangeCb( 0 ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION | UI_SCROLLABLE;

	mVScroll->setParent( this );
	mHScroll->setParent( this );

	mVScroll->on( Event::OnValueChange, [this]( auto event ) { onValueChangeCb( event ); } );
	mHScroll->on( Event::OnValueChange, [this]( auto event ) { onValueChangeCb( event ); } );

	applyDefaultTheme();
}

Uint32 UIScrollableWidget::getType() const {
	return UI_TYPE_SCROLLABLEWIDGET;
}

bool UIScrollableWidget::isType( const Uint32& type ) const {
	return UIScrollableWidget::getType() == type ? true : UITouchDraggableWidget::isType( type );
}

void UIScrollableWidget::onSizeChange() {
	onContentSizeChange();
	UIWidget::onSizeChange();
}

void UIScrollableWidget::onAlphaChange() {
	UIWidget::onAlphaChange();
	mVScroll->setAlpha( mAlpha );
	mHScroll->setAlpha( mAlpha );
}

void UIScrollableWidget::onPaddingChange() {
	onContentSizeChange();
	UIWidget::onPaddingChange();
}

void UIScrollableWidget::setVerticalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;
		onContentSizeChange();
	}
}

const ScrollBarMode& UIScrollableWidget::getVerticalScrollMode() const {
	return mVScrollMode;
}

void UIScrollableWidget::setHorizontalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;
		onContentSizeChange();
	}
}

const ScrollBarMode& UIScrollableWidget::getHorizontalScrollMode() const {
	return mHScrollMode;
}

void UIScrollableWidget::setScrollMode( const ScrollBarMode& verticalMode,
										const ScrollBarMode& horizontalMode ) {
	if ( verticalMode != mVScrollMode || horizontalMode != mHScrollMode ) {
		mVScrollMode = verticalMode;
		mHScrollMode = horizontalMode;
		onContentSizeChange();
	}
}

const ScrollViewType& UIScrollableWidget::getViewType() const {
	return mScrollViewType;
}

void UIScrollableWidget::setScrollViewType( const ScrollViewType& viewType ) {
	if ( viewType != mScrollViewType ) {
		mScrollViewType = viewType;
		onContentSizeChange();
	}
}

UIScrollBar* UIScrollableWidget::getVerticalScrollBar() const {
	return mVScroll;
}

UIScrollBar* UIScrollableWidget::getHorizontalScrollBar() const {
	return mHScroll;
}

void UIScrollableWidget::onContentSizeChange() {
	Sizef contentSize( getContentSize() );

	if ( ScrollBarMode::AlwaysOn == mHScrollMode ) {
		mHScroll->setVisible( true )->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mHScrollMode ) {
		mHScroll->setVisible( false )->setEnabled( false );
	} else {
		Float totW =
			getPixelsSize().getWidth() - getPixelsPadding().Left - getPixelsPadding().Right;

		if ( mScrollViewType == ScrollViewType::Outside )
			totW -= mVScroll->getPixelsSize().getWidth();

		bool visible = contentSize.getWidth() > totW;

		mHScroll->setVisible( visible )->setEnabled( visible );
	}

	if ( ScrollBarMode::AlwaysOn == mVScrollMode ) {
		mVScroll->setVisible( true )->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mVScrollMode ) {
		mVScroll->setVisible( false )->setEnabled( false );
	} else {
		Float totH = getPixelsSize().getHeight() - getPixelsPadding().Top -
					 getPixelsPadding().Bottom -
					 ( ScrollBarMode::AlwaysOff == mHScrollMode || !mHScroll->isVisible()
						   ? 0
						   : mHScroll->getPixelsSize().getHeight() );

		bool visible = contentSize.getHeight() > totH;

		mVScroll->setVisible( visible )->setEnabled( visible );
	}

	if ( ScrollBarMode::Auto == mHScrollMode ) {
		Float totW =
			getPixelsSize().getWidth() - getPixelsPadding().Left - getPixelsPadding().Right -
			( mVScroll->isVisible() &&
					  ( mScrollViewType == ScrollViewType::Outside || mVScroll->getAlpha() != 0.f )
				  ? mVScroll->getPixelsSize().getWidth()
				  : 0 );

		bool visible = contentSize.getWidth() > totW;

		mHScroll->setVisible( visible )->setEnabled( visible );
	}

	Sizef size = getPixelsSize() - mPaddingPx;

	if ( ScrollViewType::Outside == mScrollViewType ) {
		if ( mVScroll->isVisible() )
			size.x -= mVScroll->getPixelsSize().getWidth();

		if ( mHScroll->isVisible() )
			size.y -= mHScroll->getPixelsSize().getHeight();
	}

	mVScroll->setPixelsPosition( getPixelsSize().getWidth() - mVScroll->getPixelsSize().getWidth() -
									 mPaddingPx.Right,
								 mPaddingPx.Top );
	mHScroll->setPixelsPosition( mPaddingPx.Left, getPixelsSize().getHeight() -
													  mHScroll->getPixelsSize().getHeight() -
													  mPaddingPx.Bottom );

	mVScroll->setPixelsSize( mVScroll->getPixelsSize().getWidth(),
							 getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );

	mHScroll->setPixelsSize(
		getPixelsSize().getWidth() - mPaddingPx.Left - mPaddingPx.Right -
			( mVScroll->isVisible() ? mVScroll->getPixelsSize().getWidth() : 0 ),
		mHScroll->getPixelsSize().getHeight() );

	if ( size.getWidth() > 0 )
		mHScroll->setPageStep( size.getWidth() / contentSize.getWidth() );
	if ( size.getHeight() > 0 ) {
		mVScroll->setPageStep( size.getHeight() / contentSize.getHeight() );
		if ( mAutoSetClipStep )
			mVScroll->setClickStep( mVScroll->getPageStep() / 4.f );
	}
	updateScroll();
}

Sizef UIScrollableWidget::getScrollableArea() const {
	Sizef contentSize( getContentSize() );
	Sizef size( getVisibleArea() );
	return contentSize - size;
}

Sizef UIScrollableWidget::getVisibleArea() const {
	Sizef size = getPixelsSize() - mPaddingPx;
	if ( mVScroll->isVisible() )
		size.x -= mVScroll->getPixelsSize().getWidth();
	if ( mHScroll->isVisible() )
		size.y -= mHScroll->getPixelsSize().getHeight();
	return size;
}

Rectf UIScrollableWidget::getVisibleRect() const {
	return Rectf( mScrollOffset, getVisibleArea() );
}

bool UIScrollableWidget::shouldVerticalScrollBeVisible() const {
	Float totH = getPixelsSize().getHeight() - getPixelsPadding().Top - getPixelsPadding().Bottom -
				 ( mHScrollMode == ScrollBarMode::AlwaysOff || !mHScroll->isVisible()
					   ? 0
					   : mHScroll->getPixelsSize().getHeight() );
	return getContentSize().getHeight() > totH;
}

bool UIScrollableWidget::isAutoSetClipStep() const {
	return mAutoSetClipStep;
}

void UIScrollableWidget::setAutoSetClipStep( bool setClipStep ) {
	mAutoSetClipStep = setClipStep;
}

bool UIScrollableWidget::isScrollable() const {
	return UIWidget::isScrollable() && getScrollableArea().y > 0;
}

void UIScrollableWidget::updateScroll() {
	Sizef totalScroll = getScrollableArea();
	Vector2f initScroll( mScrollOffset );
	mScrollOffset = Vector2f::Zero;

	if ( mVScroll->isVisible() && totalScroll.y > 0 )
		mScrollOffset.y = totalScroll.y * mVScroll->getValue();

	if ( mHScroll->isVisible() && totalScroll.x > 0 )
		mScrollOffset.x = totalScroll.x * mHScroll->getValue();

	if ( initScroll != mScrollOffset )
		onScrollChange();
}

void UIScrollableWidget::onScrollChange() {}

void UIScrollableWidget::onValueChangeCb( const Event* ) {
	updateScroll();
	if ( !mApplyingScrollController )
		stopScrollController();
}

std::string UIScrollableWidget::getPropertyString( const PropertyDefinition* propertyDef,
												   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::VScrollMode:
			return getVerticalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getVerticalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		case PropertyId::HScrollMode:
			return getHorizontalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getHorizontalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		case PropertyId::ScrollBarStyle:
			return mVScroll->getScrollBarType() == UIScrollBar::NoButtons ? "no-buttons"
																		  : "two-buttons";
		case PropertyId::ScrollBarMode:
			return getViewType() == ScrollViewType::Overlay ? "overlay" : "outside";
		default:
			return UITouchDraggableWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIScrollableWidget::getPropertiesImplemented() const {
	auto props = UITouchDraggableWidget::getPropertiesImplemented();
	auto local = { PropertyId::VScrollMode, PropertyId::HScrollMode, PropertyId::ScrollBarStyle,
				   PropertyId::ScrollBarMode };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UIScrollableWidget::scrollToTop() {
	mVScroll->setValue( 0 );
}

void UIScrollableWidget::scrollToBottom() {
	mVScroll->setValue( 1 );
}

void UIScrollableWidget::scrollToPosition( const Rectf& pos, const bool& scrollVertically,
										   const bool& scrollHorizontally ) {
	Rectf visibleRect( getVisibleRect() );
	if ( visibleRect.Top < 0 || visibleRect.Bottom < 0 || visibleRect.contains( pos ) )
		return;

	if ( scrollVertically ) {
		if ( pos.getPosition().y <= visibleRect.Top ) {
			mVScroll->setValue( pos.getPosition().y / getContentSize().y );
		} else if ( pos.getPosition().y + pos.getSize().getHeight() >= visibleRect.Bottom ) {
			mVScroll->setValue( ( pos.getPosition().y + pos.getSize().getHeight() ) /
								getContentSize().y );
		}
	}

	if ( scrollHorizontally ) {
		if ( pos.getPosition().x <= visibleRect.Left ) {
			mHScroll->setValue( pos.getPosition().x / getContentSize().x );
		} else if ( pos.getPosition().x + pos.getSize().getWidth() >= visibleRect.Right ) {
			mHScroll->setValue( ( pos.getPosition().x + pos.getSize().getWidth() ) /
								getContentSize().x );
		}
	}
}

bool UIScrollableWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ScrollBarMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );
			if ( "overlay" == val || "inclusive" == val || "inside" == val )
				setScrollViewType( ScrollViewType::Overlay );
			else if ( "outside" == val || "exclusive" == val || "outside" == val )
				setScrollViewType( ScrollViewType::Outside );
			break;
		}
		case PropertyId::VScrollMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "on" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "auto" == val )
				setVerticalScrollMode( ScrollBarMode::Auto );
			break;
		}
		case PropertyId::HScrollMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "on" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "auto" == val )
				setHorizontalScrollMode( ScrollBarMode::Auto );
			break;
		}
		case PropertyId::ScrollBarStyle: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "no-buttons" == val || "nobuttons" == val ) {
				mVScroll->setScrollBarStyle( UIScrollBar::NoButtons );
				mHScroll->setScrollBarStyle( UIScrollBar::NoButtons );
			} else if ( "two-buttons" == val || "twobuttons" == val ) {
				mVScroll->setScrollBarStyle( UIScrollBar::TwoButtons );
				mHScroll->setScrollBarStyle( UIScrollBar::NoButtons );
			}
			break;
		}
		default:
			return UITouchDraggableWidget::applyProperty( attribute );
	}

	return true;
}

Uint32 UIScrollableWidget::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::FocusLoss: {
			if ( NULL != getEventDispatcher() ) {
				Node* focusNode = getEventDispatcher()->getFocusNode();

				if ( this != focusNode && !isParentOf( focusNode ) ) {
					onWidgetFocusLoss();
				}

				return 1;
			}

			break;
		}
	}
	return UITouchDraggableWidget::onMessage( Msg );
}

Uint32 UIScrollableWidget::onMouseWheel( const Vector2f& offset, bool ) {
	const Vector2f maxPosition( getScrollControllerMaxPosition() );
	Vector2f delta;
	Float durationScale = 1.f;
	if ( getInput()->isModState( KEYMOD_SHIFT ) && offset.y != 0.f && mHScroll->isEnabled() ) {
		const Float factor = getWheelScrollFactor( offset.y );
		delta.x = -factor * mHScroll->getClickStep() * maxPosition.x;
		durationScale = std::abs( factor );
	} else {
		if ( offset.y != 0.f && mVScroll->isEnabled() ) {
			const Float factor = getWheelScrollFactor( offset.y );
			delta.y = -factor * mVScroll->getClickStep() * maxPosition.y;
			durationScale = std::abs( factor );
		} else if ( offset.x != 0.f && mHScroll->isEnabled() ) {
			const Float factor = getWheelScrollFactor( offset.x );
			delta.x = factor * mHScroll->getClickStep() * maxPosition.x;
			durationScale = std::abs( factor );
		}
	}
	return scrollBy( delta, durationScale ) ? 1 : 0;
}

bool UIScrollableWidget::supportsScrollController() const {
	return true;
}

Vector2f UIScrollableWidget::getScrollControllerPosition() const {
	return mScrollOffset;
}

Vector2f UIScrollableWidget::getScrollControllerMaxPosition() const {
	const Sizef area( getScrollableArea() );
	return { eemax( 0.f, area.x ), eemax( 0.f, area.y ) };
}

void UIScrollableWidget::setScrollControllerPosition( const Vector2f& position ) {
	const Vector2f maxPosition( getScrollControllerMaxPosition() );
	mHScroll->setValue( maxPosition.x > 0.f ? position.x / maxPosition.x : 0.f );
	mVScroll->setValue( maxPosition.y > 0.f ? position.y / maxPosition.y : 0.f );
}

}} // namespace EE::UI
