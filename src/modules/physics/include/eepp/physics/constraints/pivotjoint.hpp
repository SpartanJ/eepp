#ifndef EE_PHYSICS_CPIVOTJOINT_HPP
#define EE_PHYSICS_CPIVOTJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

namespace EE { namespace Physics {

class EE_PHYSICS_API PivotJoint : public Constraint {
  public:
	PivotJoint( Body* a, Body* b, cVect pivot );

	PivotJoint( Body* a, Body* b, cVect anchr1, cVect anchr2 );

	cVect getAnchr1();

	void setAnchr1( const cVect& anchr1 );

	cVect getAnchr2();

	void setAnchr2( const cVect& anchr2 );

	virtual void draw();

#ifdef PHYSICS_RENDERER_ENABLED
	cpFloat getDrawPointSize();

	virtual void setDrawPointSize( const cpFloat& size );

  protected:
	cpFloat mDrawPointSize;
#endif
};

}} // namespace EE::Physics

#endif
