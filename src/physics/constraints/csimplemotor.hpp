#ifndef EE_PHYSICS_CSIMPLEMOTOR_HPP
#define EE_PHYSICS_CSIMPLEMOTOR_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cSimpleMotor : public cConstraint {
	public:
		cSimpleMotor( cBody * a, cBody * b, cpFloat rate );

		cpFloat Rate();

		void Rate( const cpFloat& rate );

		virtual void Draw();
};

}}

#endif
