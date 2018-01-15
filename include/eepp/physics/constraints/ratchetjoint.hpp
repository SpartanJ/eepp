#ifndef EE_PHYSICS_CRATCHETJOINT_HPP
#define EE_PHYSICS_CRATCHETJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API RatchetJoint : public Constraint {
	public:
		RatchetJoint( Body * a, Body * b, cpFloat phase, cpFloat ratchet );

		cpFloat getAngle();

		void setAngle( const cpFloat& angle );

		cpFloat getPhase();

		void setPhase( const cpFloat& phase );

		cpFloat getRatchet();

		void setRatchet( const cpFloat& ratchet );

		virtual void draw();
};

CP_NAMESPACE_END

#endif
