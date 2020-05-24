#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

KeyEvent::KeyEvent( Node* node, const Uint32& eventNum, const Uint32& keyCode, const Uint32& chr,
					const Uint32& mod ) :
	Event( node, eventNum ), mKeyCode( keyCode ), mChar( chr ), mMod( mod ) {}

KeyEvent::~KeyEvent() {}

const Uint32& KeyEvent::getKeyCode() const {
	return mKeyCode;
}

const Uint32& KeyEvent::getChar() const {
	return mChar;
}

const Uint32& KeyEvent::getMod() const {
	return mMod;
}

TextInputEvent::TextInputEvent( Node* node, const Uint32& eventNum, const Uint32& chr,
								const Uint32& timestamp ) :
	Event( node, eventNum ), mChar( chr ), mTimestamp( timestamp ) {}

const Uint32& TextInputEvent::getChar() const {
	return mChar;
}

const Uint32& TextInputEvent::getTimestamp() const {
	return mTimestamp;
}

String TextInputEvent::getText() const {
	return String( mChar );
}

}} // namespace EE::Scene
