#ifndef EE_SCENEMOUSEEVENT_HPP
#define EE_SCENEMOUSEEVENT_HPP

#include <eepp/math/vector2.hpp>
#include <eepp/scene/event.hpp>
using namespace EE::Math;

namespace EE { namespace Scene {

class EE_API MouseEvent : public Event {
  public:
	MouseEvent( Node* node, const Uint32& EventNum, const Vector2i& pos, const Uint32& flags );

	~MouseEvent();

	const Vector2i& getPosition() const;

	const Uint32& getFlags() const;

  protected:
	Vector2i mPos;
	Uint32 mFlags;
};

class EE_API MouseWheelEvent : public Event {
  public:
	MouseWheelEvent( Node* node, const Vector2i& position, const Vector2f& offset, bool flipped );

	const Vector2i& getPosition() const;

	const Vector2f& getOffset() const;

	bool isFlipped() const;

  protected:
	Vector2i mPosition;
	Vector2f mOffset;
	bool mFlipped;
};

}} // namespace EE::Scene

#endif
