#ifndef EE_PHYSICS_CRATCHETJOINT_HPP
#define EE_PHYSICS_CRATCHETJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API RatchetJoint : public Constraint {
	public:
		RatchetJoint( Body * a, Body * b, cpFloat phase, cpFloat ratchet );

		cpFloat Angle();

		void Angle( const cpFloat& angle );

		cpFloat Phase();

		void Phase( const cpFloat& phase );

		cpFloat Ratchet();

		void Ratchet( const cpFloat& ratchet );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
