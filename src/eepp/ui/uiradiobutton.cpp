#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UIRadioButton * UIRadioButton::New() {
	return eeNew( UIRadioButton, () );
}

UIRadioButton::UIRadioButton() :
	UITextBox(),
	mActiveButton(NULL),
	mInactiveButton(NULL),
	mActive( false ),
	mTextSeparation( 4 )
{
	mActiveButton 	= UIControlAnim::New();
	mActiveButton->setVisible( false );
	mActiveButton->setEnabled( true );
	mActiveButton->setParent( this );
	mActiveButton->setPosition( 0, 0 );
	mActiveButton->setSize( 16, 16 );

	mInactiveButton = UIControlAnim::New();
	mInactiveButton->setVisible( true );
	mInactiveButton->setEnabled( true );
	mInactiveButton->setParent( this );
	mInactiveButton->setPosition( 0, 0 );
	mInactiveButton->setSize( 16, 16 );

	setPadding( Recti(0,0,0,0) );

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

	onThemeLoaded();
}

void UIRadioButton::onThemeLoaded() {
	UISkin * tSkin = mActiveButton->getSkin();

	if ( tSkin ) {
		mActiveButton->setSize( tSkin->getSize() );
		mActiveButton->centerVertical();
	}

	tSkin = mInactiveButton->getSkin();

	if ( NULL != tSkin ) {
		mInactiveButton->setSize( tSkin->getSize() );
		mInactiveButton->centerVertical();
	}

	mMinControlSize = mActiveButton->getSkinSize();

	setPadding( Recti(0,0,0,0) );
}

void UIRadioButton::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->centerVertical();
		mInactiveButton->centerVertical();

		setInternalPixelsWidth( (int)mTextCache->getTextWidth() + mActiveButton->getRealSize().getWidth() );
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
				sendMouseEvent( UIEvent::EventMouseClick, UIManager::instance()->getMousePos(), UIManager::instance()->getPressTrigger() );
			}

			return 1;
		}
	}

	return 0;
}

void UIRadioButton::switchState() {
	setActive( !mActive );
}

void UIRadioButton::setActive( const bool& active ) {
	if ( !active ) {
		if ( checkActives() ) {
			mActiveButton->setVisible( false );
			mInactiveButton->setVisible( true );

			mActive = false;

			onValueChange();
		}
	} else {
		mActiveButton->setVisible( true );
		mInactiveButton->setVisible( false );

		mActive = true;

		onValueChange();
	}

	if ( active && NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->getFirstChild();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->isActive() )
						tRB->setActive( false );
				}
			}

			tChild = tChild->getNextControl();
		}
	}
}

bool UIRadioButton::checkActives() {
	if ( NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->getFirstChild();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->isActive() )
						return true;
				}
			}

			tChild = tChild->getNextControl();
		}
	}

	return false;
}

void UIRadioButton::autoActivate() {
	eeASSERT( NULL != mParentCtrl );

	if ( NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->getFirstChild();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->isActive() ) {
						return;
					}
				}
			}

			tChild = tChild->getNextControl();
		}
	}

	setActive( true );
}

const bool& UIRadioButton::isActive() const {
	return mActive;
}

void UIRadioButton::setPadding( const Recti& padding ) {
	UITextBox::setPadding( padding );

	mActiveButton->setPosition( mPadding.Left, mActiveButton->getPosition().y );
	mInactiveButton->setPosition( mPadding.Left, mInactiveButton->getPosition().y );

	mRealPadding.Left = mActiveButton->getRealPosition().x + mActiveButton->getRealSize().getWidth() + PixelDensity::dpToPxI( mTextSeparation  );
}

UIControlAnim * UIRadioButton::getActiveButton() const {
	return mActiveButton;
}

UIControlAnim * UIRadioButton::getInactiveButton() const {
	return mInactiveButton;
}

Int32 UIRadioButton::getTextSeparation() const {
	return mTextSeparation;
}

void UIRadioButton::setTextSeparation(const Int32 & textSeparation) {
	mTextSeparation = textSeparation;

	setPadding( getPadding() );
}

Uint32 UIRadioButton::onKeyDown( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			setActive( true );
		}
	}

	return UITextBox::onKeyDown( Event );
}

void UIRadioButton::onAlphaChange() {
	UITextBox::onAlphaChange();
	
	mActiveButton->setAlpha( mAlpha );
	mInactiveButton->setAlpha( mAlpha );
}

}}
