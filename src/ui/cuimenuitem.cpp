#include "cuimenuitem.hpp"

namespace EE { namespace UI {

cUIMenuItem::cUIMenuItem( cUIPushButton::CreateParams& Params ) : 
	cUIPushButton( Params )
{
	mType |= UI_TYPE_GET( UI_TYPE_MENUITEM );
	
	ApplyDefaultTheme();
}

cUIMenuItem::~cUIMenuItem() {
}

void cUIMenuItem::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "menuitem" );
	DoAfterSetTheme();
}

}}
