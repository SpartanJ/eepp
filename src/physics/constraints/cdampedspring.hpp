#ifndef EE_PHYSICS_CDAMPEDSPRING_HPP
#define EE_PHYSICS_CDAMPEDSPRING_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cDampedSpring : public cConstraint {
	public:
		cDampedSpring( cBody * a, cBody * b, cpVect anchr1, cpVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping );

		cpVect Anchr1();

		void Anchr1( const cpVect& anchr1 );

		cpVect Anchr2();

		void Anchr2( const cpVect& anchr2 );

		cpFloat RestLength();

		void RestLength( const cpFloat& restlength );

		cpFloat Stiffness();

		void Stiffness( const cpFloat& stiffness );

		cpFloat Damping();

		void Damping( const cpFloat& damping );

		virtual void Draw();
};

}}

#endif
