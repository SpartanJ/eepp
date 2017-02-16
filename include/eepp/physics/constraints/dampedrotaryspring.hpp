#ifndef EE_PHYSICS_CDAMPEDROTARYSPRING_HPP
#define EE_PHYSICS_CDAMPEDROTARYSPRING_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API DampedRotarySpring : public Constraint {
	public:
		DampedRotarySpring( Body * a, Body * b, cpFloat restAngle, cpFloat stiffness, cpFloat damping );

		cpFloat restAngle();

		void restAngle( const cpFloat& restangle );

		cpFloat stiffness();

		void stiffness( const cpFloat& stiffness );

		cpFloat damping();

		void damping( const cpFloat& damping );

		virtual void draw();
};

CP_NAMESPACE_END

#endif
