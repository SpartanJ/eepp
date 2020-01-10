#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

KeyEvent::KeyEvent( Node* node, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char,
					const Uint32& Mod ) :
	Event( node, EventNum ), mKeyCode( KeyCode ), mChar( Char ), mMod( Mod ) {}

KeyEvent::~KeyEvent() {}

const Uint32& KeyEvent::getKeyCode() const {
	return mKeyCode;
}

const Uint16& KeyEvent::getChar() const {
	return mChar;
}

const Uint32& KeyEvent::getMod() const {
	return mMod;
}

}} // namespace EE::Scene
