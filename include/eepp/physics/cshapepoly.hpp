#ifndef EE_PHYSICS_CSHAPEPOLY_HPP
#define EE_PHYSICS_CSHAPEPOLY_HPP

#include <eepp/physics/cshape.hpp>

CP_NAMESPACE_BEGIN

class CP_API cShapePoly : public cShape {
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

CP_NAMESPACE_END

#endif
