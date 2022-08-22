#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIPopUpMenu* UIPopUpMenu::New() {
	return eeNew( UIPopUpMenu, () );
}

UIPopUpMenu::UIPopUpMenu() : UIMenu() {
	setElementTag( "popupmenu" );
	setVisible( false );
	setEnabled( false );
	applyDefaultTheme();
}

UIPopUpMenu::~UIPopUpMenu() {
}

Uint32 UIPopUpMenu::getType() const {
	return UI_TYPE_POPUPMENU;
}

bool UIPopUpMenu::isType( const Uint32& type ) const {
	return UIPopUpMenu::getType() == type ? true : UIMenu::isType( type );
}

void UIPopUpMenu::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "popupmenu" );
	onThemeLoaded();
}

bool UIPopUpMenu::show() {
	if ( !isVisible() || 0.f == mAlpha ) {
		setEnabled( true );
		setVisible( true );
		toFront();
		if ( NULL != getUISceneNode() &&
			 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
			if ( mHidingAction ) {
				removeAction( mHidingAction );
				mHidingAction = nullptr;
			}
			runAction( Actions::Sequence::New(
				Actions::Fade::New(
					255.f == mAlpha ? 0.f : mAlpha, 255.f,
					getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
				Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) ) );
		}
		sendCommonEvent( Event::OnMenuShow );
		setFocus();
		return true;
	}

	return false;
}

bool UIPopUpMenu::hide() {
	if ( isVisible() && !mHidingAction ) {
		if ( NULL != getUISceneNode() &&
			 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
			mHidingAction = Actions::Sequence::New(
				Actions::FadeOut::New(
					getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
				Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ),
									 Actions::Runnable::New( [&] {
										 mHidingAction = nullptr;
										 if ( mCloseOnHide )
											 close();
									 } ) ) );
			runAction( mHidingAction );
		} else {
			setEnabled( false );
			setVisible( false );
			if ( mCloseOnHide )
				close();
		}
		safeHide();
		sendCommonEvent( Event::OnMenuHide );
		return true;
	}
	return false;
}

bool UIPopUpMenu::isHiding() const {
	return mHidingAction != nullptr;
}

bool UIPopUpMenu::getCloseOnHide() const {
	return mCloseOnHide;
}

void UIPopUpMenu::setCloseOnHide( bool closeOnHide ) {
	mCloseOnHide = closeOnHide;
}

}} // namespace EE::UI
