#ifndef EE_PHYSICS_CSHAPEPOLY_HPP
#define EE_PHYSICS_CSHAPEPOLY_HPP

#include "cshape.hpp"

namespace EE { namespace Physics {

class cShapePoly : public cShape {
	public:
		static cShapePoly * New( cBody * body, int numVerts, cVect *verts, cVect offset );

		static cShapePoly * New( cBody * body, cpFloat width, cpFloat height );

		cShapePoly( cBody * body, int numVerts, cVect *verts, cVect offset );

		cShapePoly( cBody * body, cpFloat width, cpFloat height );

		static bool Validate( const cVect * verts, const int numVerts );

		int GetNumVerts();

		cVect GetVert( int idx );

		void SetVerts( int numVerts, cVect *verts, cVect offset );

		virtual void Draw( cSpace * space );

		static void Recenter( int numVerts, cVect * verts );

		static cVect Centroid( int numVerts, const cVect * verts );
};

}}

#endif
