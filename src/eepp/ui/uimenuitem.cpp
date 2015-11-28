#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

UIMenuItem::UIMenuItem( UIPushButton::CreateParams& Params ) : 
	UIPushButton( Params )
{
	ApplyDefaultTheme();
}

UIMenuItem::~UIMenuItem() {
}

Uint32 UIMenuItem::Type() const {
	return UI_TYPE_MENUITEM;
}

bool UIMenuItem::IsType( const Uint32& type ) const {
	return UIMenuItem::Type() == type ? true : UIPushButton::IsType( type );
}

void UIMenuItem::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "menuitem" );
	DoAfterSetTheme();
}

Uint32 UIMenuItem::OnMouseEnter( const Vector2i &Pos, const Uint32 Flags ) {
	UIPushButton::OnMouseEnter( Pos, Flags );

	reinterpret_cast<UIMenu*> ( Parent() )->SetItemSelected( this );

	return 1;
}

void UIMenuItem::OnStateChange() {
	UIMenu * tMenu = reinterpret_cast<UIMenu*> ( Parent() );

	if ( mSkinState->GetState() == UISkinState::StateSelected ) {
		mTextBox->Color( tMenu->mFontSelectedColor );
	} else if ( mSkinState->GetState() == UISkinState::StateMouseEnter ) {
		mTextBox->Color( tMenu->mFontOverColor );
	} else {
		mTextBox->Color( tMenu->mFontColor );
	}
}

}}
