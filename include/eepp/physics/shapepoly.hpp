#ifndef EE_PHYSICS_CSHAPEPOLY_HPP
#define EE_PHYSICS_CSHAPEPOLY_HPP

#include <eepp/physics/shape.hpp>

CP_NAMESPACE_BEGIN

class CP_API ShapePoly : public Shape {
	public:
		static ShapePoly * New( Physics::Body * body, int numVerts, cVect *verts, cVect offset );

		static ShapePoly * New( Physics::Body * body, cpFloat width, cpFloat height );

		ShapePoly( Physics::Body * body, int numVerts, cVect *verts, cVect offset );

		ShapePoly( Physics::Body * body, cpFloat width, cpFloat height );

		static bool Validate( const cVect * verts, const int numVerts );

		int GetNumVerts();

		cVect GetVert( int idx );

		void SetVerts( int numVerts, cVect *verts, cVect offset );

		virtual void Draw( Space * space );

		virtual void DrawBorder( Space * space );

		static void Recenter( int numVerts, cVect * verts );

		static cVect Centroid( int numVerts, const cVect * verts );
};

CP_NAMESPACE_END

#endif
