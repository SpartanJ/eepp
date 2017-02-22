#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UICheckBox::UICheckBox( const UITextBox::CreateParams& Params ) :
	UITextBox( Params ),
	mActive( false ),
	mTextSeparation( 4 )
{
	UIControlAnim::CreateParams ButtonParams( Params );

	ButtonParams.setParent( this );
	ButtonParams.setPosition( Vector2i( 0, 0 ) );
	ButtonParams.Size = Sizei( 16, 16 );

	mActiveButton 	= eeNew( UIControlAnim, ( ButtonParams ) );
	mActiveButton->setVisible( false );
	mActiveButton->setEnabled( true );

	mInactiveButton = eeNew( UIControlAnim, ( ButtonParams ) );
	mInactiveButton->setVisible( true );
	mInactiveButton->setEnabled( true );

	setPadding( Recti(0,0,0,0) );

	applyDefaultTheme();
}

UICheckBox::UICheckBox() :
	UITextBox(),
	mActive( false ),
	mTextSeparation( dpToPxI( 4 ) )
{
	mActiveButton 	= eeNew( UIControlAnim, () );
	mActiveButton->setVisible( false );
	mActiveButton->setEnabled( true );
	mActiveButton->setParent( this );
	mActiveButton->setPosition( 0, 0 );
	mActiveButton->setSize( dpToPxI( 16 ), dpToPxI( 16 ) );

	mInactiveButton = eeNew( UIControlAnim, () );
	mInactiveButton->setVisible( true );
	mInactiveButton->setEnabled( true );
	mInactiveButton->setParent( this );
	mInactiveButton->setPosition( 0, 0 );
	mInactiveButton->setSize( dpToPxI( 16 ), dpToPxI( 16 ) );

	setPadding( Recti(0,0,0,0) );

	applyDefaultTheme();
}


UICheckBox::~UICheckBox() {
}

Uint32 UICheckBox::getType() const {
	return UI_TYPE_CHECKBOX;
}

bool UICheckBox::isType( const Uint32& type ) const {
	return UICheckBox::getType() == type ? true : UITextBox::isType( type );
}

void UICheckBox::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "checkbox" );

	mActiveButton->setThemeControl	( Theme, "checkbox_active" );
	mInactiveButton->setThemeControl( Theme, "checkbox_inactive" );

	doAftersetTheme();
}

void UICheckBox::doAftersetTheme() {
	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = mActiveButton->getSkin();

	if ( tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mActiveButton->setSize( dpToPxI( tSubTexture->getRealSize() ) );
			mActiveButton->centerVertical();
		}
	}

	tSkin = mInactiveButton->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mInactiveButton->setSize( dpToPxI( tSubTexture->getRealSize() ) );
			mInactiveButton->centerVertical();
		}
	}

	setPadding( Recti(0,0,0,0) );
}

void UICheckBox::autoSize() {
	UITextBox::autoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->centerVertical();
		mInactiveButton->centerVertical();

		setInternalWidth( (int)mTextCache->getTextWidth() + mActiveButton->getSize().getWidth() );
	}
}

void UICheckBox::onSizeChange() {
	UITextBox::onSizeChange();

	mActiveButton->centerVertical();
	mInactiveButton->centerVertical();
}

Uint32 UICheckBox::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgClick: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				switchState();
			}

			if ( Msg->getSender() == mActiveButton || Msg->getSender() == mInactiveButton ) {
				sendMouseEvent( UIEvent::EventMouseClick, UIManager::instance()->getMousePos(), UIManager::instance()->getPressTrigger() );
			}

			return 1;
		}
	}

	return 0;
}

void UICheckBox::switchState() {
	setActive( !mActive );
}

void UICheckBox::setActive( const bool& active ) {
	if ( !active ) {
		mActiveButton->setVisible( false );
		mInactiveButton->setVisible( true );

		mActive = false;
	} else {
		mActiveButton->setVisible( true );
		mInactiveButton->setVisible( false );

		mActive = true;
	}

	onValueChange();
}

const bool& UICheckBox::isActive() const {
	return mActive;
}

void UICheckBox::setPadding( const Recti& padding ) {
	UITextBox::setPadding( padding );

	mActiveButton->setPosition( mPadding.Left, mActiveButton->getPosition().y );
	mInactiveButton->setPosition( mPadding.Left, mInactiveButton->getPosition().y );

	mRealPadding.Left = mActiveButton->getPosition().x + mActiveButton->getSize().getWidth() + dpToPxI( mTextSeparation );
}

UIControlAnim * UICheckBox::getActiveButton() const {
	return mActiveButton;
}

UIControlAnim * UICheckBox::getInactiveButton() const {
	return mInactiveButton;
}

Int32 UICheckBox::getTextSeparation() const {
	return mTextSeparation;
}

void UICheckBox::setTextSeparation(const Int32 & textSeparation) {
	mTextSeparation = textSeparation;

	setPadding( getPadding() );
}

Uint32 UICheckBox::onKeyDown( const UIEventKey& Event ) {
	UITextBox::onKeyDown( Event );

	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			setActive( !mActive );
		}
	}

	return 1;
}

void UICheckBox::onAlphaChange() {
	UITextBox::onAlphaChange();
	
	mActiveButton->setAlpha( mAlpha );
	mInactiveButton->setAlpha( mAlpha );
}

}}
