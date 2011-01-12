#ifndef EE_PHYSICS_CPIVOTJOINT_HPP
#define EE_PHYSICS_CPIVOTJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cPivotJoint : public cConstraint {
	public:
		cPivotJoint( cBody * a, cBody * b, cpVect pivot );

		cPivotJoint( cBody * a, cBody * b, cpVect anchr1, cpVect anchr2 );

		cpVect Anchr1();

		void Anchr1( const cpVect& anchr1 );

		cpVect Anchr2();

		void Anchr2( const cpVect& anchr2 );

		virtual void Draw();
};

}}

#endif
