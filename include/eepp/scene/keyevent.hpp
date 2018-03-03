#ifndef EE_SCENEEVENTKEY_HPP
#define EE_SCENEEVENTKEY_HPP

#include <eepp/scene/event.hpp>

namespace EE { namespace Scene {

class EE_API KeyEvent : public Event {
	public:
		KeyEvent( Node * node, const Uint32& EventNum, const Uint32& getKeyCode, const Uint16& getChar, const Uint32& getMod );

		~KeyEvent();

		const Uint32& getKeyCode() const;

		const Uint16& getChar() const;

		const Uint32& getMod() const;
	protected:
		Uint32	mKeyCode;
		Uint16	mChar;
		Uint32	mMod;
};

}}

#endif
