#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIScrollView * UIScrollView::New() {
	return eeNew( UIScrollView, () );
}

UIScrollView::UIScrollView() :
	UITouchDragableWidget(),
	mViewType( Exclusive ),
	mVScrollMode( UI_SCROLLBAR_AUTO ),
	mHScrollMode( UI_SCROLLBAR_AUTO ),
	mVScroll( UIScrollBar::New( UI_VERTICAL ) ),
	mHScroll( UIScrollBar::New( UI_HORIZONTAL ) ),
	mContainer( UINode::New() ),
	mScrollView( NULL ),
	mSizeChangeCb( 0 )
{
	enableReportSizeChangeToChilds();

	mVScroll->setParent( this );
	mHScroll->setParent( this );
	mContainer->setParent( this );
	mContainer->clipEnable();
	mContainer->enableReportSizeChangeToChilds();

	mVScroll->addEventListener( Event::OnValueChange, cb::Make1( this, &UIScrollView::onValueChangeCb ) );
	mHScroll->addEventListener( Event::OnValueChange, cb::Make1( this, &UIScrollView::onValueChangeCb ) );

	applyDefaultTheme();
}

Uint32 UIScrollView::getType() const {
	return UI_TYPE_SCROLLVIEW;
}

bool UIScrollView::isType( const Uint32& type ) const {
	return UIScrollView::getType() == type ? true : UITouchDragableWidget::isType( type );
}

void UIScrollView::onSizeChange() {
	UIWidget::onSizeChange();
	containerUpdate();
}

void UIScrollView::onAlphaChange() {
	UIWidget::onAlphaChange();
	mVScroll->setAlpha( mAlpha );
	mHScroll->setAlpha( mAlpha );
	mContainer->setAlpha( mAlpha );

	if ( NULL != mScrollView )
		mScrollView->setAlpha( mAlpha );
}

void UIScrollView::onChildCountChange() {
	Node * child = mChild;
	bool found = false;

	while ( NULL != child ) {
		if ( child != mVScroll && child != mHScroll && child != mContainer && child != mScrollView ) {
			found = true;
			break;
		}

		child = child->getNextNode();
	}

	if ( found ) {
		if ( NULL != mScrollView ) {
			if ( 0 != mSizeChangeCb )
				mScrollView->removeEventListener( mSizeChangeCb );

			mScrollView->close();
		}

		child->setParent( mContainer );
		mScrollView = child;
		mSizeChangeCb = mScrollView->addEventListener( Event::OnSizeChange, cb::Make1( this, &UIScrollView::onScrollViewSizeChange ) );

		containerUpdate();
	}
}

void UIScrollView::onPaddingChange() {
	containerUpdate();
	UIWidget::onPaddingChange();
}

void UIScrollView::setVerticalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;
		containerUpdate();
	}
}

const UI_SCROLLBAR_MODE& UIScrollView::getVerticalScrollMode() {
	return mVScrollMode;
}

void UIScrollView::setHorizontalScrollMode( const UI_SCROLLBAR_MODE& Mode ) {
	if ( Mode != mHScrollMode ) {
		mHScrollMode = Mode;
		containerUpdate();
	}
}

const UI_SCROLLBAR_MODE& UIScrollView::getHorizontalScrollMode() {
	return mHScrollMode;
}

const UIScrollView::ScrollViewType& UIScrollView::getViewType() const {
	return mViewType;
}

void UIScrollView::setViewType( const ScrollViewType& viewType ) {
	if ( viewType != mViewType ) {
		mViewType = viewType;
		containerUpdate();
	}
}

UIScrollBar * UIScrollView::getVerticalScrollBar() const {
	return mVScroll;
}

UIScrollBar * UIScrollView::getHorizontalScrollBar() const {
	return mHScroll;
}

UINode * UIScrollView::getContainer() const {
	return mContainer;
}

void UIScrollView::containerUpdate() {
	if ( NULL == mScrollView )
		return;

	Sizef size = mDpSize - mPadding;

	if ( Exclusive == mViewType ) {
		if ( mVScroll->isVisible() )
			size.x -= mVScroll->getSize().getWidth();

		if ( mHScroll->isVisible() )
			size.y -= mHScroll->getSize().getHeight();
	}

	mContainer->setPosition( mPadding.Left, mPadding.Top );
	mContainer->setSize( size );

	if ( UI_SCROLLBAR_ALWAYS_ON == mHScrollMode ) {
		mHScroll->setVisible( true );
		mHScroll->setEnabled( true );
	} else if ( UI_SCROLLBAR_ALWAYS_OFF == mHScrollMode ) {
		mHScroll->setVisible( false );
		mHScroll->setEnabled( false );
	} else {
		bool visible = mScrollView->getSize().getWidth() > mContainer->getSize().getWidth();

		mHScroll->setVisible( visible );
		mHScroll->setEnabled( visible );
	}

	if ( UI_SCROLLBAR_ALWAYS_ON == mVScrollMode ) {
		mVScroll->setVisible( true );
		mVScroll->setEnabled( true );
	} else if ( UI_SCROLLBAR_ALWAYS_OFF == mVScrollMode ) {
		mVScroll->setVisible( false );
		mVScroll->setEnabled( false );
	} else {
		bool visible = mScrollView->getSize().getHeight() > mContainer->getSize().getHeight();

		mVScroll->setVisible( visible );
		mVScroll->setEnabled( visible );
	}

	mVScroll->setPosition( mDpSize.getWidth() - mVScroll->getSize().getWidth() - mPadding.Right, mPadding.Top );
	mHScroll->setPosition( mPadding.Left, mDpSize.getHeight() - mHScroll->getSize().getHeight() - mPadding.Bottom );

	mVScroll->setSize( mVScroll->getSize().getWidth(), mDpSize.getHeight() - mPadding.Top - mPadding.Bottom );
	mHScroll->setSize( mDpSize.getWidth() - mPadding.Left - mPadding.Right - ( mVScroll->isVisible() ? mVScroll->getSize().getWidth() : 0 ), mHScroll->getSize().getHeight() );

	if ( mVScroll->isVisible() && 0 != mScrollView->getSize().getHeight() )
		mVScroll->setPageStep( (Float)mContainer->getSize().getHeight() / (Float)mScrollView->getSize().getHeight() );

	if ( mHScroll->isVisible() && 0 != mScrollView->getSize().getWidth() ) {
		mHScroll->setPageStep( (Float)mContainer->getSize().getWidth() / (Float)mScrollView->getSize().getWidth() );
	}

	updateScroll();
}

void UIScrollView::updateScroll() {
	if ( NULL == mScrollView )
		return;

	mScrollView->setPosition(
		mHScroll->isVisible() ? -( mHScroll->getSlider()->getValue() * eemax( 0.f, mScrollView->getSize().getWidth() - mDpSize.getWidth() ) ) : 0.f ,
		mVScroll->isVisible() ? -( mVScroll->getSlider()->getValue() * eemax( 0.f, mScrollView->getSize().getHeight() - mDpSize.getHeight() ) ) : 0.f
	);
}

void UIScrollView::onValueChangeCb( const Event * Event ) {
	updateScroll();
}

void UIScrollView::onScrollViewSizeChange(const Event * Event) {
	containerUpdate();
}

void UIScrollView::onTouchDragValueChange( Vector2f diff ) {
	if ( NULL == mScrollView )
		return;

	if ( mVScroll->isEnabled() && 0 != mScrollView->getSize().getHeight() )
		mVScroll->setValue( mVScroll->getValue() + ( -diff.y / (Float)( mScrollView->getSize().getHeight() ) ) );

	if ( mHScroll->isEnabled() && 0 != mScrollView->getSize().getWidth() )
		mHScroll->setValue( mHScroll->getValue() + ( -diff.x / (Float)( mScrollView->getSize().getWidth() ) ) );
}

bool UIScrollView::isTouchOverAllowedChilds() {
	bool ret = mViewType == Exclusive ? !mVScroll->isMouseOverMeOrChilds() && !mHScroll->isMouseOverMeOrChilds() : true;
	return isMouseOverMeOrChilds() && mScrollView->isMouseOverMeOrChilds() && ret;
}

bool UIScrollView::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "type" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );

		if ( "inclusive" == val ) setViewType( Inclusive );
		else if ( "exclusive" == val ) setViewType( Exclusive );
	} else if ( "vscroll_mode" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );

		if ( "on" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "off" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "auto" == val ) setVerticalScrollMode( UI_SCROLLBAR_AUTO );
	} else if ( "hscroll_mode" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );

		if ( "on" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "off" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
		else if ( "auto" == val ) setHorizontalScrollMode( UI_SCROLLBAR_AUTO );
	} else if ( "scrollbartype" == name ) {
		std::string val( attribute.asString() );
		String::toLowerInPlace( val );

		if ( "nobuttons" == val ) {
			mVScroll->setScrollBarType( UIScrollBar::NoButtons );
			mHScroll->setScrollBarType( UIScrollBar::NoButtons );
		} else if ( "twobuttons" == val ) {
			mVScroll->setScrollBarType( UIScrollBar::TwoButtons );
			mHScroll->setScrollBarType( UIScrollBar::NoButtons );
		}
	} else {
		return UITouchDragableWidget::setAttribute( attribute );
	}

	return true;
}

}}
