#ifndef EE_PHYSICS_CSIMPLEMOTOR_HPP
#define EE_PHYSICS_CSIMPLEMOTOR_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API SimpleMotor : public Constraint {
	public:
		SimpleMotor( Body * a, Body * b, cpFloat rate );

		cpFloat Rate();

		void Rate( const cpFloat& rate );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
