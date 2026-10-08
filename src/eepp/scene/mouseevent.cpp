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

MouseWheelEvent::MouseWheelEvent( Node* node, const Vector2i& position, const Vector2f& offset,
								  bool flipped ) :
	Event( node, Event::MouseWheel ), mPosition( position ), mOffset( offset ), mFlipped( flipped ) {}

const Vector2i& MouseWheelEvent::getPosition() const {
	return mPosition;
}

const Vector2f& MouseWheelEvent::getOffset() const {
	return mOffset;
}

bool MouseWheelEvent::isFlipped() const {
	return mFlipped;
}

}} // namespace EE::Scene
