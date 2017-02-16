#ifndef EE_PHYSICS_CSHAPECIRCLE_HPP
#define EE_PHYSICS_CSHAPECIRCLE_HPP

#include <eepp/physics/shape.hpp>

CP_NAMESPACE_BEGIN

class CP_API ShapeCircle : public Shape {
	public:
		static ShapeCircle * New( Physics::Body * body, cpFloat radius, cVect offset );

		ShapeCircle( Physics::Body * body, cpFloat radius, cVect offset );

		cVect offset();

		virtual void offset( const cVect& offset );

		cpFloat radius();

		virtual void radius( const cpFloat& radius );

		virtual void draw( Space * space );
};

CP_NAMESPACE_END

#endif
