#include <eepp/ui/uieventkey.hpp>
#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

UIEventKey::UIEventKey( UIControl * Ctrl, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) :
	UIEvent( Ctrl, EventNum ),
	mKeyCode( KeyCode ),
	mChar( Char ),
	mMod( Mod )
{
}

UIEventKey::~UIEventKey()
{
}

const Uint32& UIEventKey::KeyCode() const {
	return mKeyCode;
}

const Uint16& UIEventKey::Char() const {
	return mChar;
}

const Uint32& UIEventKey::Mod() const {
	return mMod;
}

}}
