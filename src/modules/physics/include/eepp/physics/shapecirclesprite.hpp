#ifndef EE_PHYSICS_SHAPECIRCLESPRITE_HPP
#define EE_PHYSICS_SHAPECIRCLESPRITE_HPP

#include <eepp/physics/shapecircle.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}} // namespace EE::Graphics
using namespace EE::Graphics;

namespace EE { namespace Physics {

class EE_PHYSICS_API ShapeCircleSprite : public ShapeCircle {
  public:
	static ShapeCircleSprite* New( Physics::Body* body, cpFloat radius, cVect offset,
								   Sprite* Sprite, bool AutoDeleteSprite = false );

	ShapeCircleSprite( Physics::Body* body, cpFloat radius, cVect offset, Sprite* Sprite,
					   bool AutoDeleteSprite = false );

	virtual ~ShapeCircleSprite();

	virtual void draw( Space* space );

	virtual void setRadius( const cpFloat& radius );

	virtual void setOffset( const cVect& offset );

	Sprite* getSprite() const;

  protected:
	Sprite* mSprite;
	bool mSpriteAutoDelete;

	void offsetSet();
};

}} // namespace EE::Physics

#endif

#endif
