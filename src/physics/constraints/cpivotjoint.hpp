#ifndef EE_PHYSICS_CPIVOTJOINT_HPP
#define EE_PHYSICS_CPIVOTJOINT_HPP

#include "cconstraint.hpp"

CP_NAMESPACE_BEGIN

class CP_API cPivotJoint : public cConstraint {
	public:
		cPivotJoint( cBody * a, cBody * b, cVect pivot );

		cPivotJoint( cBody * a, cBody * b, cVect anchr1, cVect anchr2 );

		cVect Anchr1();

		void Anchr1( const cVect& anchr1 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
