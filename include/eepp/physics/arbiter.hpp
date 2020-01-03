#ifndef EE_PHYSICS_CARBITER_HPP
#define EE_PHYSICS_CARBITER_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/shape.hpp>
#include <eepp/physics/body.hpp>

namespace EE { namespace Physics {

class EE_API Arbiter {
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

		cpContactPointSet getContactPointSet();

		void setContactPointSet( cpContactPointSet * contact );

		cpArbiter *	getArbiter() const;

		cpFloat getElasticity();

		void setElasticity( cpFloat value );

		cpFloat getFriction();

		void setFriction( cpFloat value );

		cVect getSurfaceVelocity();

		void setSurfaceVelocity( cVect value );

		void setUserData( cpDataPointer value );

		cpDataPointer getUserData() const;
	protected:
		cpArbiter *		mArbiter;
};

}}

#endif
