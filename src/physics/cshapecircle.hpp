#ifndef EE_PHYSICS_CSHAPECIRCLE_HPP
#define EE_PHYSICS_CSHAPECIRCLE_HPP

#include "cshape.hpp"

namespace EE { namespace Physics {

class cShapeCircle : public cShape {
	public:
		cShapeCircle( cBody * body, cpFloat radius, cpVect offset );

		cpVect Offset();

		void Offset( const cpVect& offset );

		cpFloat Radius();

		void Radius( const cpFloat& radius );

		virtual void Draw( cSpace * space );
};

}}

#endif
