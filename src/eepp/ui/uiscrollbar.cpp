#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIScrollBar::UIScrollBar( const UIScrollBar::CreateParams& Params ) :
	UIComplexControl( Params )
{
	UIControlAnim::CreateParams CParams = Params;
	CParams.Size = Sizei( 16, 16 );
	CParams.setParent( this );

	mBtnDown	= eeNew( UIControlAnim, ( CParams ) );
	mBtnUp		= eeNew( UIControlAnim, ( CParams ) );

	mBtnDown->setVisible( true );
	mBtnDown->setEnabled( true );
	mBtnUp->setVisible( true );
	mBtnUp->setEnabled( true );

	UISlider::CreateParams SParams;
	SParams.Background = Params.Background;
	SParams.Blend = Params.Blend;
	SParams.Border = Params.Border;
	SParams.Flags = Params.Flags;
	SParams.setParent( this );
	SParams.Pos = Params.Pos;
	SParams.Size = Params.Size;
	SParams.VerticalSlider = Params.VerticalScrollBar;
	SParams.AllowHalfSliderOut = false;
	SParams.ExpandBackground = true;

	mSlider		= eeNew( UISlider, ( SParams ) );
	mSlider->setVisible( true );
	mSlider->setEnabled( true );

	mSlider->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIScrollBar::onValueChangeCb ) );

	adjustChilds();

	applyDefaultTheme();
}

UIScrollBar::~UIScrollBar() {
}

Uint32 UIScrollBar::getType() const {
	return UI_TYPE_SCROLLBAR;
}

bool UIScrollBar::isType( const Uint32& type ) const {
	return UIScrollBar::getType() == type ? true : UIComplexControl::isType( type );
}

void UIScrollBar::setTheme( UITheme * Theme ) {
	if ( !isVertical() ) {
		UIControl::setThemeControl( Theme, "hscrollbar" );
		mSlider->setThemeControl( Theme, "hscrollbar_slider" );
		mSlider->getBackSlider()->setThemeControl( Theme, "hscrollbar_bg" );
		mSlider->getSliderButton()->setThemeControl( Theme, "hscrollbar_button" );
		mBtnUp->setThemeControl( Theme, "hscrollbar_btnup" );
		mBtnDown->setThemeControl( Theme, "hscrollbar_btndown" );
	} else {
		UIControl::setThemeControl( Theme, "vscrollbar" );
		mSlider->setThemeControl( Theme, "vscrollbar_slider" );
		mSlider->getBackSlider()->setThemeControl( Theme, "vscrollbar_bg" );
		mSlider->getSliderButton()->setThemeControl( Theme, "vscrollbar_button" );
		mBtnUp->setThemeControl( Theme, "vscrollbar_btnup" );
		mBtnDown->setThemeControl( Theme, "vscrollbar_btndown" );
	}

	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = NULL;

	tSkin = mBtnUp->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mBtnUp->setSize( tSubTexture->getRealSize() );
		}
	}

	tSkin = mBtnDown->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mBtnDown->setSize( tSubTexture->getRealSize() );
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		tSkin = mSlider->getBackSlider()->getSkin();

		if ( NULL != tSkin ) {
			tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

			if ( NULL != tSubTexture ) {
				if ( mSlider->isVertical() ) {
					mSlider->setSize( tSubTexture->getRealSize().getWidth() , mSize.getHeight() );
					setSize( tSubTexture->getRealSize().getWidth() , mSize.getHeight() );
					mMinControlSize.x = mSize.getWidth();
				} else {
					mSlider->setSize( mSize.getWidth(), tSubTexture->getRealSize().getHeight() );
					setSize( mSize.getWidth(), tSubTexture->getRealSize().getHeight() );
					mMinControlSize.y = mSize.getHeight();
				}
			}
		}
	}

	adjustChilds();

	mSlider->adjustChilds();
}

void UIScrollBar::onSizeChange() {
	adjustChilds();
	mSlider->adjustChilds();
	UIComplexControl::onSizeChange();
}

void UIScrollBar::adjustChilds() {
	mBtnUp->setPosition( 0, 0 );

	if ( !isVertical() ) {
		mBtnDown->setPosition( mSize.getWidth() - mBtnDown->getSize().getWidth(), 0 );
		mSlider->setSize( mSize.getWidth() - mBtnDown->getSize().getWidth() - mBtnUp->getSize().getWidth(), mSlider->getSize().getHeight() );
		mSlider->setPosition( mBtnUp->getSize().getWidth(), 0 );

		mBtnDown->centerVertical();
		mBtnUp->centerVertical();
		mSlider->centerVertical();
	} else {
		mBtnDown->setPosition( 0, mSize.getHeight() - mBtnDown->getSize().getHeight() );
		mSlider->setSize( mSlider->getSize().getWidth(), mSize.getHeight() - mBtnDown->getSize().getHeight() - mBtnUp->getSize().getHeight() );
		mSlider->setPosition( 0, mBtnUp->getSize().getHeight() );

		mBtnDown->centerHorizontal();
		mBtnUp->centerHorizontal();
		mSlider->centerHorizontal();
	}
}

void UIScrollBar::update() {
	UIControlAnim::update();

	if ( mBtnUp->isMouseOver() || mBtnDown->isMouseOver() ) {
		manageClick( UIManager::instance()->getInput()->getClickTrigger() );
	}
}

void UIScrollBar::manageClick( const Uint32& Flags ) {
	if ( Flags & EE_BUTTONS_WUWD ) {
		if ( Flags & EE_BUTTON_WUMASK )
			mSlider->value( value() + clickStep() );
		else
			mSlider->value( value() - clickStep() );
	}
}

Uint32 UIScrollBar::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mBtnUp ) {
					mSlider->value( value() - clickStep() );
				} else if ( Msg->getSender() == mBtnDown ) {
					mSlider->value( value() + clickStep() );
				}
			}

			return 1;
		}
	}

	return 0;
}

void UIScrollBar::value( Float Val ) {
	mSlider->value( Val );
}

const Float& UIScrollBar::value() const {
	return mSlider->value();
}

void UIScrollBar::minValue( const Float& MinVal ) {
	mSlider->minValue( MinVal );
}

const Float& UIScrollBar::minValue() const {
	return mSlider->minValue();
}

void UIScrollBar::maxValue( const Float& MaxVal ) {
	mSlider->maxValue( MaxVal );
}

const Float& UIScrollBar::maxValue() const {
	return mSlider->maxValue();
}

void UIScrollBar::clickStep( const Float& step ) {
	mSlider->clickStep( step );
}

const Float& UIScrollBar::clickStep() const {
	return mSlider->clickStep();
}

const bool& UIScrollBar::isVertical() const {
	return mSlider->isVertical();
}

void UIScrollBar::onValueChangeCb( const UIEvent * Event ) {
	onValueChange();
}

UISlider * UIScrollBar::getSlider() const {
	return mSlider;
}

UIControlAnim * UIScrollBar::getButtonUp() const {
	return mBtnUp;
}

UIControlAnim * UIScrollBar::getButtonDown() const {
	return mBtnDown;
}

void UIScrollBar::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mSlider->setAlpha( mAlpha );
	mBtnUp->setAlpha( mAlpha );
	mBtnDown->setAlpha( mAlpha );
}

}}
