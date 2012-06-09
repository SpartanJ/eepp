#include <eepp/ui/cuieventkey.hpp>
#include <eepp/ui/cuicontrol.hpp>

namespace EE { namespace UI {

cUIEventKey::cUIEventKey( cUIControl * Ctrl, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) :
	cUIEvent( Ctrl, EventNum ),
	mKeyCode( KeyCode ),
	mChar( Char ),
	mMod( Mod )
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

const Uint32& cUIEventKey::Mod() const {
	return mMod;
}

}}
