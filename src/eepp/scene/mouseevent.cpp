#include <eepp/scene/mouseevent.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

MouseEvent::MouseEvent( Node* node, const Uint32& EventNum, const Vector2i& pos,
						const Uint32& flags ) :
	Event( node, EventNum ), mPos( pos ), mFlags( flags ) {}

MouseEvent::~MouseEvent() {}

const Vector2i& MouseEvent::getPosition() const {
	return mPos;
}

const Uint32& MouseEvent::getFlags() const {
	return mFlags;
}

}} // namespace EE::Scene
