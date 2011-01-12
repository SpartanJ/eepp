#ifndef EE_PHYSICS_CCONSTRAINT_HPP
#define EE_PHYSICS_CCONSTRAINT_HPP

#include "../base.hpp"
#include "../cbody.hpp"

namespace EE { namespace Physics {

class cConstraint {
	public:
		cConstraint( cpConstraint * Constraint );

		cConstraint();

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

		void SetData();
};

}}

#endif
