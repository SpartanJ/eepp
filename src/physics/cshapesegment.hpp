#ifndef EE_PHYSICS_CSHAPESEGMENT_HPP
#define EE_PHYSICS_CSHAPESEGMENT_HPP

#include "cshape.hpp"

namespace EE { namespace Physics {

class cShapeSegment : public cShape {
	public:
		cShapeSegment( cBody * body, cpVect a, cpVect b, cpFloat radius );

		cpVect A();

		cpVect B();

		cpVect Normal();

		cpFloat Radius();

		void Radius( const cpFloat& radius );

		void Endpoints( const cpVect& a, const cpVect& b );

		bool Query( cpVect a, cpVect b, cpSegmentQueryInfo * info );

		static cpVect HitPoint( const cpVect start, const cpVect end, const cpSegmentQueryInfo info );

		static cpFloat HitDist( const cpVect start, const cpVect end, const cpSegmentQueryInfo info );

		virtual void Draw( cSpace * space );
};

}}

#endif
