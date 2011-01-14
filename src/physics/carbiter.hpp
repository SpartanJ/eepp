#ifndef EE_PHYSICS_CARBITER_HPP
#define EE_PHYSICS_CARBITER_HPP

#include "base.hpp"
#include "cshape.hpp"
#include "cbody.hpp"

namespace EE { namespace Physics {

class cArbiter {
	public:
		cArbiter( cpArbiter * arbiter );

		cVect TotalImpulse();

		cVect TotalImpulseWithFriction();

		void Ignore();

		void GetShapes( cShape ** a, cShape ** b );

		void GetBodies( cBody ** a, cBody ** b);

		bool IsFirstContact();

		int GetCount();

		cVect GetNormal( int i );

		cVect GetPoint( int i );

		cpFloat GetDepth( int i );

		cpContactPointSet GetContactPointSet();

		cpArbiter *	Arbiter() const;
	protected:
		cpArbiter *		mArbiter;
};

}}

#endif
