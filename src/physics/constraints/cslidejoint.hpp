#ifndef EE_PHYSICS_CSLIDEJOINT_HPP
#define EE_PHYSICS_CSLIDEJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cSlideJoint : public cConstraint {
	public:
		cSlideJoint( cBody * a, cBody *b, cpVect anchr1, cpVect anchr2, cpFloat min, cpFloat max );

		cpVect Anchr1();

		void Anchr1( const cpVect& anchr1 );

		cpVect Anchr2();

		void Anchr2( const cpVect& anchr2 );

		cpFloat Min();

		void Min( const cpFloat& min );

		cpFloat Max();

		void Max( const cpFloat& max );

		virtual void Draw();
};

}}

#endif
