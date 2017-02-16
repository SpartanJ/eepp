#ifndef EE_PHYSICS_CGROOVEJOINT_HPP
#define EE_PHYSICS_CGROOVEJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API GrooveJoint : public Constraint {
	public:
		GrooveJoint( Body * a, Body * b, cVect groove_a, cVect groove_b, cVect anchr2 );

		cVect anchr2();

		void anchr2( const cVect& anchr2 );

		cVect grooveA();

		void grooveA( const cVect& groove_a );

		cVect grooveB();

		void grooveB( const cVect& groove_b );

		virtual void draw();

#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat drawPointSize();

		virtual void drawPointSize( const cpFloat& size );
	protected:
		cpFloat mDrawPointSize;
#endif
};

CP_NAMESPACE_END

#endif
