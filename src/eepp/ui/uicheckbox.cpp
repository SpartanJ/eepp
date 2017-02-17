#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UICheckBox::UICheckBox( const UITextBox::CreateParams& Params ) :
	UITextBox( Params ),
	mActive( false )
{
	UIControlAnim::CreateParams ButtonParams( Params );

	ButtonParams.setParent( this );
	ButtonParams.setPos( Vector2i( 0, 0 ) );
	ButtonParams.Size = Sizei( 16, 16 );

	mActiveButton 	= eeNew( UIControlAnim, ( ButtonParams ) );
	mActiveButton->visible( false );
	mActiveButton->enabled( true );

	mInactiveButton = eeNew( UIControlAnim, ( ButtonParams ) );
	mInactiveButton->visible( true );
	mInactiveButton->enabled( true );

	padding( Recti(0,0,0,0) );

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
			mActiveButton->size( tSubTexture->realSize() );
			mActiveButton->centerVertical();
		}
	}

	tSkin = mInactiveButton->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mInactiveButton->size( tSubTexture->realSize() );
			mInactiveButton->centerVertical();
		}
	}

	padding( Recti(0,0,0,0) );
}

void UICheckBox::autoSize() {
	UITextBox::autoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->centerVertical();
		mInactiveButton->centerVertical();

		mSize.setWidth( (int)mTextCache->getTextWidth() + mActiveButton->size().getWidth() );
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
				sendMouseEvent( UIEvent::EventMouseClick, UIManager::instance()->getMousePos(), UIManager::instance()->pressTrigger() );
			}

			return 1;
		}
	}

	return 0;
}

void UICheckBox::switchState() {
	active( !mActive );
}

void UICheckBox::active( const bool& active ) {
	if ( !active ) {
		mActiveButton->visible( false );
		mInactiveButton->visible( true );

		mActive = false;
	} else {
		mActiveButton->visible( true );
		mInactiveButton->visible( false );

		mActive = true;
	}

	onValueChange();
}

const bool& UICheckBox::active() const {
	return mActive;
}

const bool& UICheckBox::isActive() const {
	return active();
}

void UICheckBox::padding( const Recti& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->size().getWidth();
}

UIControlAnim * UICheckBox::activeButton() const {
	return mActiveButton;
}

UIControlAnim * UICheckBox::inactiveButton() const {
	return mInactiveButton;
}

Uint32 UICheckBox::onKeyDown( const UIEventKey& Event ) {
	UITextBox::onKeyDown( Event );

	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			active( !mActive );
		}
	}

	return 1;
}

void UICheckBox::onAlphaChange() {
	UITextBox::onAlphaChange();
	
	mActiveButton->alpha( mAlpha );
	mInactiveButton->alpha( mAlpha );
}

}}
