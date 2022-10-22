#ifndef EE_PHYSICS_CCONSTRAINT_HPP
#define EE_PHYSICS_CCONSTRAINT_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/body.hpp>

namespace EE { namespace Physics {

class EE_API Constraint {
  public:
	static void Free( Constraint* constraint );

	Constraint( cpConstraint* Constraint );

	virtual ~Constraint();

	cpConstraint* getConstraint() const;

	Body* getA();

	Body* getB();

	cpFloat getMaxForce();

	void setMaxForce( const cpFloat& maxforce );

	cpFloat getMaxBias();

	void setMaxBias( const cpFloat& maxbias );

	virtual void draw();

	cpFloat getErrorBias();

	void setErrorBias( cpFloat value );

	void setData( void* data );

	void* getData() const;

	cpFloat getImpulse();

  protected:
	cpConstraint* mConstraint;

	void* mData;

	Constraint();

	void setData();
};

}} // namespace EE::Physics

#endif
