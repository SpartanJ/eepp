#ifndef EE_SCENENODEMESSAGE_HPP
#define EE_SCENENODEMESSAGE_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace Scene {

class Node;

class EE_API NodeMessage {
	public:
		enum Message
		{
			Click = 0,
			DoubleClick,
			MouseEnter,
			MouseExit,
			MouseDown,
			MouseUp,
			WindowResize,
			Focus,
			FocusLoss,
			CellClicked,
			Selected,
			DragStart,
			DragStop,
			LayoutAttributeChange,
			UserMessage,
			NoMessage = eeINDEX_NOT_FOUND
		};

		NodeMessage( Node * node, const Uint32& Msg, const Uint32& Flags = NoMessage );

		~NodeMessage();

		Node * getSender() const;

		const Uint32& getMsg() const;

		const Uint32& getFlags() const;
	private:
		Node *	mNode;
		Uint32	mMsg;
		Uint32	mFlags;
};

}}

#endif
