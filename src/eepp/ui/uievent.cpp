#include <eepp/ui/uievent.hpp>
#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

UIEvent::UIEvent( UIControl * Ctrl, const Uint32& EventType ) :
	mCtrl( Ctrl ),
	mEventType( EventType )
{
}

UIEvent::~UIEvent()
{
}

UIControl * UIEvent::Ctrl() const {
	return mCtrl;
}

const Uint32& UIEvent::EventType() const {
	return mEventType;
}

}}
