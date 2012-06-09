#ifndef EE_UICUIEVENTKEY_HPP
#define EE_UICUIEVENTKEY_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/cuievent.hpp>

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIEventKey : public cUIEvent {
	public:
		cUIEventKey( cUIControl * Ctrl, const Uint32& EventNum, const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		~cUIEventKey();

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
