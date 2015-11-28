#ifndef EE_PHYSICS_CPINJOINT_HPP
#define EE_PHYSICS_CPINJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

CP_NAMESPACE_BEGIN

class CP_API PinJoint : public Constraint {
	public:
		PinJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 );

		cVect Anchr1();

		void Anchr1( const cVect& anchr1 );

		cVect Anchr2();

		void Anchr2( const cVect& anchr2 );

		cpFloat Dist();

		void Dist( const cpFloat& dist );

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
