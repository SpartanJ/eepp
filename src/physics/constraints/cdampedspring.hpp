#ifndef EE_PHYSICS_CDAMPEDSPRING_HPP
#define EE_PHYSICS_CDAMPEDSPRING_HPP

#include "cconstraint.hpp"

CP_NAMESPACE_BEGIN

class CP_API cDampedSpring : public cConstraint {
	public:
		cDampedSpring( cBody * a, cBody * b, cVect anchr1, cVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping );

		cVect Anchr1();

		void Anchr1( const cVect& anchr1 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		cpFloat RestLength();

		void RestLength( const cpFloat& restlength );

		cpFloat Stiffness();

		void Stiffness( const cpFloat& stiffness );

		cpFloat Damping();

		void Damping( const cpFloat& damping );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
