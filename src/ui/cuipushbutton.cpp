#include "cuipushbutton.hpp"

namespace EE { namespace UI {

cUIPushButton::cUIPushButton( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params )
{
	mType |= UI_TYPE_GET(UI_TYPE_PUSHBUTTON);
}

cUIPushButton::~cUIPushButton() {
}

void cUIPushButton::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "button" );
}

}}
