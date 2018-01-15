#ifndef EE_UICUIEVENTKEY_HPP
#define EE_UICUIEVENTKEY_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uievent.hpp>

namespace EE { namespace UI {

class UIControl;

class EE_API UIEventKey : public UIEvent {
	public:
		UIEventKey( UIControl * getControl, const Uint32& EventNum, const Uint32& getKeyCode, const Uint16& getChar, const Uint32& getMod );

		~UIEventKey();

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
