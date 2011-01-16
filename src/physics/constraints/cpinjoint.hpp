#ifndef EE_PHYSICS_CPINJOINT_HPP
#define EE_PHYSICS_CPINJOINT_HPP

#include "cconstraint.hpp"

CP_NAMESPACE_BEGIN

class CP_API cPinJoint : public cConstraint {
	public:
		cPinJoint( cBody * a, cBody * b, cVect anchr1, cVect anchr2 );

		cVect Anchr1();

		void Anchr1( const cVect& anchr1 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		cpFloat Dist();

		void Dist( const cpFloat& dist );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
