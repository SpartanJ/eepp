#ifndef EE_UICUIMESSAGE_HPP
#define EE_UICUIMESSAGE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace UI {

class UIControl;

class EE_API UIMessage {
	public:
		enum Message
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
			MsgDragStart,
			MsgDragEnd,
			MsgUser,
			MsgForceDWord = eeINDEX_NOT_FOUND
		};

		UIMessage( UIControl * Ctrl, const Uint32& Msg, const Uint32& Flags = MsgForceDWord );

		~UIMessage();

		UIControl * Sender() const;

		const Uint32& Msg() const;

		const Uint32& Flags() const;
	private:
		UIControl *	mCtrl;
		Uint32			mMsg;
		Uint32			mFlags;
};

}}

#endif
