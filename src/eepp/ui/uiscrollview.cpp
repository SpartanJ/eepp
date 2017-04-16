#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>

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
	mContainer( UIControlAnim::New() ),
	mScrollView( NULL )
{
	mVScroll->setParent( this );
	mHScroll->setParent( this );
	mContainer->setParent( this );
	mContainer->setFlags( UI_CLIP_ENABLE );

	mVScroll->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIScrollView::onValueChangeCb ) );
	mHScroll->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIScrollView::onValueChangeCb ) );

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

	if ( NULL != mScrollView && mScrollView->isAnimated() )
		reinterpret_cast<UIControlAnim*>( mScrollView )->setAlpha( mAlpha );
}

void UIScrollView::onChildCountChange() {
	UIControl * child = mChild;
	bool found = false;

	while ( NULL != child ) {
		if ( child != mVScroll && child != mHScroll && child != mContainer && child != mScrollView ) {
			found = true;
			break;
		}

		child = child->getNextControl();
	}

	if ( found ) {
		if ( NULL != mScrollView )
			mScrollView->close();

		child->setParent( mContainer );
		mScrollView = child;

		containerUpdate();
	}
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

UIControlAnim * UIScrollView::getContainer() const {
	return mContainer;
}

void UIScrollView::containerUpdate() {
	if ( NULL == mScrollView )
		return;

	Sizei size = mSize;

	if ( Exclusive == mViewType ) {
		if ( mVScroll->isVisible() )
			size.x -= mVScroll->getSize().getWidth();

		if ( mHScroll->isVisible() )
			size.y -= mHScroll->getSize().getHeight();
	}

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

	mVScroll->setPosition( mSize.getWidth() - mVScroll->getSize().getWidth(), 0 );
	mHScroll->setPosition( 0, mSize.getHeight() - mHScroll->getSize().getHeight() );

	mVScroll->setSize( mVScroll->getSize().getWidth(), mSize.getHeight() );
	mHScroll->setSize( mSize.getWidth() - ( mVScroll->isVisible() ? mVScroll->getSize().getWidth() : 0 ), mHScroll->getSize().getHeight() );

	mVScroll->setPageStep( (Float)mContainer->getSize().getHeight() / (Float)mScrollView->getSize().getHeight());
	mHScroll->setPageStep( (Float)mContainer->getSize().getWidth() / (Float)mScrollView->getSize().getWidth() );

	updateScroll();
}

void UIScrollView::updateScroll() {
	mScrollView->setPosition(
		-( mHScroll->getSlider()->getValue() * ( mScrollView->getSize().getWidth() - mSize.getWidth() ) ),
		-( mVScroll->getSlider()->getValue() * ( mScrollView->getSize().getHeight() - mSize.getHeight() ) )
	);
}

void UIScrollView::onValueChangeCb( const UIEvent * Event ) {
	updateScroll();
}

void UIScrollView::onTouchDragValueChange( Vector2f diff ) {
	if ( mVScroll->isEnabled() )
		mVScroll->setValue( mVScroll->getValue() + ( -diff.y / (Float)( mScrollView->getSize().getHeight() ) ) );

	if ( mHScroll->isEnabled() )
		mHScroll->setValue( mHScroll->getValue() + ( -diff.x / (Float)( mScrollView->getSize().getWidth() ) ) );
}

bool UIScrollView::isTouchOverAllowedChilds() {
	bool ret = mViewType == Exclusive ? !mVScroll->isMouseOverMeOrChilds() && !mHScroll->isMouseOverMeOrChilds() : true;
	return isMouseOverMeOrChilds() && mScrollView->isMouseOver() && ret;
}

void UIScrollView::loadFromXmlNode( const pugi::xml_node& node ) {
	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "type" == name ) {
			std::string val( ait->as_string() );
			String::toLowerInPlace( val );

			if ( "inclusive" == val ) setViewType( Inclusive );
			else if ( "exclusive" == val ) setViewType( Exclusive );
		} else if ( "vscroll_mode" == name ) {
			std::string val( ait->as_string() );
			String::toLowerInPlace( val );

			if ( "on" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
			else if ( "off" == val ) setVerticalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
			else if ( "auto" == val ) setVerticalScrollMode( UI_SCROLLBAR_AUTO );
		} else if ( "hscroll_mode" == name ) {
			std::string val( ait->as_string() );
			String::toLowerInPlace( val );

			if ( "on" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
			else if ( "off" == val ) setHorizontalScrollMode( UI_SCROLLBAR_ALWAYS_ON );
			else if ( "auto" == val ) setHorizontalScrollMode( UI_SCROLLBAR_AUTO );
		}
	}
}

}}
