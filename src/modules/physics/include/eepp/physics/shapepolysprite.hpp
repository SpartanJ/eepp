#ifndef EE_PHYSICS_CSHAPEPOLYSPRITE_HPP
#define EE_PHYSICS_CSHAPEPOLYSPRITE_HPP

#include <eepp/physics/shapepoly.hpp>

#ifdef PHYSICS_RENDERER_ENABLED

namespace EE { namespace Graphics {
class Sprite;
}} // namespace EE::Graphics
using namespace EE::Graphics;

namespace EE { namespace Physics {

class EE_PHYSICS_API ShapePolySprite : public ShapePoly {
  public:
	static ShapePolySprite* New( Physics::Body* body, int numVerts, cVect* verts, cVect offset,
								 Sprite* Sprite, bool AutoDeleteSprite = false );

	static ShapePolySprite* New( Physics::Body* body, cpFloat width, cpFloat height, Sprite* Sprite,
								 bool AutoDeleteSprite = false );

	ShapePolySprite( Physics::Body* body, int numVerts, cVect* verts, cVect offset, Sprite* Sprite,
					 bool AutoDeleteSprite = false );

	ShapePolySprite( Physics::Body* body, cpFloat width, cpFloat height, Sprite* Sprite,
					 bool AutoDeleteSprite = false );

	virtual ~ShapePolySprite();

	virtual void draw( Space* space );

	Sprite* getSprite() const;

  protected:
	Sprite* mSprite;
	bool mSpriteAutoDelete;
	Vector2i mOffset;

	void offsetSet( cVect center );
};

}} // namespace EE::Physics

#endif

#endif
