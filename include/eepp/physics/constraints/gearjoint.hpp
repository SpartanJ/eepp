#ifndef EE_PHYSICS_CGEARJOINT_HPP
#define EE_PHYSICS_CGEARJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

namespace EE { namespace Physics {

class EE_API GearJoint : public Constraint {
	public:
		GearJoint( Body * a, Body * b, cpFloat phase, cpFloat ratio );

		cpFloat getPhase();

		void setPhase( const cpFloat& phase );

		cpFloat getRatio();

		void setRatio( const cpFloat& ratio );

		virtual void draw();
};

}}

#endif
