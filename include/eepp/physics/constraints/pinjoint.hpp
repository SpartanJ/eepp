#ifndef EE_PHYSICS_CPINJOINT_HPP
#define EE_PHYSICS_CPINJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API PinJoint : public Constraint {
	public:
		PinJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 );

		cVect anchr1();

		void anchr1( const cVect& anchr1 );

		cVect anchr2();

		void anchr2( const cVect& anchr2 );

		cpFloat dist();

		void dist( const cpFloat& dist );

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
