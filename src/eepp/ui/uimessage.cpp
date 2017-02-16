#include <eepp/ui/uimessage.hpp>
#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

UIMessage::UIMessage( UIControl * Ctrl, const Uint32& Msg, const Uint32& Flags ) :
	mCtrl( Ctrl ),
	mMsg( Msg ),
	mFlags( Flags )
{
}

UIMessage::~UIMessage()
{
}

UIControl * UIMessage::getSender() const {
	return mCtrl;
}

const Uint32& UIMessage::getMsg() const {
	return mMsg;
}

const Uint32& UIMessage::getFlags() const {
	return mFlags;
}

}}
