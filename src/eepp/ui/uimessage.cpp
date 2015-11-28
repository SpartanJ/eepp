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

UIControl * UIMessage::Sender() const {
	return mCtrl;
}

const Uint32& UIMessage::Msg() const {
	return mMsg;
}

const Uint32& UIMessage::Flags() const {
	return mFlags;
}

}}
