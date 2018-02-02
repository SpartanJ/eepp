#include <eepp/scene/mouseevent.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene {

MouseEvent::MouseEvent( Node * Ctrl, const Uint32& EventNum, const Vector2i& Pos, const Uint32& Flags ) :
	Event( Ctrl, EventNum ),
	mPos( Pos ),
	mFlags( Flags )
{
}

MouseEvent::~MouseEvent()
{
}

const Vector2i& MouseEvent::getPosition() const {
	return mPos;
}

const Uint32& MouseEvent::getFlags() const {
	return mFlags;
}

}}

