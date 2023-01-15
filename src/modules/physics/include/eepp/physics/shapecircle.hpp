#ifndef EE_PHYSICS_CSHAPECIRCLE_HPP
#define EE_PHYSICS_CSHAPECIRCLE_HPP

#include <eepp/physics/shape.hpp>

namespace EE { namespace Physics {

class EE_PHYSICS_API ShapeCircle : public Shape {
  public:
	static ShapeCircle* New( Physics::Body* body, cpFloat radius, cVect offset );

	ShapeCircle( Physics::Body* body, cpFloat radius, cVect offset );

	cVect getOffset();

	virtual void setOffset( const cVect& offset );

	cpFloat getRadius();

	virtual void setRadius( const cpFloat& radius );

	virtual void draw( Space* space );
};

}} // namespace EE::Physics

#endif
