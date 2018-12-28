#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/scene/actions/actions.hpp>

namespace EE { namespace UI {

UIPopUpMenu * UIPopUpMenu::New() {
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
			runAction( Actions::Sequence::New( Actions::Fade::New( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->getControlsFadeOutTime() ),
										  Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) ) );
		}

		setFocus();

		return true;
	}

	return false;
}

bool UIPopUpMenu::hide() {
	if ( isVisible() ) {
		if ( NULL != mItemSelected )
			mItemSelected->popState( UIState::StateSelected );

		mItemSelected		= NULL;
		mItemSelectedIndex	= eeINDEX_NOT_FOUND;

		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New( Actions::FadeOut::New( UIThemeManager::instance()->getControlsFadeOutTime() ),
											   Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
		} else {
			setEnabled( false );
			setVisible( false );
		}

		return true;
	}

	return false;
}

void UIPopUpMenu::onWidgetFocusLoss() {
	hide();

	UIMenu::onWidgetFocusLoss();
}

Uint32 UIPopUpMenu::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp:
		{
			#ifdef EE_PLATFORM_TOUCH
			if ( mTE.getElapsed().asMilliseconds() > 250.f ) {
			#endif
				if ( !Msg->getSender()->isType( UI_TYPE_MENUSUBMENU ) && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
					sendCommonEvent( Event::OnHideByClick );

					if ( isVisible() && NULL != mSceneNode )
						mSceneNode->setFocus();

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
