#ifndef EE_PHYSICS_CPINJOINT_HPP
#define EE_PHYSICS_CPINJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

namespace EE { namespace Physics {

class EE_API PinJoint : public Constraint {
	public:
		PinJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 );

		cVect getAnchr1();

		void setAnchr1( const cVect& anchr1 );

		cVect getAnchr2();

		void setAnchr2( const cVect& anchr2 );

		cpFloat getDist();

		void setDist( const cpFloat& dist );

		virtual void draw();

#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat getDrawPointSize();

		virtual void setDrawPointSize( const cpFloat& size );
	protected:
		cpFloat mDrawPointSize;
#endif
};

}}

#endif
