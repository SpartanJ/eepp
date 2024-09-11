#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

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

UIPopUpMenu::~UIPopUpMenu() {}

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

void UIPopUpMenu::showAtScreenPosition( Vector2f pos ) {
	nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, this );
	setPixelsPosition( pos );
	show();
}

void UIPopUpMenu::showOverMouseCursor() {
	showAtScreenPosition( getUISceneNode()->getWindow()->getInput()->getMousePos().asFloat() );
}

bool UIPopUpMenu::hide() {
	if ( isVisible() && !mHidingAction ) {
		if ( NULL != getUISceneNode() &&
			 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
			mHidingAction = Actions::Sequence::New(
				Actions::FadeOut::New(
					getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
				Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ),
									 Actions::Runnable::New( [this] {
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

void UIPopUpMenu::close() {
	UIMenu::close();

	auto menuSubMenus = findAllByType( UI_TYPE_MENUSUBMENU );
	for ( auto* menu : menuSubMenus )
		menu->asType<UIMenuSubMenu>()->getSubMenu()->close();
}

bool UIPopUpMenu::isHiding() const {
	return mHidingAction != nullptr;
}

bool UIPopUpMenu::closeOnHide() const {
	return mCloseOnHide;
}

void UIPopUpMenu::setCloseOnHide( bool closeOnHide ) {
	mCloseOnHide = closeOnHide;
}

bool UIPopUpMenu::closeSubMenusOnClose() const {
	return mCloseSubMenusOnClose;
}

void UIPopUpMenu::setCloseSubMenusOnClose( bool closeSubMenusOnClose ) {
	mCloseSubMenusOnClose = closeSubMenusOnClose;
}

}} // namespace EE::UI
