#ifndef EE_UICUIMESSAGE_HPP
#define EE_UICUIMESSAGE_HPP

#include "base.hpp"

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIMessage {
	public:
		enum
		{
			ClickMsg = 0,
			MouseEnter,
			MouseExit,
			WindowResize,
			UserMsg,
			ForceDWord = 0xFFFFFFFF,
		};

		cUIMessage( cUIControl * Ctrl, const Uint32& Msg );

		~cUIMessage();

		cUIControl * Sender() const;

		Uint32 Msg( void ) const;
	private:
		cUIControl *	mCtrl;
		Uint32			mMsg;
};

}}

#endif
