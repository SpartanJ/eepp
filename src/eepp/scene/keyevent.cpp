#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace Scene {

KeyEvent::KeyEvent( Node* node, const Uint32& eventNum, const Keycode& keyCode,
					const Scancode& scancode, const Uint32& chr, const Uint32& mod ) :
	Event( node, eventNum ),
	mKeyCode( keyCode ),
	mScancode( scancode ),
	mChar( chr ),
	mMod( mod ) {}

KeyEvent::KeyEvent( const KeyEvent& event ) :
	Event( event.getNode(), event.getType() ),
	mKeyCode( event.getKeyCode() ),
	mScancode( event.getScancode() ),
	mChar( event.getChar() ),
	mMod( event.getMod() ) {}

KeyEvent::~KeyEvent() {}

const Keycode& KeyEvent::getKeyCode() const {
	return mKeyCode;
}

const Scancode& KeyEvent::getScancode() const {
	return mScancode;
}

const String::StringBaseType& KeyEvent::getChar() const {
	return mChar;
}

const Uint32& KeyEvent::getMod() const {
	return mMod;
}

Uint32 KeyEvent::getSanitizedMod() const {
	return mMod & KEYMOD_CTRL_SHIFT_ALT_META;
}

bool TextInputEvent::isValidTextInputEvent( Input* input, const TextInputEvent& event ) {
	// Meta/Command key shortcuts do not generate text
	if ( input->isMetaPressed() )
		return false;

	// Ctrl shortcuts (without Alt/AltGr) do not generate text
	if ( input->isLeftControlPressed() && !input->isLeftAltPressed() && !input->isAltGrPressed() )
		return false;

	// Alt+Tab should not insert a tab character
	if ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' )
		return false;

#if EE_PLATFORM != EE_PLATFORM_MACOS
	// On non-macOS platforms, Alt key combinations (without Ctrl) do not generate text
	if ( input->isLeftAltPressed() && !input->isLeftControlPressed() )
		return false;
#endif

	return true;
}

bool TextInputEvent::isValid( Input* input ) const {
	return isValidTextInputEvent( input, *this );
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

TextEditingEvent::TextEditingEvent( Node* node, const Uint32& eventNum, const String& text,
									const Int32& start, const Int32& length ) :
	Event( node, eventNum ), mText( text ), mStart( start ), mLength( length ) {}

const String& TextEditingEvent::getText() const {
	return mText;
}

const Int32& TextEditingEvent::getStart() const {
	return mStart;
}

const Int32& TextEditingEvent::getLength() const {
	return mLength;
}

}} // namespace EE::Scene
