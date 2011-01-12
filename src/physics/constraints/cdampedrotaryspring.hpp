#ifndef EE_PHYSICS_CDAMPEDROTARYSPRING_HPP
#define EE_PHYSICS_CDAMPEDROTARYSPRING_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cDampedRotarySpring : public cConstraint {
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

}}

#endif
