#ifndef EE_PHYSICS_CGROOVEJOINT_HPP
#define EE_PHYSICS_CGROOVEJOINT_HPP

#include <eepp/physics/constraints/cconstraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API cGrooveJoint : public cConstraint {
	public:
		cGrooveJoint( cBody * a, cBody * b, cVect groove_a, cVect groove_b, cVect anchr2 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		cVect GrooveA();

		void GrooveA( const cVect& groove_a );

		cVect GrooveB();

		void GrooveB( const cVect& groove_b );

		virtual void Draw();
};

CP_NAMESPACE_END

#endif
