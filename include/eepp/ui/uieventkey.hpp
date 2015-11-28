#ifndef EE_UICUIEVENTKEY_HPP
#define EE_UICUIEVENTKEY_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uievent.hpp>

namespace EE { namespace UI {

class UIControl;

class EE_API UIEventKey : public UIEvent {
	public:
		UIEventKey( UIControl * Ctrl, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		~UIEventKey();

		const Uint32& KeyCode() const;

		const Uint16& Char() const;

		const Uint32& Mod() const;
	protected:
		Uint32	mKeyCode;
		Uint16	mChar;
		Uint32	mMod;
};

}}

#endif
