#ifndef EE_UICUIMESSAGE_HPP
#define EE_UICUIMESSAGE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class cUIControl;

class EE_API cUIMessage {
	public:
		enum UIMessage
		{
			MsgClick = 0,
			MsgDoubleClick,
			MsgMouseEnter,
			MsgMouseExit,
			MsgMouseDown,
			MsgMouseUp,
			MsgWindowResize,
			MsgFocus,
			MsgFocusLoss,
			MsgCellClicked,
			MsgSelected,
			MsgUser,
			MsgForceDWord = eeINDEX_NOT_FOUND
		};

		cUIMessage( cUIControl * Ctrl, const Uint32& Msg, const Uint32& Flags = MsgForceDWord );

		~cUIMessage();

		cUIControl * Sender() const;

		const Uint32& Msg() const;

		const Uint32& Flags() const;
	private:
		cUIControl *	mCtrl;
		Uint32			mMsg;
		Uint32			mFlags;
};

}}

#endif
