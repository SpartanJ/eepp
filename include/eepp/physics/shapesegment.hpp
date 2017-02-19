#ifndef EE_PHYSICS_CSHAPESEGMENT_HPP
#define EE_PHYSICS_CSHAPESEGMENT_HPP

#include <eepp/physics/shape.hpp>

CP_NAMESPACE_BEGIN

class CP_API ShapeSegment : public Shape {
	public:
		static ShapeSegment * New( Physics::Body * body, cVect a, cVect b, cpFloat radius );

		ShapeSegment( Physics::Body * body, cVect a, cVect b, cpFloat radius );

		cVect getA();

		cVect getB();

		cVect getNormal();

		cpFloat getRadius();

		void setRadius( const cpFloat& radius );

		void setEndpoints( const cVect& a, const cVect& b );

		bool query( cVect a, cVect b, cpSegmentQueryInfo * info );

		static cVect queryHitPoint( const cVect start, const cVect end, const cpSegmentQueryInfo info );

		static cpFloat queryHitDist( const cVect start, const cVect end, const cpSegmentQueryInfo info );

		virtual void draw( Space * space );
};

CP_NAMESPACE_END

#endif
