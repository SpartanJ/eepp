#ifndef EE_PHYSICS_CSHAPEPOLYSPRITE_HPP
#define EE_PHYSICS_CSHAPEPOLYSPRITE_HPP

#include <eepp/physics/shapepoly.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

#include <eepp/graphics/sprite.hpp>
using namespace EE::Graphics;

namespace EE { namespace Physics {

class EE_PHYSICS_API ShapePolySprite : public ShapePoly {
  public:
	static ShapePolySprite* New( Physics::Body* body, int numVerts, cVect* verts, cVect offset,
								 SpritePtr sprite );

	static ShapePolySprite* New( Physics::Body* body, cpFloat width, cpFloat height,
								 SpritePtr sprite );

	ShapePolySprite( Physics::Body* body, int numVerts, cVect* verts, cVect offset,
					 SpritePtr sprite );

	ShapePolySprite( Physics::Body* body, cpFloat width, cpFloat height, SpritePtr sprite );

	virtual ~ShapePolySprite();

	virtual void draw( Space* space );

	const SpritePtr& getSprite() const;

  protected:
	SpritePtr mSprite;
	Vector2i mOffset;

	void offsetSet( cVect center );
};

}} // namespace EE::Physics

#endif

#endif
