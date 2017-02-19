#ifndef EE_PHYSICS_CDAMPEDSPRING_HPP
#define EE_PHYSICS_CDAMPEDSPRING_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API DampedSpring : public Constraint {
	public:
		DampedSpring( Body * a, Body * b, cVect anchr1, cVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping );

		cVect getAnchr1();

		void setAnchr1( const cVect& anchr1 );

		cVect getAnchr2();

		void setAnchr2( const cVect& anchr2 );

		cpFloat getRestLength();

		void setRestLength( const cpFloat& restlength );

		cpFloat getStiffness();

		void setStiffness( const cpFloat& stiffness );

		cpFloat getDamping();

		void setDamping( const cpFloat& damping );

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
