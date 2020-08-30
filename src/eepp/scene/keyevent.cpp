#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

KeyEvent::KeyEvent( Node* node, const Uint32& eventNum, const Keycode& keyCode, const Uint32& chr,
					const Uint32& mod ) :
	Event( node, eventNum ), mKeyCode( keyCode ), mChar( chr ), mMod( mod ) {}

KeyEvent::KeyEvent( const KeyEvent& event ) :
	Event( event.getNode(), event.getType() ),
	mKeyCode( event.getKeyCode() ),
	mChar( event.getChar() ),
	mMod( event.getMod() ) {}

KeyEvent::~KeyEvent() {}

const Keycode& KeyEvent::getKeyCode() const {
	return mKeyCode;
}

const String::StringBaseType& KeyEvent::getChar() const {
	return mChar;
}

const Uint32& KeyEvent::getMod() const {
	return mMod;
}

TextInputEvent::TextInputEvent( Node* node, const Uint32& eventNum, const Uint32& chr,
								const Uint32& timestamp ) :
	Event( node, eventNum ), mChar( chr ), mTimestamp( timestamp ) {}

const String::StringBaseType& TextInputEvent::getChar() const {
	return mChar;
}

const Uint32& TextInputEvent::getTimestamp() const {
	return mTimestamp;
}

String TextInputEvent::getText() const {
	return String( (String::StringBaseType)mChar );
}

}} // namespace EE::Scene
