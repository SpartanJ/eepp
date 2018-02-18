#ifndef EE_SCENEMOUSEEVENT_HPP
#define EE_SCENEMOUSEEVENT_HPP

#include <eepp/scene/event.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

namespace EE { namespace Scene {

class EE_API MouseEvent : public Event {
	public:
		MouseEvent( Node * node, const Uint32& EventNum, const Vector2i& Pos, const Uint32& getFlags );

		~MouseEvent();

		const Vector2i& getPosition() const;

		const Uint32& getFlags() const;
	protected:
		Vector2i mPos;
		Uint32 mFlags;
};

}}

#endif
 
