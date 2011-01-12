#ifndef EE_PHYSICS_CRATCHETJOINT_HPP
#define EE_PHYSICS_CRATCHETJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cRatchetJoint : public cConstraint {
	public:
		cRatchetJoint( cBody * a, cBody * b, cpFloat phase, cpFloat ratchet );

		cpFloat Angle();

		void Angle( const cpFloat& angle );

		cpFloat Phase();

		void Phase( const cpFloat& phase );

		cpFloat Ratchet();

		void Ratchet( const cpFloat& ratchet );

		virtual void Draw();
};

}}

#endif
