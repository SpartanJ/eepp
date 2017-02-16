#ifndef EE_PHYSICS_CDAMPEDSPRING_HPP
#define EE_PHYSICS_CDAMPEDSPRING_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API DampedSpring : public Constraint {
	public:
		DampedSpring( Body * a, Body * b, cVect anchr1, cVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping );

		cVect anchr1();

		void anchr1( const cVect& anchr1 );

		cVect anchr2();

		void anchr2( const cVect& anchr2 );

		cpFloat restLength();

		void restLength( const cpFloat& restlength );

		cpFloat stiffness();

		void stiffness( const cpFloat& stiffness );

		cpFloat damping();

		void damping( const cpFloat& damping );

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
