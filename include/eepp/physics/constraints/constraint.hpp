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

		cpConstraint * GetConstraint() const;

		Body * A();

		Body * B();

		cpFloat MaxForce();

		void MaxForce( const cpFloat& maxforce );

		cpFloat MaxBias();

		void MaxBias( const cpFloat& maxbias );

		virtual void Draw();

		cpFloat ErrorBias();

		void ErrorBias( cpFloat value );

		void Data( void * data );

		void * Data() const;

		cpFloat Impulse();
	protected:
		cpConstraint *		mConstraint;

		void *				mData;

		Constraint();

		void SetData();
};

CP_NAMESPACE_END

#endif
