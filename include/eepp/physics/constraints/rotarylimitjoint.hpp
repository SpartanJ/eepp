#ifndef EE_PHYSICS_CROTARYLIMITJOINT_HPP
#define EE_PHYSICS_CROTARYLIMITJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API RotaryLimitJoint : public Constraint {
	public:
		RotaryLimitJoint( Body * a, Body * b, cpFloat min, cpFloat max );

		cpFloat Min();

		void Min( const cpFloat& min );

		cpFloat Max();

		void Max( const cpFloat& max );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
