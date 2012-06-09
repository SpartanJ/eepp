#include <eepp/ui/cuimessage.hpp>
#include <eepp/ui/cuicontrol.hpp>

namespace EE { namespace UI {

cUIMessage::cUIMessage( cUIControl * Ctrl, const Uint32& Msg, const Uint32& Flags ) :
	mCtrl( Ctrl ),
	mMsg( Msg ),
	mFlags( Flags )
{
}

cUIMessage::~cUIMessage()
{
}

cUIControl * cUIMessage::Sender() const {
	return mCtrl;
}

const Uint32& cUIMessage::Msg() const {
	return mMsg;
}

const Uint32& cUIMessage::Flags() const {
	return mFlags;
}

}}
