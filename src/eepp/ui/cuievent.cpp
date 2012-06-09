#include <eepp/ui/cuievent.hpp>
#include <eepp/ui/cuicontrol.hpp>

namespace EE { namespace UI {

cUIEvent::cUIEvent( cUIControl * Ctrl, const Uint32& EventType ) :
	mCtrl( Ctrl ),
	mEventType( EventType )
{
}

cUIEvent::~cUIEvent()
{
}

cUIControl * cUIEvent::Ctrl() const {
	return mCtrl;
}

const Uint32& cUIEvent::EventType() const {
	return mEventType;
}

}}
