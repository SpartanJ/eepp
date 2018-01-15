#ifndef EE_PHYSICS_CROTARYLIMITJOINT_HPP
#define EE_PHYSICS_CROTARYLIMITJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API RotaryLimitJoint : public Constraint {
	public:
		RotaryLimitJoint( Body * a, Body * b, cpFloat min, cpFloat max );

		cpFloat getMin();

		void setMin( const cpFloat& min );

		cpFloat getMax();

		void setMax( const cpFloat& max );

		virtual void draw();
};

CP_NAMESPACE_END

#endif
