#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>

namespace EE { namespace UI {

UIScrollView* UIScrollView::New() {
	return eeNew( UIScrollView, () );
}

UIScrollView::UIScrollView() :
	UITouchDraggableWidget( "scrollview" ),
	mViewType( ScrollViewType::Outside ),
	mVScrollMode( ScrollBarMode::Auto ),
	mHScrollMode( ScrollBarMode::Auto ),
	mVScroll( UIScrollBar::NewVertical() ),
	mHScroll( UIScrollBar::NewHorizontal() ),
	mContainer( UIWidget::NewWithTag( "scrollview::container" ) ),
	mScrollView( NULL ),
	mSizeChangeCb( 0 ),
	mPosChangeCb( 0 ) {
	mFlags |= UI_OWNS_CHILDREN_POSITION | UI_SCROLLABLE;
	enableReportSizeChangeToChildren();

	mVScroll->setParent( this );
	mHScroll->setParent( this );
	mContainer->setParent( this );
	mContainer->setClipType( ClipType::ContentBox );
	mContainer->setFlags( UI_OWNS_CHILDREN_POSITION );
	mContainer->enableReportSizeChangeToChildren();

	mContainer->on( Event::OnSizeChange, [this]( auto ) { containerUpdate(); } );
	mVScroll->on( Event::OnValueChange, [this]( auto event ) { onValueChangeCb( event ); } );
	mHScroll->on( Event::OnValueChange, [this]( auto event ) { onValueChangeCb( event ); } );

	applyDefaultTheme();
}

Uint32 UIScrollView::getType() const {
	return UI_TYPE_SCROLLVIEW;
}

bool UIScrollView::isType( const Uint32& type ) const {
	return UIScrollView::getType() == type ? true : UITouchDraggableWidget::isType( type );
}

void UIScrollView::onSizeChange() {
	containerUpdate();
	UIWidget::onSizeChange();
}

void UIScrollView::onAlphaChange() {
	UIWidget::onAlphaChange();
	mVScroll->setAlpha( mAlpha );
	mHScroll->setAlpha( mAlpha );
	mContainer->setAlpha( mAlpha );

	if ( NULL != mScrollView )
		mScrollView->setAlpha( mAlpha );
}

void UIScrollView::onChildCountChange( Node* child, const bool& removed ) {
	if ( !removed && child != mVScroll && child != mHScroll && child != mContainer &&
		 child != mScrollView ) {
		if ( NULL != mScrollView ) {
			if ( 0 != mSizeChangeCb )
				mScrollView->removeEventListener( mSizeChangeCb );

			if ( 0 != mPosChangeCb )
				mScrollView->removeEventListener( mPosChangeCb );

			mScrollView->close();
		}

		mScrollView = child;
		mScrollView->setParent( mContainer );
		mSizeChangeCb = mScrollView->on(
			Event::OnSizeChange, [this]( auto event ) { onScrollViewSizeChange( event ); } );
		mPosChangeCb = mScrollView->on( Event::OnPositionChange, [this]( auto event ) {
			onScrollViewPositionChange( event );
		} );
		mLastScrollViewSize = mScrollView->getPixelsSize();

		containerUpdate();
	}

	UITouchDraggableWidget::onChildCountChange( child, removed );
}

void UIScrollView::onPaddingChange() {
	containerUpdate();
	UIWidget::onPaddingChange();
}

void UIScrollView::setVerticalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;
		containerUpdate();
	}
}

const ScrollBarMode& UIScrollView::getVerticalScrollMode() const {
	return mVScrollMode;
}

void UIScrollView::setHorizontalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;
		containerUpdate();
	}
}

const ScrollBarMode& UIScrollView::getHorizontalScrollMode() const {
	return mHScrollMode;
}

const ScrollViewType& UIScrollView::getViewType() const {
	return mViewType;
}

void UIScrollView::setViewType( const ScrollViewType& viewType ) {
	if ( viewType != mViewType ) {
		mViewType = viewType;
		containerUpdate();
	}
}

UIScrollBar* UIScrollView::getVerticalScrollBar() const {
	return mVScroll;
}

UIScrollBar* UIScrollView::getHorizontalScrollBar() const {
	return mHScroll;
}

UIWidget* UIScrollView::getContainer() const {
	return mContainer;
}

Node* UIScrollView::getScrollView() const {
	return mScrollView;
}

void UIScrollView::containerUpdate() {
	if ( NULL == mScrollView )
		return;

	if ( ScrollBarMode::AlwaysOn == mHScrollMode ) {
		mHScroll->setVisible( true );
		mHScroll->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mHScrollMode ) {
		mHScroll->setVisible( false );
		mHScroll->setEnabled( false );
	} else {
		bool visible = mScrollView->getSize().getWidth() >
					   getSize().getWidth() - getPadding().Left - getPadding().Right;

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
		bool visible = mScrollView->getSize().getHeight() >
					   getSize().getHeight() - getPadding().Top - getPadding().Bottom;

		mVScroll->setVisible( visible );
		mVScroll->setEnabled( visible );
	}

	Sizef size = getSize() - mPadding;

	if ( ScrollViewType::Outside == mViewType ) {
		if ( mVScroll->isVisible() )
			size.x -= mVScroll->getSize().getWidth();

		if ( mHScroll->isVisible() )
			size.y -= mHScroll->getSize().getHeight();
	}

	mContainer->setPosition( mPadding.Left, mPadding.Top );

	// TODO: Fix layouting to avoid needing this hack.
	if ( size != mContainer->getSize() )
		runOnMainThread( [this, size]() { mContainer->setSize( size ); } );

	mVScroll->setPosition( getSize().getWidth() - mVScroll->getSize().getWidth() - mPadding.Right,
						   mPadding.Top );
	mHScroll->setPosition( mPadding.Left, getSize().getHeight() - mHScroll->getSize().getHeight() -
											  mPadding.Bottom );

	mVScroll->setSize( mVScroll->getSize().getWidth(),
					   getSize().getHeight() - mPadding.Top - mPadding.Bottom );
	mHScroll->setSize( getSize().getWidth() - mPadding.Left - mPadding.Right -
						   ( mVScroll->isVisible() ? mVScroll->getSize().getWidth() : 0 ),
					   mHScroll->getSize().getHeight() );

	if ( mVScroll->isVisible() && 0 != mScrollView->getSize().getHeight() ) {
		mVScroll->setPageStep( mContainer->getSize().getHeight() /
							   mScrollView->getSize().getHeight() );

		if ( mAutoSetClipStep )
			mVScroll->setClickStep( mVScroll->getPageStep() / 4.f );
	}

	if ( mHScroll->isVisible() && 0 != mScrollView->getSize().getWidth() ) {
		mHScroll->setPageStep( mContainer->getSize().getWidth() /
							   mScrollView->getSize().getWidth() );
	}

	updateScroll();
}

void UIScrollView::updateScroll() {
	if ( NULL == mScrollView )
		return;

	mScrollView->setPosition(
		mHScroll->isVisible()
			? -static_cast<int>( mHScroll->getSlider()->getValue() *
								 eemax( 0.f, mScrollView->getSize().getWidth() -
												 mContainer->getSize().getWidth() ) )
			: 0.f,
		mVScroll->isVisible()
			? -static_cast<int>( mVScroll->getSlider()->getValue() *
								 eemax( 0.f, mScrollView->getSize().getHeight() -
												 mContainer->getSize().getHeight() ) )
			: 0.f );
}

void UIScrollView::onValueChangeCb( const Event* ) {
	updateScroll();
}

void UIScrollView::onScrollViewSizeChange( const Event* ) {
	Float lastScrollWidth =
		eemax( 0.f, mLastScrollViewSize.getWidth() - mContainer->getSize().getWidth() );
	Float lastScrollHeight =
		eemax( 0.f, mLastScrollViewSize.getHeight() - mContainer->getSize().getHeight() );
	Float lastScrollX = mHScroll->getValue() * lastScrollWidth;
	Float lastScrollY = mVScroll->getValue() * lastScrollHeight;

	containerUpdate();

	if ( mAnchorScroll ) {
		Float scrollHeight = eemax( 0.f, mScrollView->getPixelsSize().getHeight() -
											 mContainer->getSize().getHeight() );
		Float valY = lastScrollY / scrollHeight;
		mVScroll->setValue( valY );

		Float scrollWidth = eemax( 0.f, mScrollView->getPixelsSize().getWidth() -
											mContainer->getSize().getWidth() );
		Float valX = lastScrollX / scrollWidth;
		mHScroll->setValue( valX );
	}

	mLastScrollViewSize = mScrollView->getPixelsSize();
}

void UIScrollView::onScrollViewPositionChange( const Event* ) {
	updateScroll();
}

void UIScrollView::onTouchDragValueChange( Vector2f diff ) {
	if ( NULL == mScrollView )
		return;

	if ( mVScroll->isEnabled() && 0 != mScrollView->getSize().getHeight() )
		mVScroll->setValue( mVScroll->getValue() +
							( -diff.y / (Float)( mScrollView->getSize().getHeight() ) ) );

	if ( mHScroll->isEnabled() && 0 != mScrollView->getSize().getWidth() )
		mHScroll->setValue( mHScroll->getValue() +
							( -diff.x / (Float)( mScrollView->getSize().getWidth() ) ) );
}

bool UIScrollView::isTouchOverAllowedChildren() {
	bool ret = mViewType == ScrollViewType::Outside
				   ? !mVScroll->isMouseOverMeOrChildren() && !mHScroll->isMouseOverMeOrChildren()
				   : true;
	return isMouseOverMeOrChildren() && mScrollView->isMouseOverMeOrChildren() && ret;
}

std::string UIScrollView::getPropertyString( const PropertyDefinition* propertyDef,
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

std::vector<PropertyId> UIScrollView::getPropertiesImplemented() const {
	auto props = UITouchDraggableWidget::getPropertiesImplemented();
	auto local = { PropertyId::VScrollMode, PropertyId::HScrollMode, PropertyId::ScrollBarStyle,
				   PropertyId::ScrollBarMode };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIScrollView::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ScrollBarMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );
			if ( "overlay" == val || "inclusive" == val || "inside" == val )
				setViewType( ScrollViewType::Overlay );
			else if ( "exclusive" == val || "outside" == val )
				setViewType( ScrollViewType::Outside );
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

Uint32 UIScrollView::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			if ( mVScroll->isEnabled() && 0 != mScrollView->getSize().getHeight() &&
				 isTouchOverAllowedChildren() && Msg->getSender()->isUINode() &&
				 !Msg->getSender()->asType<UINode>()->isScrollable() ) {
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
	return UITouchDraggableWidget::onMessage( Msg );
}

bool UIScrollView::isAutoSetClipStep() const {
	return mAutoSetClipStep;
}

void UIScrollView::setAutoSetClipStep( bool setClipStep ) {
	mAutoSetClipStep = setClipStep;
}

bool UIScrollView::isScrollAnchored() const {
	return mAnchorScroll;
}

void UIScrollView::setAnchorScroll( bool anchor ) {
	mAnchorScroll = anchor;
}

}} // namespace EE::UI
