#include <eepp/ui/cuimenuitem.hpp>
#include <eepp/ui/cuimenu.hpp>

namespace EE { namespace UI {

cUIMenuItem::cUIMenuItem( cUIPushButton::CreateParams& Params ) : 
	cUIPushButton( Params )
{
	ApplyDefaultTheme();
}

cUIMenuItem::~cUIMenuItem() {
}

Uint32 cUIMenuItem::Type() const {
	return UI_TYPE_MENUITEM;
}

bool cUIMenuItem::IsType( const Uint32& type ) const {
	return cUIMenuItem::Type() == type ? true : cUIPushButton::IsType( type );
}

void cUIMenuItem::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "menuitem" );
	DoAfterSetTheme();
}

Uint32 cUIMenuItem::OnMouseEnter( const eeVector2i &Pos, const Uint32 Flags ) {
	cUIPushButton::OnMouseEnter( Pos, Flags );

	reinterpret_cast<cUIMenu*> ( Parent() )->SetItemSelected( this );

	return 1;
}

void cUIMenuItem::OnStateChange() {
	cUIMenu * tMenu = reinterpret_cast<cUIMenu*> ( Parent() );

	if ( mSkinState->GetState() == cUISkinState::StateSelected ) {
		mTextBox->Color( tMenu->mFontSelectedColor );
	} else if ( mSkinState->GetState() == cUISkinState::StateMouseEnter ) {
		mTextBox->Color( tMenu->mFontOverColor );
	} else {
		mTextBox->Color( tMenu->mFontColor );
	}
}

}}
