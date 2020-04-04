#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

UIMenuItem* UIMenuItem::New() {
	return eeNew( UIMenuItem, () );
}

UIMenuItem::UIMenuItem( const std::string& tag ) : UIPushButton( tag ) {
	mIcon->setElementTag( getElementTag() + "::icon" );
	mTextBox->setElementTag( getElementTag() + "::text" );
	applyDefaultTheme();
}

UIMenuItem::UIMenuItem() : UIMenuItem( "menu::item" ) {}

UIMenuItem::~UIMenuItem() {}

Uint32 UIMenuItem::getType() const {
	return UI_TYPE_MENUITEM;
}

bool UIMenuItem::isType( const Uint32& type ) const {
	return UIMenuItem::getType() == type ? true : UIPushButton::isType( type );
}

void UIMenuItem::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "menuitem" );
	onThemeLoaded();
}

Uint32 UIMenuItem::onMouseOver( const Vector2i& Pos, const Uint32& Flags ) {
	UIPushButton::onMouseOver( Pos, Flags );

	getParent()->asType<UIMenu>()->setItemSelected( this );

	return 1;
}

}} // namespace EE::UI
