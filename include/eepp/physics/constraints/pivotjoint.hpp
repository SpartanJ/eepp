#ifndef EE_PHYSICS_CPIVOTJOINT_HPP
#define EE_PHYSICS_CPIVOTJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API PivotJoint : public Constraint {
	public:
		PivotJoint( Body * a, Body * b, cVect pivot );

		PivotJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 );

		cVect anchr1();

		void anchr1( const cVect& anchr1 );

		cVect anchr2();

		void anchr2( const cVect& anchr2 );

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
