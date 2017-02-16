#ifndef EE_PHYSICS_CSLIDEJOINT_HPP
#define EE_PHYSICS_CSLIDEJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API SlideJoint : public Constraint {
	public:
		SlideJoint( Body * a, Body *b, cVect anchr1, cVect anchr2, cpFloat min, cpFloat max );

		cVect anchr1();

		void anchr1( const cVect& anchr1 );

		cVect anchr2();

		void anchr2( const cVect& anchr2 );

		cpFloat min();

		void min( const cpFloat& min );

		cpFloat max();

		void max( const cpFloat& max );

		virtual void draw();

#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat drawPointSize();

		virtual void drawPointSize( const cpFloat& size );
	protected:
		cpFloat mDrawPointSize;
#endif
};

CP_NAMESPACE_END

#endif
