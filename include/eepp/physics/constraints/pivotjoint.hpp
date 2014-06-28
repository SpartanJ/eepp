#ifndef EE_PHYSICS_CPIVOTJOINT_HPP
#define EE_PHYSICS_CPIVOTJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API PivotJoint : public Constraint {
	public:
		PivotJoint( Body * a, Body * b, cVect pivot );

		PivotJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 );

		cVect Anchr1();

		void Anchr1( const cVect& anchr1 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		virtual void Draw();

#ifdef PHYSICS_RENDERER_ENABLED
		cpFloat DrawPointSize();

		virtual void DrawPointSize( const cpFloat& size );
	protected:
		cpFloat mDrawPointSize;
#endif
};

CP_NAMESPACE_END

#endif
