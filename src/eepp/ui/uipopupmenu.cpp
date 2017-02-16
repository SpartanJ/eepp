#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIPopUpMenu::UIPopUpMenu( UIPopUpMenu::CreateParams Params ) :
	UIMenu( Params )
{
	applyDefaultTheme();
}

UIPopUpMenu::~UIPopUpMenu() {
	onClose();
}

Uint32 UIPopUpMenu::getType() const {
	return UI_TYPE_POPUPMENU;
}

bool UIPopUpMenu::isType( const Uint32& type ) const {
	return UIPopUpMenu::getType() == type ? true : UIMenu::isType( type );
}

void UIPopUpMenu::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "popupmenu" );
	doAftersetTheme();
}

bool UIPopUpMenu::show() {
	if ( !visible() || 0.f == mAlpha ) {
		#ifdef EE_PLATFORM_TOUCH
		mTE.Restart();
		#endif

		enabled( true );
		visible( true );

		toFront();

		if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
			startAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->controlsFadeInTime() );
		}

		setFocus();

		return true;
	}

	return false;
}

bool UIPopUpMenu::hide() {
	if ( visible() ) {
		if ( !isFadingOut() ) {
			if ( NULL != mItemSelected )
				mItemSelected->setSkinState( UISkinState::StateNormal );

			mItemSelected		= NULL;
			mItemSelectedIndex	= eeINDEX_NOT_FOUND;

			if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
				disableFadeOut( UIThemeManager::instance()->controlsFadeOutTime() );
			} else {
				enabled( false );
				visible( false );
			}
		}

		return true;
	}

	return false;
}

void UIPopUpMenu::onComplexControlFocusLoss() {
	hide();

	UIMenu::onComplexControlFocusLoss();
}

Uint32 UIPopUpMenu::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgMouseUp:
		{
			#ifdef EE_PLATFORM_TOUCH
			if ( mTE.Elapsed().AsMilliseconds() > 250.f ) {
			#endif
				if ( !Msg->getSender()->isType( UI_TYPE_MENUSUBMENU ) && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
					sendCommonEvent( UIEvent::EventOnHideByClick );

					if ( visible() )
						UIManager::instance()->mainControl()->setFocus();

					hide();
				}
			#ifdef EE_PLATFORM_TOUCH
			}
			#endif
		}
	}

	return UIMenu::onMessage( Msg );
}

}}
