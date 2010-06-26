#ifndef EE_UICUIEVENTKEY_HPP
#define EE_UICUIEVENTKEY_HPP

#include "base.hpp"
#include "cuievent.hpp"

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIEventKey : public cUIEvent {
	public:
		cUIEventKey( cUIControl * Ctrl, const Uint32& KeyCode, const Uint16& Char );

		~cUIEventKey();

		Uint32 KeyCode() const;

		Uint16 Char() const;
	protected:
		Uint32	mKeyCode;
		Uint16	mChar;
};

}}

#endif 
