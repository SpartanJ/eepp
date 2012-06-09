#ifndef EE_PHYSICS_CSIMPLEMOTOR_HPP
#define EE_PHYSICS_CSIMPLEMOTOR_HPP

#include <eepp/physics/constraints/cconstraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API cSimpleMotor : public cConstraint {
	public:
		cSimpleMotor( cBody * a, cBody * b, cpFloat rate );

		cpFloat Rate();

		void Rate( const cpFloat& rate );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
