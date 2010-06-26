#include "cuimessage.hpp"
#include "cuicontrol.hpp"

namespace EE { namespace UI {

cUIMessage::cUIMessage( cUIControl * Ctrl, const Uint32& Msg ) : 
	mCtrl( Ctrl ), 
	mMsg( Msg )
{
}

cUIMessage::~cUIMessage()
{
}

cUIControl * cUIMessage::Sender() const {
	return mCtrl;
}

Uint32 cUIMessage::Msg( void ) const {
	return mMsg;
}

}}