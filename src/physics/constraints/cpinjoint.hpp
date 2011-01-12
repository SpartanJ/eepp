#ifndef EE_PHYSICS_CPINJOINT_HPP
#define EE_PHYSICS_CPINJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cPinJoint : public cConstraint {
	public:
		cPinJoint( cBody * a, cBody * b, cpVect anchr1, cpVect anchr2 );

		cpVect Anchr1();

		void Anchr1( const cpVect& anchr1 );

		cpVect Anchr2();

		void Anchr2( const cpVect& anchr2 );

		cpFloat Dist();

		void Dist( const cpFloat& dist );

		virtual void Draw();
};

}}

#endif
