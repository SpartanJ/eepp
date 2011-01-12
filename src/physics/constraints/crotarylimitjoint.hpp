#ifndef EE_PHYSICS_CROTARYLIMITJOINT_HPP
#define EE_PHYSICS_CROTARYLIMITJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cRotaryLimitJoint : public cConstraint {
	public:
		cRotaryLimitJoint( cBody * a, cBody * b, cpFloat min, cpFloat max );

		cpFloat Min();

		void Min( const cpFloat& min );

		cpFloat Max();

		void Max( const cpFloat& max );

		virtual void Draw();
};

}}

#endif
