#ifndef EE_PHYSICS_CSHAPEPOINT_HPP
#define EE_PHYSICS_CSHAPEPOINT_HPP

#include <eepp/physics/cshape.hpp>

CP_NAMESPACE_BEGIN

class CP_API cShapePoint : public cShape {
	public:
		static cShapePoint * New( cBody * body, cpFloat radius, cVect offset );

		cShapePoint( cBody * body, cpFloat radius, cVect offset );

		cVect Offset();

		virtual void Offset( const cVect& offset );

		cpFloat Radius();

		virtual void Radius( const cpFloat& radius );

		virtual void Draw( cSpace * space );

		#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat DrawRadius();

		virtual void DrawRadius( const cpFloat& radius );
	protected:
		cpFloat mDrawRadius;
		#endif
};

CP_NAMESPACE_END

#endif
