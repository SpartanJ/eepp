#include <eepp/ui/uievent.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

UIEvent::UIEvent(UINode * control, const Uint32& eventType ) :
	mCtrl( control ),
	mEventType( eventType )
{
}

UIEvent::~UIEvent()
{
}

UINode * UIEvent::getControl() const {
	return mCtrl;
}

const Uint32& UIEvent::getEventType() const {
	return mEventType;
}

}}
