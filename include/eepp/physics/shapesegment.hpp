#ifndef EE_PHYSICS_CSHAPESEGMENT_HPP
#define EE_PHYSICS_CSHAPESEGMENT_HPP

#include <eepp/physics/shape.hpp>

CP_NAMESPACE_BEGIN

class CP_API ShapeSegment : public Shape {
	public:
		static ShapeSegment * New( Physics::Body * body, cVect a, cVect b, cpFloat radius );

		ShapeSegment( Physics::Body * body, cVect a, cVect b, cpFloat radius );

		cVect A();

		cVect B();

		cVect Normal();

		cpFloat Radius();

		void Radius( const cpFloat& radius );

		void Endpoints( const cVect& a, const cVect& b );

		bool Query( cVect a, cVect b, cpSegmentQueryInfo * info );

		static cVect QueryHitPoint( const cVect start, const cVect end, const cpSegmentQueryInfo info );

		static cpFloat QueryHitDist( const cVect start, const cVect end, const cpSegmentQueryInfo info );

		virtual void Draw( Space * space );
};

CP_NAMESPACE_END

#endif
