#include <eepp/ui/uievent.hpp>
#include <eepp/ui/uicontrol.hpp>

namespace EE { namespace UI {

UIEvent::UIEvent(UIControl * control, const Uint32& eventType ) :
	mCtrl( control ),
	mEventType( eventType )
{
}

UIEvent::~UIEvent()
{
}

UIControl * UIEvent::getControl() const {
	return mCtrl;
}

const Uint32& UIEvent::getEventType() const {
	return mEventType;
}

}}
