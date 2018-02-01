#include <eepp/ui/uievent.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

UIEvent::UIEvent( Node * control, const Uint32& eventType ) :
	mCtrl( control ),
	mEventType( eventType )
{
}

UIEvent::~UIEvent()
{
}

Node * UIEvent::getControl() const {
	return mCtrl;
}

const Uint32& UIEvent::getEventType() const {
	return mEventType;
}

}}
