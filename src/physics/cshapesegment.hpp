#ifndef EE_PHYSICS_CSHAPESEGMENT_HPP
#define EE_PHYSICS_CSHAPESEGMENT_HPP

#include "cshape.hpp"

namespace EE { namespace Physics {

class cShapeSegment : public cShape {
	public:
		static cShapeSegment * New( cBody * body, cVect a, cVect b, cpFloat radius );

		cShapeSegment( cBody * body, cVect a, cVect b, cpFloat radius );

		cVect A();

		cVect B();

		cVect Normal();

		cpFloat Radius();

		void Radius( const cpFloat& radius );

		void Endpoints( const cVect& a, const cVect& b );

		bool Query( cVect a, cVect b, cpSegmentQueryInfo * info );

		static cVect QueryHitPoint( const cVect start, const cVect end, const cpSegmentQueryInfo info );

		static cpFloat QueryHitDist( const cVect start, const cVect end, const cpSegmentQueryInfo info );

		virtual void Draw( cSpace * space );
};

}}

#endif
