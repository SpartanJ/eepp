#ifndef EE_SCENEEVENTKEY_HPP
#define EE_SCENEEVENTKEY_HPP

#include <eepp/core/string.hpp>
#include <eepp/scene/event.hpp>

namespace EE { namespace Scene {

class EE_API KeyEvent : public Event {
  public:
	KeyEvent( Node* node, const Uint32& eventNum, const Uint32& keyCode, const Uint32& chr,
			  const Uint32& mod );

	~KeyEvent();

	const Uint32& getKeyCode() const;

	const Uint32& getChar() const;

	const Uint32& getMod() const;

  protected:
	Uint32 mKeyCode;
	Uint32 mChar;
	Uint32 mMod;
};

class EE_API TextInputEvent : public Event {
  public:
	TextInputEvent( Node* node, const Uint32& eventNum, const Uint32& chr,
					const Uint32& timestamp );

	const Uint32& getChar() const;

	const Uint32& getTimestamp() const;

	String getText() const;

  protected:
	Uint32 mChar;
	Uint32 mTimestamp;
};

}} // namespace EE::Scene

#endif
