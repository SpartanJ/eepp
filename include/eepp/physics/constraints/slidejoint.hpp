#ifndef EE_PHYSICS_CSLIDEJOINT_HPP
#define EE_PHYSICS_CSLIDEJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API SlideJoint : public Constraint {
	public:
		SlideJoint( Body * a, Body *b, cVect anchr1, cVect anchr2, cpFloat min, cpFloat max );

		cVect getAnchr1();

		void setAnchr1( const cVect& anchr1 );

		cVect getAnchr2();

		void setAnchr2( const cVect& anchr2 );

		cpFloat getMin();

		void setMin( const cpFloat& min );

		cpFloat getMax();

		void setMax( const cpFloat& max );

		virtual void draw();

#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat getDrawPointSize();

		virtual void setDrawPointSize( const cpFloat& size );
	protected:
		cpFloat mDrawPointSize;
#endif
};

CP_NAMESPACE_END

#endif
