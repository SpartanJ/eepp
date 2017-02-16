#ifndef EE_PHYSICS_CCONSTRAINT_HPP
#define EE_PHYSICS_CCONSTRAINT_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/body.hpp>

CP_NAMESPACE_BEGIN

class CP_API Constraint {
	public:
		static void Free( Constraint * constraint );

		Constraint( cpConstraint * Constraint );

		virtual ~Constraint();

		cpConstraint * getConstraint() const;

		Body * a();

		Body * b();

		cpFloat maxForce();

		void maxForce( const cpFloat& maxforce );

		cpFloat maxBias();

		void maxBias( const cpFloat& maxbias );

		virtual void draw();

		cpFloat errorBias();

		void errorBias( cpFloat value );

		void data( void * data );

		void * data() const;

		cpFloat impulse();
	protected:
		cpConstraint *		mConstraint;

		void *				mData;

		Constraint();

		void setData();
};

CP_NAMESPACE_END

#endif
