#ifndef EE_PHYSICS_CCONSTRAINT_HPP
#define EE_PHYSICS_CCONSTRAINT_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/cbody.hpp>

CP_NAMESPACE_BEGIN

class CP_API cConstraint {
	public:
		static void Free( cConstraint * constraint );

		cConstraint( cpConstraint * Constraint );

		virtual ~cConstraint();

		cpConstraint * Constraint() const;

		cBody * A();

		cBody * B();

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

		cConstraint();

		void SetData();
};

CP_NAMESPACE_END

#endif
