#ifndef EE_PHYSICS_CDAMPEDROTARYSPRING_HPP
#define EE_PHYSICS_CDAMPEDROTARYSPRING_HPP

#include <eepp/physics/constraints/cconstraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API cDampedRotarySpring : public cConstraint {
	public:
		cDampedRotarySpring( cBody * a, cBody * b, cpFloat restAngle, cpFloat stiffness, cpFloat damping );

		cpFloat RestAngle();

		void RestAngle( const cpFloat& restangle );

		cpFloat Stiffness();

		void Stiffness( const cpFloat& stiffness );

		cpFloat Damping();

		void Damping( const cpFloat& damping );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
