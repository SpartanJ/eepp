#include <eepp/ui/uieventkey.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace Scene {

UIEventKey::UIEventKey( Node * Ctrl, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) :
	UIEvent( Ctrl, EventNum ),
	mKeyCode( KeyCode ),
	mChar( Char ),
	mMod( Mod )
{
}

UIEventKey::~UIEventKey()
{
}

const Uint32& UIEventKey::getKeyCode() const {
	return mKeyCode;
}

const Uint16& UIEventKey::getChar() const {
	return mChar;
}

const Uint32& UIEventKey::getMod() const {
	return mMod;
}

}}
