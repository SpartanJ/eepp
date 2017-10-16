#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIPopUpMenu *UIPopUpMenu::New() {
	return eeNew( UIPopUpMenu, () );
}

UIPopUpMenu::UIPopUpMenu() :
	UIMenu()
{
	setVisible( false );
	setEnabled( false );
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
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "popupmenu" );
	onThemeLoaded();
}

bool UIPopUpMenu::show() {
	if ( !isVisible() || 0.f == mAlpha ) {
		#ifdef EE_PLATFORM_TOUCH
		mTE.restart();
		#endif

		setEnabled( true );
		setVisible( true );

		toFront();

		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			startAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->getControlsFadeInTime() );
		}

		setFocus();

		return true;
	}

	return false;
}

bool UIPopUpMenu::hide() {
	if ( isVisible() ) {
		if ( !isFadingOut() ) {
			if ( NULL != mItemSelected )
				mItemSelected->setSkinState( UISkinState::StateNormal );

			mItemSelected		= NULL;
			mItemSelectedIndex	= eeINDEX_NOT_FOUND;

			if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
				disableFadeOut( UIThemeManager::instance()->getControlsFadeOutTime() );
			} else {
				setEnabled( false );
				setVisible( false );
			}
		}

		return true;
	}

	return false;
}

void UIPopUpMenu::onWidgetFocusLoss() {
	hide();

	UIMenu::onWidgetFocusLoss();
}

Uint32 UIPopUpMenu::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MouseUp:
		{
			#ifdef EE_PLATFORM_TOUCH
			if ( mTE.getElapsed().asMilliseconds() > 250.f ) {
			#endif
				if ( !Msg->getSender()->isType( UI_TYPE_MENUSUBMENU ) && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
					sendCommonEvent( UIEvent::OnHideByClick );

					if ( isVisible() )
						UIManager::instance()->getMainControl()->setFocus();

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
