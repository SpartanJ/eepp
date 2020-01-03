#ifndef EE_PHYSICS_CSHAPEPOLY_HPP
#define EE_PHYSICS_CSHAPEPOLY_HPP

#include <eepp/physics/shape.hpp>

namespace EE { namespace Physics {

class EE_API ShapePoly : public Shape {
	public:
		static ShapePoly * New( Physics::Body * body, int numVerts, cVect *verts, cVect offset );

		static ShapePoly * New( Physics::Body * body, cpFloat width, cpFloat height );

		ShapePoly( Physics::Body * body, int numVerts, cVect *verts, cVect offset );

		ShapePoly( Physics::Body * body, cpFloat width, cpFloat height );

		static bool validate( const cVect * verts, const int numVerts );

		int getNumVerts();

		cVect getVert( int idx );

		void setVerts( int numVerts, cVect *verts, cVect offset );

		virtual void draw( Space * space );

		virtual void drawBorder( Space * space );

		static void recenter( int numVerts, cVect * verts );

		static cVect centroid( int numVerts, const cVect * verts );
};

}}

#endif
