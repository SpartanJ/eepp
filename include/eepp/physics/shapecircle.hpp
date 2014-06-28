#ifndef EE_PHYSICS_CSHAPECIRCLE_HPP
#define EE_PHYSICS_CSHAPECIRCLE_HPP

#include <eepp/physics/shape.hpp>

CP_NAMESPACE_BEGIN

class CP_API ShapeCircle : public Shape {
	public:
		static ShapeCircle * New( Physics::Body * body, cpFloat radius, cVect offset );

		ShapeCircle( Physics::Body * body, cpFloat radius, cVect offset );

		cVect Offset();

		virtual void Offset( const cVect& offset );

		cpFloat Radius();

		virtual void Radius( const cpFloat& radius );

		virtual void Draw( Space * space );
};

CP_NAMESPACE_END

#endif
