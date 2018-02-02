#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/textureregion.hpp>

namespace EE { namespace UI {

UIScrollBar * UIScrollBar::New( const UI_ORIENTATION& orientation ) {
	return eeNew( UIScrollBar, ( orientation ) );
}

UIScrollBar::UIScrollBar( const UI_ORIENTATION& orientation ) :
	UIWidget(),
#ifdef EE_PLATFORM_TOUCH
	mScrollBarType( NoButtons )
#else
	mScrollBarType( TwoButtons )
#endif
{
	mFlags |= UI_AUTO_SIZE;

	mBtnDown	= UINode::New();
	mBtnUp		= UINode::New();
	mBtnUp->setParent( this );
	mBtnUp->setSize( 16, 16 );
	mBtnDown->setParent( this );
	mBtnDown->setSize( 16, 16 );

	mSlider		= UISlider::New();
	mSlider->setOrientation( orientation );
	mSlider->setParent( this );
	mSlider->setAllowHalfSliderOut( false );
	mSlider->setExpandBackground( false );

	mSlider->addEventListener( Event::OnValueChange, cb::Make1( this, &UIScrollBar::onValueChangeCb ) );

	adjustChilds();

	applyDefaultTheme();
}

UIScrollBar::~UIScrollBar() {
}

Uint32 UIScrollBar::getType() const {
	return UI_TYPE_SCROLLBAR;
}

bool UIScrollBar::isType( const Uint32& type ) const {
	return UIScrollBar::getType() == type ? true : UIWidget::isType( type );
}

void UIScrollBar::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	if ( !isVertical() ) {
		UINode::setThemeSkin( Theme, "hscrollbar" );
		mSlider->setThemeSkin( Theme, "hscrollbar_slider" );
		mSlider->getBackSlider()->setThemeSkin( Theme, "hscrollbar_bg" );
		mSlider->getSliderButton()->setThemeSkin( Theme, "hscrollbar_button" );
		mBtnUp->setThemeSkin( Theme, "hscrollbar_btnup" );
		mBtnDown->setThemeSkin( Theme, "hscrollbar_btndown" );
	} else {
		UINode::setThemeSkin( Theme, "vscrollbar" );
		mSlider->setThemeSkin( Theme, "vscrollbar_slider" );
		mSlider->getBackSlider()->setThemeSkin( Theme, "vscrollbar_bg" );
		mSlider->getSliderButton()->setThemeSkin( Theme, "vscrollbar_button" );
		mBtnUp->setThemeSkin( Theme, "vscrollbar_btnup" );
		mBtnDown->setThemeSkin( Theme, "vscrollbar_btndown" );
	}

	UISkin * tSkin = NULL;

	tSkin = mBtnUp->getSkin();

	if ( NULL != tSkin ) {
		mBtnUp->setSize( tSkin->getSize() );
	}

	tSkin = mBtnDown->getSkin();

	if ( NULL != tSkin ) {
		mBtnDown->setSize( tSkin->getSize() );
	}

	adjustChilds();

	mSlider->adjustChilds();
}

void UIScrollBar::onAutoSize() {
	UISkin * tSkin = mSlider->getBackSlider()->getSkin();

	if ( NULL != tSkin ) {
		Sizef size = tSkin->getSize();

		mMinControlSize = PixelDensity::pxToDp( size );

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( mSlider->isVertical() ) {
				mSlider->setSize( size.getWidth() , mDpSize.getHeight() );
				setSize( size.getWidth(), mDpSize.getHeight() );
			} else {
				mSlider->setSize( mDpSize.getWidth(), size.getHeight() );
				setSize( mDpSize.getWidth(), size.getHeight() );
			}
		}
	} else if ( NULL != mSlider->getSliderButton() ) {
		UISkin * tSkin = mSlider->getSliderButton()->getSkin();

		if ( NULL != tSkin ) {
			Sizef size = tSkin->getSize();

			if ( mFlags & UI_AUTO_SIZE ) {
				if ( mSlider->isVertical() ) {
					setSize( size.getWidth(), mDpSize.getHeight() );
				} else {
					setSize( mDpSize.getWidth(), size.getHeight() );
				}
			}
		}
	}
}

void UIScrollBar::onSizeChange() {
	adjustChilds();

	mSlider->adjustChilds();

	UIWidget::onSizeChange();
}

void UIScrollBar::adjustChilds() {
	onAutoSize();

	mBtnUp->setPosition( 0, 0 );

	switch ( mScrollBarType ) {
		case NoButtons:
		{
			mBtnDown->setVisible( false )->setEnabled( false );
			mBtnUp->setVisible( false )->setEnabled( false );

			if ( !isVertical() ) {
				mSlider->setSize( mDpSize )->setPosition( 0, 0 )->centerVertical();
			} else {
				mSlider->setSize( mDpSize )->setPosition( 0, 0 )->centerHorizontal();
			}

			break;
		}
		case TwoButtons:
		default:
		{
			mBtnDown->setVisible( true )->setEnabled( true );
			mBtnUp->setVisible( true )->setEnabled( true );

			if ( !isVertical() ) {
				mBtnDown->setPosition( mDpSize.getWidth() - mBtnDown->getSize().getWidth(), 0 );
				mSlider->setSize( mDpSize.getWidth() - mBtnDown->getSize().getWidth() - mBtnUp->getSize().getWidth(), mDpSize.getHeight() );
				mSlider->setPosition( mBtnUp->getSize().getWidth(), 0 );

				mBtnDown->centerVertical();
				mBtnUp->centerVertical();
				mSlider->centerVertical();
			} else {
				mBtnDown->setPosition( 0, mDpSize.getHeight() - mBtnDown->getSize().getHeight() );
				mSlider->setSize( mDpSize.getWidth(), mDpSize.getHeight() - mBtnDown->getSize().getHeight() - mBtnUp->getSize().getHeight() );
				mSlider->setPosition( 0, mBtnUp->getSize().getHeight() );

				mBtnDown->centerHorizontal();
				mBtnUp->centerHorizontal();
				mSlider->centerHorizontal();
			}

			break;
		}
	}
}

void UIScrollBar::update( const Time& time ) {
	UINode::update( time );

	if ( mBtnUp->isMouseOver() || mBtnDown->isMouseOver() ) {
		manageClick( UIManager::instance()->getInput()->getClickTrigger() );
	}
}

void UIScrollBar::manageClick( const Uint32& Flags ) {
	if ( Flags & EE_BUTTONS_WUWD ) {
		if ( Flags & EE_BUTTON_WUMASK )
			mSlider->setValue( getValue() + getClickStep() );
		else
			mSlider->setValue( getValue() - getClickStep() );
	}
}

Uint32 UIScrollBar::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mBtnUp ) {
					mSlider->setValue( getValue() - getClickStep() );
				} else if ( Msg->getSender() == mBtnDown ) {
					mSlider->setValue( getValue() + getClickStep() );
				}
			}

			return 1;
		}
	}

	return 0;
}

void UIScrollBar::setValue( Float Val ) {
	mSlider->setValue( Val );
}

const Float& UIScrollBar::getValue() const {
	return mSlider->getValue();
}

void UIScrollBar::setMinValue( const Float& MinVal ) {
	mSlider->setMinValue( MinVal );
}

const Float& UIScrollBar::getMinValue() const {
	return mSlider->getMinValue();
}

void UIScrollBar::setMaxValue( const Float& MaxVal ) {
	mSlider->setMaxValue( MaxVal );
}

const Float& UIScrollBar::getMaxValue() const {
	return mSlider->getMaxValue();
}

void UIScrollBar::setClickStep( const Float& step ) {
	mSlider->setClickStep( step );
}

const Float& UIScrollBar::getClickStep() const {
	return mSlider->getClickStep();
}

Float UIScrollBar::getPageStep() const {
	return mSlider->getPageStep();
}

void UIScrollBar::setPageStep(const Float& pageStep) {
	mSlider->setPageStep( pageStep );
}

bool UIScrollBar::isVertical() const {
	return mSlider->isVertical();
}

void UIScrollBar::onValueChangeCb( const Event * Event ) {
	onValueChange();
}

UISlider * UIScrollBar::getSlider() const {
	return mSlider;
}

UINode * UIScrollBar::getButtonUp() const {
	return mBtnUp;
}

UINode * UIScrollBar::getButtonDown() const {
	return mBtnDown;
}

bool UIScrollBar::getExpandBackground() const {
	return mSlider->getExpandBackground();
}

void UIScrollBar::setExpandBackground( bool expandBackground ) {
	mSlider->setExpandBackground( expandBackground );

	adjustChilds();
}

void UIScrollBar::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	mSlider->loadFromXmlNode( node );

	endPropertiesTransaction();
}

UIScrollBar::ScrollBarType UIScrollBar::getScrollBarType() const {
	return mScrollBarType;
}

void UIScrollBar::setScrollBarType( const ScrollBarType & scrollBarType ) {
	if ( mScrollBarType != scrollBarType ) {
		mScrollBarType = scrollBarType;

		adjustChilds();
	}
}

UI_ORIENTATION UIScrollBar::getOrientation() const {
	return mSlider->getOrientation();
}

UINode * UIScrollBar::setOrientation( const UI_ORIENTATION & orientation ) {
	mSlider->setOrientation( orientation );

	applyDefaultTheme();

	return this;
}

void UIScrollBar::onAlphaChange() {
	UINode::onAlphaChange();
	
	mSlider->setAlpha( mAlpha );
	mBtnUp->setAlpha( mAlpha );
	mBtnDown->setAlpha( mAlpha );
}

}}
