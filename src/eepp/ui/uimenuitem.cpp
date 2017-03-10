#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

UIMenuItem * UIMenuItem::New() {
	return eeNew( UIMenuItem, () );
}

UIMenuItem::UIMenuItem() :
	UIPushButton()
{
	applyDefaultTheme();
}

UIMenuItem::~UIMenuItem() {
}

Uint32 UIMenuItem::getType() const {
	return UI_TYPE_MENUITEM;
}

bool UIMenuItem::isType( const Uint32& type ) const {
	return UIMenuItem::getType() == type ? true : UIPushButton::isType( type );
}

void UIMenuItem::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );
	setThemeControl( Theme, "menuitem" );
	onThemeLoaded();
}

Uint32 UIMenuItem::onMouseEnter( const Vector2i &Pos, const Uint32 Flags ) {
	UIPushButton::onMouseEnter( Pos, Flags );

	reinterpret_cast<UIMenu*> ( getParent() )->setItemSelected( this );

	return 1;
}

void UIMenuItem::onStateChange() {
	UIMenu * tMenu = reinterpret_cast<UIMenu*> ( getParent() );

	if ( mSkinState->getState() == UISkinState::StateSelected ) {
		mTextBox->setFontColor( tMenu->getFontStyleConfig().getFontSelectedColor() );
	} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
		mTextBox->setFontColor( tMenu->getFontStyleConfig().getFontOverColor() );
	} else {
		mTextBox->setFontColor( tMenu->getFontStyleConfig().getFontColor() );
	}
}

}}
