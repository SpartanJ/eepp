#include <eepp/scene/event.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

Event::Event( Node* node, const Uint32& eventType ) : mNode( node ), mEventType( eventType ) {}

Event::~Event() {}

Node* Event::getNode() const {
	return mNode;
}

const Uint32& Event::getType() const {
	return mEventType;
}

}} // namespace EE::Scene
