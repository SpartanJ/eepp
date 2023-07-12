#ifndef EE_PHYSICS_CGROOVEJOINT_HPP
#define EE_PHYSICS_CGROOVEJOINT_HPP

#include <eepp/physics/constraints/constraint.hpp>

namespace EE { namespace Physics {

class EE_PHYSICS_API GrooveJoint : public Constraint {
  public:
	GrooveJoint( Body* a, Body* b, cVect groove_a, cVect groove_b, cVect anchr2 );

	cVect getAnchr2();

	void setAnchr2( const cVect& anchr2 );

	cVect getGrooveA();

	void setGrooveA( const cVect& groove_a );

	cVect getGrooveB();

	void setGrooveB( const cVect& groove_b );

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
