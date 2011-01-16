#ifndef EE_PHYSICS_CCONSTRAINT_HPP
#define EE_PHYSICS_CCONSTRAINT_HPP

#include "../base.hpp"
#include "../cbody.hpp"

CP_NAMESPACE_BEGIN

class CP_API cConstraint {
	public:
		static void Free( cConstraint * constraint );

		cConstraint( cpConstraint * Constraint );

		~cConstraint();

		cpConstraint * Constraint() const;

		cBody * A();

		cBody * B();

		cpFloat MaxForce();

		void MaxForce( const cpFloat& maxforce );

		cpFloat BiasCoef();

		void BiasCoef( const cpFloat& biascoef );

		cpFloat MaxBias();

		void MaxBias( const cpFloat& maxbias );

		virtual void Draw();
	protected:
		cpConstraint *		mConstraint;

		cConstraint();

		void SetData();
};

CP_NAMESPACE_END

#endif
