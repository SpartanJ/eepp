#ifndef EE_PHYSICS_CSHAPEPOINT_HPP
#define EE_PHYSICS_CSHAPEPOINT_HPP

#include <eepp/physics/shape.hpp>

CP_NAMESPACE_BEGIN

class CP_API ShapePoint : public Shape {
	public:
		static ShapePoint * New( Physics::Body * body, cpFloat radius, cVect offset );

		ShapePoint( Physics::Body * body, cpFloat radius, cVect offset );

		cVect offset();

		virtual void offset( const cVect& offset );

		cpFloat radius();

		virtual void radius( const cpFloat& radius );

		virtual void draw( Space * space );

		#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat drawRadius();

		virtual void drawRadius( const cpFloat& radius );
	protected:
		cpFloat mDrawRadius;
		#endif
};

CP_NAMESPACE_END

#endif
