#ifndef EE_PHYSICS_CSHAPECIRCLE_HPP
#define EE_PHYSICS_CSHAPECIRCLE_HPP

#include "cshape.hpp"

namespace EE { namespace Physics {

class cShapeCircle : public cShape {
	public:
		static cShapeCircle * New( cBody * body, cpFloat radius, cVect offset );

		cShapeCircle( cBody * body, cpFloat radius, cVect offset );

		cVect Offset();

		void Offset( const cVect& offset );

		cpFloat Radius();

		void Radius( const cpFloat& radius );

		virtual void Draw( cSpace * space );
};

}}

#endif
