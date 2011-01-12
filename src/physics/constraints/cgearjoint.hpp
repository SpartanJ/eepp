#ifndef EE_PHYSICS_CGEARJOINT_HPP
#define EE_PHYSICS_CGEARJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cGearJoint : public cConstraint {
	public:
		cGearJoint( cBody * a, cBody * b, cpFloat phase, cpFloat ratio );

		cpFloat Phase();

		void Phase( const cpFloat& phase );

		cpFloat Ratio();

		void Ratio( const cpFloat& ratio );

		virtual void Draw();
};

}}

#endif
