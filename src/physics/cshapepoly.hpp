#ifndef EE_PHYSICS_CSHAPEPOLY_HPP
#define EE_PHYSICS_CSHAPEPOLY_HPP

#include "cshape.hpp"

namespace EE { namespace Physics {

class cShapePoly : public cShape {
	public:
		cShapePoly( cBody * body, int numVerts, cpVect *verts, cpVect offset );

		cShapePoly( cBody * body, cpFloat width, cpFloat height );

		static bool Validate( const cpVect * verts, const int numVerts );

		int GetNumVerts();

		cpVect GetVert( int idx );

		void SetVerts( int numVerts, cpVect *verts, cpVect offset );

		virtual void Draw( cSpace * space );
};

}}

#endif
