#ifndef EE_PHYSICS_CRATCHETJOINT_HPP
#define EE_PHYSICS_CRATCHETJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API RatchetJoint : public Constraint {
	public:
		RatchetJoint( Body * a, Body * b, cpFloat phase, cpFloat ratchet );

		cpFloat angle();

		void angle( const cpFloat& angle );

		cpFloat phase();

		void phase( const cpFloat& phase );

		cpFloat ratchet();

		void ratchet( const cpFloat& ratchet );

		virtual void draw();
};

CP_NAMESPACE_END

#endif
