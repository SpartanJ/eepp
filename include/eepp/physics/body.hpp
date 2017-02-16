#ifndef EE_PHYSICS_CBODY_HPP
#define EE_PHYSICS_CBODY_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class Shape;
class Constraint;
class Arbiter;

class CP_API Body {
	public:
		typedef cb::Callback3<void, Body *, Shape *, void * >			ShapeIteratorFunc;
		typedef cb::Callback3<void, Body *, Constraint *, void *>		ConstraintIteratorFunc;
		typedef cb::Callback3<void, Body *, Arbiter *, void *>		ArbiterIteratorFunc;
		typedef cb::Callback4<void, Body *, cVect, cpFloat, cpFloat>	BodyVelocityFunc;
		typedef cb::Callback2<void, Body *, cpFloat>					BodyPositionFunc;

		class ShapeIterator {
			public:
				ShapeIterator( Physics::Body * body, void * data, ShapeIteratorFunc func ) :
					Body( body ),
					Data( data ),
					Func( func )
				{}

				Physics::Body *				Body;
				void *						Data;
				ShapeIteratorFunc			Func;
		};

		class ConstraintIterator {
			public:
				ConstraintIterator( Physics::Body * body, void * data, ConstraintIteratorFunc func ) :
					Body( body ),
					Data( data ),
					Func( func )
				{}

				Physics::Body *				Body;
				void *						Data;
				ConstraintIteratorFunc		Func;
		};

		class ArbiterIterator {
			public:
				ArbiterIterator( Physics::Body * body, void * data, ArbiterIteratorFunc func ) :
					Body( body ),
					Data( data ),
					Func( func )
				{}

				Physics::Body *				Body;
				void *						Data;
				ArbiterIteratorFunc			Func;
		};

		static Body * New( cpFloat m, cpFloat i );

		static Body * New( cpBody * body );

		static Body * New();

		static void Free( Body * body );

		Body( cpBody * body );

		Body( cpFloat m, cpFloat i );

		Body();

		virtual ~Body();

		void activate();

		void activateStatic( Body *body, Shape * filter );

		void sleep();

		void sleepWithGroup( Body * Group );

		bool isSleeping();

		bool isStatic();

		bool isRogue();

		cpBody * getBody() const;

		cpFloat mass() const;

		void mass( const cpFloat& mass );

		cpFloat moment() const;

		void moment( const cpFloat& i );

		cVect pos() const;

		void pos( const cVect& pos );

		cVect vel() const;

		void vel( const cVect& vel );

		cVect force() const;

		void force( const cVect& force );

		cpFloat angle() const;

		void angle( const cpFloat& rads );

		cpFloat angleDeg();

		void angleDeg( const cpFloat& angle );

		cpFloat angVel() const;

		void angVel( const cpFloat& angVel );

		cpFloat torque() const;

		void torque( const cpFloat& torque );

		cVect rot() const;

		cpFloat velLimit() const;

		void velLimit( const cpFloat& speed );

		cpFloat angVelLimit() const;

		void angVelLimit( const cpFloat& speed );

		void updateVelocity( cVect gravity, cpFloat damping, cpFloat dt );

		void updatePosition( cpFloat dt );

		cVect local2World( const cVect v );

		cVect world2Local( const cVect v );

		void applyImpulse( const cVect j, const cVect r );

		void resetForces();

		void applyForce( const cVect f, const cVect r );

		cpFloat kineticEnergy();

		void * data() const;

		void data( void * data );

		void eachShape( ShapeIteratorFunc Func, void * data );

		virtual void onEachShape( Shape * Shape, ShapeIterator * it );

		void eachConstraint( ConstraintIteratorFunc Func, void * data );

		virtual void onEachConstraint( Constraint * Constraint, ConstraintIterator * it );

		void eachArbiter( ArbiterIteratorFunc Func, void * data );

		virtual void onEachArbiter( Arbiter * Arbiter, ArbiterIterator * it );

		void velocityFunc( BodyVelocityFunc func );

		void positionFunc( BodyPositionFunc func );
	protected:
		friend class Space;

		static void bodyVelocityFuncWrapper( cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt );

		static void bodyPositionFuncWrapper( cpBody* body, cpFloat dt );

		cpBody *				mBody;
		void *					mData;

		BodyVelocityFunc		mVelocityFunc;

		BodyPositionFunc		mPositionFunc;

		void setData();
};

CP_NAMESPACE_END

#endif
