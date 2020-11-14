#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscrollablewidget.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace EE { namespace UI {

UIScrollableWidget::UIScrollableWidget( const std::string& tag ) :
	UIWidget( tag ),
	mViewType( Exclusive ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mVScroll( UIScrollBar::NewVertical() ),
	mHScroll( UIScrollBar::NewHorizontal() ),
	mSizeChangeCb( 0 ),
	mPosChangeCb( 0 ) {
	mFlags |= UI_OWNS_CHILDS_POSITION;

	mVScroll->setParent( this );
	mHScroll->setParent( this );

	mVScroll->addEventListener( Event::OnValueChange,
								cb::Make1( this, &UIScrollableWidget::onValueChangeCb ) );
	mHScroll->addEventListener( Event::OnValueChange,
								cb::Make1( this, &UIScrollableWidget::onValueChangeCb ) );

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

const ScrollBarMode& UIScrollableWidget::getVerticalScrollMode() {
	return mVScrollMode;
}

void UIScrollableWidget::setHorizontalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;
		onContentSizeChange();
	}
}

const ScrollBarMode& UIScrollableWidget::getHorizontalScrollMode() {
	return mHScrollMode;
}

const UIScrollableWidget::ScrollViewType& UIScrollableWidget::getViewType() const {
	return mViewType;
}

void UIScrollableWidget::setViewType( const ScrollViewType& viewType ) {
	if ( viewType != mViewType ) {
		mViewType = viewType;
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
		mHScroll->setVisible( true );
		mHScroll->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mHScrollMode ) {
		mHScroll->setVisible( false );
		mHScroll->setEnabled( false );
	} else {
		Float totW = getPixelsSize().getWidth() - getPixelsPadding().Left -
					 getPixelsPadding().Right - mVScroll->getPixelsSize().getWidth();

		bool visible = contentSize.getWidth() > totW;

		mHScroll->setVisible( visible );
		mHScroll->setEnabled( visible );
	}

	if ( ScrollBarMode::AlwaysOn == mVScrollMode ) {
		mVScroll->setVisible( true );
		mVScroll->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mVScrollMode ) {
		mVScroll->setVisible( false );
		mVScroll->setEnabled( false );
	} else {
		Float totH = getPixelsSize().getHeight() - getPixelsPadding().Top -
					 getPixelsPadding().Bottom - mHScroll->getPixelsSize().getHeight();

		bool visible = contentSize.getHeight() > totH;

		mVScroll->setVisible( visible );
		mVScroll->setEnabled( visible );
	}

	if ( ScrollBarMode::Auto == mHScrollMode ) {
		Float totW = getPixelsSize().getWidth() - getPixelsPadding().Left -
					 getPixelsPadding().Right -
					 ( mVScroll->isVisible() ? mVScroll->getPixelsSize().getWidth() : 0 );

		bool visible = contentSize.getWidth() > totW;

		mHScroll->setVisible( visible );
		mHScroll->setEnabled( visible );
	}

	Sizef size = getPixelsSize() - mPaddingPx;

	if ( Exclusive == mViewType ) {
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
	if ( size.getHeight() > 0 )
		mVScroll->setPageStep( size.getHeight() / contentSize.getHeight() );

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
												   const Uint32& propertyIndex ) {
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

void UIScrollableWidget::scrollToTop() {
	mVScroll->setValue( 0 );
}

void UIScrollableWidget::scrollToBottom() {
	mVScroll->setValue( 1 );
}

void UIScrollableWidget::scrollToPosition( const Rectf& pos, const bool& scrollVertically,
										   const bool& scrollHorizontally ) {
	Rectf visibleRect( getVisibleRect() );
	if ( visibleRect.contains( pos ) )
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
				setViewType( Inclusive );
			else if ( "exclusive" == val || "outside" == val )
				setViewType( Exclusive );
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
			if ( mVScroll->isEnabled() ) {
				if ( Msg->getFlags() & EE_BUTTON_WUMASK ) {
					mVScroll->setValue( mVScroll->getValue() - mVScroll->getClickStep() );
					return 1;
				} else if ( Msg->getFlags() & EE_BUTTON_WDMASK ) {
					mVScroll->setValue( mVScroll->getValue() + mVScroll->getClickStep() );
					return 1;
				}
			}
		}
	}
	return UIWidget::onMessage( Msg );
}

}} // namespace EE::UI
