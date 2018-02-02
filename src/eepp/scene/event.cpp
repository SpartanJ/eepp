#include <eepp/scene/event.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

Event::Event( Node * control, const Uint32& eventType ) :
	mCtrl( control ),
	mEventType( eventType )
{
}

Event::~Event()
{
}

Node * Event::getNode() const {
	return mCtrl;
}

const Uint32& Event::getType() const {
	return mEventType;
}

}}
