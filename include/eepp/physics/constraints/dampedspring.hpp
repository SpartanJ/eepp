#ifndef EE_PHYSICS_CDAMPEDSPRING_HPP
#define EE_PHYSICS_CDAMPEDSPRING_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API DampedSpring : public Constraint {
	public:
		DampedSpring( Body * a, Body * b, cVect anchr1, cVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping );

		cVect Anchr1();

		void Anchr1( const cVect& anchr1 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		cpFloat RestLength();

		void RestLength( const cpFloat& restlength );

		cpFloat Stiffness();

		void Stiffness( const cpFloat& stiffness );

		cpFloat Damping();

		void Damping( const cpFloat& damping );

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
