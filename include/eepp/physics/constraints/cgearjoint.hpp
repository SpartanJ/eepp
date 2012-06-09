#ifndef EE_PHYSICS_CGEARJOINT_HPP
#define EE_PHYSICS_CGEARJOINT_HPP

#include <eepp/physics/constraints/cconstraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API cGearJoint : public cConstraint {
	public:
		cGearJoint( cBody * a, cBody * b, cpFloat phase, cpFloat ratio );

		cpFloat Phase();

		void Phase( const cpFloat& phase );

		cpFloat Ratio();

		void Ratio( const cpFloat& ratio );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
