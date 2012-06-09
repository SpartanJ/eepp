#ifndef EE_PHYSICS_CROTARYLIMITJOINT_HPP
#define EE_PHYSICS_CROTARYLIMITJOINT_HPP

#include <eepp/physics/constraints/cconstraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API cRotaryLimitJoint : public cConstraint {
	public:
		cRotaryLimitJoint( cBody * a, cBody * b, cpFloat min, cpFloat max );

		cpFloat Min();

		void Min( const cpFloat& min );

		cpFloat Max();

		void Max( const cpFloat& max );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
