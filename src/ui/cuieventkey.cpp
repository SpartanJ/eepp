#include "cuieventkey.hpp"
#include "cuicontrol.hpp"

namespace EE { namespace UI {

cUIEventKey::cUIEventKey( cUIControl * Ctrl, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char ) :
	cUIEvent( Ctrl, EventNum ),
	mKeyCode( KeyCode ),
	mChar( Char )
{
}

cUIEventKey::~cUIEventKey()
{
}

const Uint32& cUIEventKey::KeyCode() const {
	return mKeyCode;
}

const Uint16& cUIEventKey::Char() const {
	return mChar;
}

}}
