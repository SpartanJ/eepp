#include <eepp/scene/event.hpp>
#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/mouseevent.hpp>
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

const Uint32& Event::getCallbackId() const {
	return mCallbackId;
}

const MouseEvent* Event::asMouseEvent() const {
	return static_cast<const MouseEvent*>( this );
}

const KeyEvent* Event::asKeyEvent() const {
	return static_cast<const KeyEvent*>( this );
}

const DropEvent* Event::asDropEvent() const {
	return static_cast<const DropEvent*>( this );
}

const TextEvent* Event::asTextEvent() const {
	return static_cast<const TextEvent*>( this );
}

const TextInputEvent* Event::asTextInputEvent() const {
	return static_cast<const TextInputEvent*>( this );
}

const WindowEvent* Event::asWindowEvent() const {
	return static_cast<const WindowEvent*>( this );
}

}} // namespace EE::Scene
