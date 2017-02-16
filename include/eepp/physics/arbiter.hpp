#ifndef EE_PHYSICS_CARBITER_HPP
#define EE_PHYSICS_CARBITER_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/shape.hpp>
#include <eepp/physics/body.hpp>

CP_NAMESPACE_BEGIN

class CP_API Arbiter {
	public:
		Arbiter( cpArbiter * arbiter );

		cVect totalImpulse();

		cVect totalImpulseWithFriction();

		void ignore();

		void getShapes( Shape ** a, Shape ** b );

		void getBodies( Body ** a, Body ** b);

		bool isFirstContact();

		int getCount();

		cVect getNormal( int i );

		cVect getPoint( int i );

		cpFloat getDepth( int i );

		cpContactPointSet contactPointSet();

		void contactPointSet( cpContactPointSet * contact );

		cpArbiter *	getArbiter() const;

		cpFloat elasticity();

		void elasticity( cpFloat value );

		cpFloat friction();

		void friction( cpFloat value );

		cVect surfaceVelocity();

		void surfaceVelocity( cVect value );

		void userData( cpDataPointer value );

		cpDataPointer userData() const;
	protected:
		cpArbiter *		mArbiter;
};

CP_NAMESPACE_END

#endif
