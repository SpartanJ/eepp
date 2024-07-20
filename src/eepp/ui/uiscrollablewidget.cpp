#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace EE { namespace UI {

UIScrollableWidget::UIScrollableWidget( const std::string& tag ) :
	UIWidget( tag ),
	mScrollViewType( Exclusive ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mVScroll( UIScrollBar::NewVertical() ),
	mHScroll( UIScrollBar::NewHorizontal() ),
	mSizeChangeCb( 0 ),
	mPosChangeCb( 0 ) {
	mFlags |= UI_OWNS_CHILDS_POSITION | UI_SCROLLABLE;

	mVScroll->setParent( this );
	mHScroll->setParent( this );

	mVScroll->addEventListener( Event::OnValueChange,
								[this]( auto event ) { onValueChangeCb( event ); } );
	mHScroll->addEventListener( Event::OnValueChange,
								[this]( auto event ) { onValueChangeCb( event ); } );

	applyDefaultTheme();
}

Uint32 UIScrollableWidget::getType() const {
	return UI_TYPE_SCROLLABLEWIDGET;
}

bool UIScrollableWidget::isType( const Uint32& type ) const {
	return UIWidget::getType() == type ? true : UIWidget::isType( type );
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

const UIScrollableWidget::ScrollViewType& UIScrollableWidget::getViewType() const {
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

		if ( mScrollViewType == Exclusive )
			totW -= mVScroll->getPixelsSize().getWidth();

		bool visible = contentSize.getWidth() > totW;

		mHScroll->setVisible( visible )->setEnabled( visible );
	}

	if ( ScrollBarMode::AlwaysOn == mVScrollMode ) {
		mVScroll->setVisible( true )->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mVScrollMode ) {
		mVScroll->setVisible( false )->setEnabled( false );
	} else {
		Float totH =
			getPixelsSize().getHeight() - getPixelsPadding().Top - getPixelsPadding().Bottom -
			( ScrollBarMode::AlwaysOff == mHScrollMode ? 0
													   : mHScroll->getPixelsSize().getHeight() );

		bool visible = contentSize.getHeight() > totH;

		mVScroll->setVisible( visible )->setEnabled( visible );
	}

	if ( ScrollBarMode::Auto == mHScrollMode ) {
		Float totW = getPixelsSize().getWidth() - getPixelsPadding().Left -
					 getPixelsPadding().Right -
					 ( mVScroll->isVisible() &&
							   ( mScrollViewType == Exclusive || mVScroll->getAlpha() != 0.f )
						   ? mVScroll->getPixelsSize().getWidth()
						   : 0 );

		bool visible = contentSize.getWidth() > totW;

		mHScroll->setVisible( visible )->setEnabled( visible );
	}

	Sizef size = getPixelsSize() - mPaddingPx;

	if ( Exclusive == mScrollViewType ) {
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
				 mHScroll->getPixelsSize().getHeight();
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
			return getViewType() == Inclusive ? "inclusive" : "exclusive";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIScrollableWidget::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
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
			if ( "inclusive" == val || "inside" == val )
				setScrollViewType( Inclusive );
			else if ( "exclusive" == val || "outside" == val )
				setScrollViewType( Exclusive );
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
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

Uint32 UIScrollableWidget::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			bool moved = false;
			if ( mVScroll->isEnabled() ) {

				if ( Msg->getFlags() & EE_BUTTON_WUMASK ) {
					mVScroll->setValue( mVScroll->getValue() - mVScroll->getClickStep() );
					moved = true;
				} else if ( Msg->getFlags() & EE_BUTTON_WDMASK ) {
					mVScroll->setValue( mVScroll->getValue() + mVScroll->getClickStep() );
					moved = true;
				}
			}

			if ( mHScroll->isEnabled() ) {
				if ( Msg->getFlags() & EE_BUTTON_WLMASK ) {
					mHScroll->setValue( mHScroll->getValue() - mHScroll->getClickStep() );
					moved = true;
				} else if ( Msg->getFlags() & EE_BUTTON_WRMASK ) {
					mHScroll->setValue( mHScroll->getValue() + mHScroll->getClickStep() );
					moved = true;
				}
			}

			if ( moved )
				return 1;
		}
	}
	return UIWidget::onMessage( Msg );
}

}} // namespace EE::UI
