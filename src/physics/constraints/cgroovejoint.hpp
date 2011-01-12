#ifndef EE_PHYSICS_CGROOVEJOINT_HPP
#define EE_PHYSICS_CGROOVEJOINT_HPP

#include "cconstraint.hpp"

namespace EE { namespace Physics {

class cGrooveJoint : public cConstraint {
	public:
		cGrooveJoint( cBody * a, cBody * b, cpVect groove_a, cpVect groove_b, cpVect anchr2 );

		cpVect Anchr2();

		void Anchr2( const cpVect& anchr2 );

		cpVect GrooveA();

		void GrooveA( const cpVect& groove_a );

		cpVect GrooveB();

		void GrooveB( const cpVect& groove_b );

		virtual void Draw();
};

}}

#endif
