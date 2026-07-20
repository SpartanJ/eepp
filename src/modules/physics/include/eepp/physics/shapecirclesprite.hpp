#ifndef EE_PHYSICS_SHAPECIRCLESPRITE_HPP
#define EE_PHYSICS_SHAPECIRCLESPRITE_HPP

#include <eepp/physics/shapecircle.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/sprite.hpp>
using namespace EE::Graphics;

namespace EE { namespace Physics {

class EE_PHYSICS_API ShapeCircleSprite : public ShapeCircle {
  public:
	static ShapeCircleSprite* New( Physics::Body* body, cpFloat radius, cVect offset,
								   SpritePtr sprite );

	ShapeCircleSprite( Physics::Body* body, cpFloat radius, cVect offset, SpritePtr sprite );

	virtual ~ShapeCircleSprite();

	virtual void draw( Space* space );

	virtual void setRadius( const cpFloat& radius );

	virtual void setOffset( const cVect& offset );

	const SpritePtr& getSprite() const;

  protected:
	SpritePtr mSprite;

	void offsetSet();
};

}} // namespace EE::Physics

#endif

#endif
