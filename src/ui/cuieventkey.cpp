#include "cuieventkey.hpp"
#include "cuicontrol.hpp"

namespace EE { namespace UI {

cUIEventKey::cUIEventKey( cUIControl * Ctrl, const Uint32& KeyCode, const Uint16& Char ) :
	cUIEvent( Ctrl ), 
	mKeyCode( KeyCode ), 
	mChar( Char )
{
}

cUIEventKey::~cUIEventKey()
{
}

Uint32 cUIEventKey::KeyCode() const {
	return mKeyCode;
}

Uint16 cUIEventKey::Char() const {
	return mChar;
}
	
}}