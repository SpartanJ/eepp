#ifndef EE_PHYSICS_CARBITER_HPP
#define EE_PHYSICS_CARBITER_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/shape.hpp>
#include <eepp/physics/body.hpp>

CP_NAMESPACE_BEGIN

class CP_API Arbiter {
	public:
		Arbiter( cpArbiter * arbiter );

		cVect TotalImpulse();

		cVect TotalImpulseWithFriction();

		void Ignore();

		void GetShapes( Shape ** a, Shape ** b );

		void GetBodies( Body ** a, Body ** b);

		bool IsFirstContact();

		int GetCount();

		cVect GetNormal( int i );

		cVect GetPoint( int i );

		cpFloat GetDepth( int i );

		cpContactPointSet ContactPointSet();

		void ContactPointSet( cpContactPointSet * contact );

		cpArbiter *	GetArbiter() const;

		cpFloat Elasticity();

		void Elasticity( cpFloat value );

		cpFloat Friction();

		void Friction( cpFloat value );

		cVect SurfaceVelocity();

		void SurfaceVelocity( cVect value );

		void UserData( cpDataPointer value );

		cpDataPointer UserData() const;
	protected:
		cpArbiter *		mArbiter;
};

CP_NAMESPACE_END

#endif
