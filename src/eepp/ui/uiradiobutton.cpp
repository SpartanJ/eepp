#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UIRadioButton::UIRadioButton( const UITextBox::CreateParams& Params ) :
	UITextBox( Params ),
	mActiveButton(NULL),
	mInactiveButton(NULL),
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

	autoActivate();

	applyDefaultTheme();
}

UIRadioButton::~UIRadioButton() {
}

Uint32 UIRadioButton::getType() const {
	return UI_TYPE_RADIOBUTTON;
}

bool UIRadioButton::isType( const Uint32& type ) const {
	return UIRadioButton::getType() == type ? true : UITextBox::isType( type );
}

void UIRadioButton::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "radiobutton" );

	mActiveButton->setThemeControl	( Theme, "radiobutton_active" );
	mInactiveButton->setThemeControl( Theme, "radiobutton_inactive" );

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

void UIRadioButton::autoSize() {
	UITextBox::autoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->centerVertical();
		mInactiveButton->centerVertical();

		mSize.width( (int)mTextCache->getTextWidth() + mActiveButton->size().width() );
	}
}

void UIRadioButton::onSizeChange() {
	UITextBox::onSizeChange();

	mActiveButton->centerVertical();
	mInactiveButton->centerVertical();
}

Uint32 UIRadioButton::onMessage( const UIMessage * Msg ) {
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

void UIRadioButton::switchState() {
	active( !mActive );
}

void UIRadioButton::active( const bool& active ) {
	if ( !active ) {
		if ( checkActives() ) {
			mActiveButton->visible( false );
			mInactiveButton->visible( true );

			mActive = false;

			onValueChange();
		}
	} else {
		mActiveButton->visible( true );
		mInactiveButton->visible( false );

		mActive = true;

		onValueChange();
	}

	if ( active && NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->childGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->active() )
						tRB->active( false );
				}
			}

			tChild = tChild->nextGet();
		}
	}
}

bool UIRadioButton::checkActives() {
	if ( NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->childGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->active() )
						return true;
				}
			}

			tChild = tChild->nextGet();
		}
	}

	return false;
}

void UIRadioButton::autoActivate() {
	eeASSERT( NULL != mParentCtrl );

	if ( NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->childGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->active() ) {
						return;
					}
				}
			}

			tChild = tChild->nextGet();
		}
	}

	active( true );
}

const bool& UIRadioButton::active() const {
	return mActive;
}

const bool& UIRadioButton::isActive() const {
	return active();
}

void UIRadioButton::padding( const Recti& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->size().width();
}

UIControlAnim * UIRadioButton::activeButton() const {
	return mActiveButton;
}

UIControlAnim * UIRadioButton::inactiveButton() const {
	return mInactiveButton;
}

Uint32 UIRadioButton::onKeyDown( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			active( true );
		}
	}

	return UITextBox::onKeyDown( Event );
}

void UIRadioButton::onAlphaChange() {
	UITextBox::onAlphaChange();
	
	mActiveButton->alpha( mAlpha );
	mInactiveButton->alpha( mAlpha );
}

}}
