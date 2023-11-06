#ifndef EE_SCENEEVENTKEY_HPP
#define EE_SCENEEVENTKEY_HPP

#include <eepp/core/string.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/window/keycodes.hpp>

using namespace EE::Window;

namespace EE { namespace Scene {

class EE_API KeyEvent : public Event {
  public:
	KeyEvent( Node* node, const Uint32& eventNum, const Keycode& keyCode, const Scancode& scancode,
			  const Uint32& chr, const Uint32& mod );

	KeyEvent( const KeyEvent& event );

	~KeyEvent();

	const Keycode& getKeyCode() const;

	const Scancode& getScancode() const;

	const String::StringBaseType& getChar() const;

	const Uint32& getMod() const;

	/** The modifier key mask only for CTRL ALT SHIFT and META (no caps, num, etc) */
	Uint32 getSanitizedMod() const;

  protected:
	Keycode mKeyCode{ Keycode::KEY_UNKNOWN };
	Scancode mScancode{ Scancode::SCANCODE_UNKNOWN };
	String::StringBaseType mChar{ 0 };
	Uint32 mMod{ 0 };
};

class EE_API TextInputEvent : public Event {
  public:
	TextInputEvent( Node* node, const Uint32& eventNum, const Uint32& chr,
					const Uint32& timestamp );

	const String::StringBaseType& getChar() const;

	const Uint32& getTimestamp() const;

	String getText() const;

  protected:
	String::StringBaseType mChar;
	Uint32 mTimestamp;
};

class EE_API TextEditingEvent : public Event {
  public:
	TextEditingEvent( Node* node, const Uint32& eventNum, const String& text, const Int32& start,
					  const Int32& length );

	const String& getText() const;

	const Int32& getStart() const;

	const Int32& getLength() const;

  protected:
	String mText;
	Int32 mStart;
	Int32 mLength;
};

}} // namespace EE::Scene

#endif
