#ifndef EE_PHYSICS_CSHAPECIRCLE_HPP
#define EE_PHYSICS_CSHAPECIRCLE_HPP

#include <eepp/physics/cshape.hpp>

CP_NAMESPACE_BEGIN

class CP_API cShapeCircle : public cShape {
	public:
		static cShapeCircle * New( cBody * body, cpFloat radius, cVect offset );

		cShapeCircle( cBody * body, cpFloat radius, cVect offset );

		cVect Offset();

		virtual void Offset( const cVect& offset );

		cpFloat Radius();

		virtual void Radius( const cpFloat& radius );

		virtual void Draw( cSpace * space );
};

CP_NAMESPACE_END

#endif
