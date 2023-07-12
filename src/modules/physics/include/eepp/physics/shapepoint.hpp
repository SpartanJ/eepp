#ifndef EE_PHYSICS_CSHAPEPOINT_HPP
#define EE_PHYSICS_CSHAPEPOINT_HPP

#include <eepp/physics/shape.hpp>

namespace EE { namespace Physics {

class EE_PHYSICS_API ShapePoint : public Shape {
  public:
	static ShapePoint* New( Physics::Body* body, cpFloat radius, cVect offset );

	ShapePoint( Physics::Body* body, cpFloat radius, cVect offset );

	cVect getOffset();

	virtual void setOffset( const cVect& offset );

	cpFloat getRadius();

	virtual void setRadius( const cpFloat& radius );

	virtual void draw( Space* space );

#ifdef PHYSICS_RENDERER_ENABLED
	cpFloat getDrawRadius();

	virtual void setDrawRadius( const cpFloat& radius );

  protected:
	cpFloat mDrawRadius;
#endif
};

}} // namespace EE::Physics

#endif
