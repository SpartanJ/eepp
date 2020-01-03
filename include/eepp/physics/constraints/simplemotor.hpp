#ifndef EE_PHYSICS_CSIMPLEMOTOR_HPP
#define EE_PHYSICS_CSIMPLEMOTOR_HPP

#include <eepp/physics/constraints/constraint.hpp>

namespace EE { namespace Physics {

class EE_API SimpleMotor : public Constraint {
	public:
		SimpleMotor( Body * a, Body * b, cpFloat rate );

		cpFloat getRate();

		void setRate( const cpFloat& rate );

		virtual void draw();
};

}}

#endif
