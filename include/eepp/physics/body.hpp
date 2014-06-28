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

		void Activate();

		void ActivateStatic( Body *body, Shape * filter );

		void Sleep();

		void SleepWithGroup( Body * Group );

		bool IsSleeping();

		bool IsStatic();

		bool IsRogue();

		cpBody * GetBody() const;

		cpFloat Mass() const;

		void Mass( const cpFloat& mass );

		cpFloat Moment() const;

		void Moment( const cpFloat& i );

		cVect Pos() const;

		void Pos( const cVect& pos );

		cVect Vel() const;

		void Vel( const cVect& vel );

		cVect Force() const;

		void Force( const cVect& force );

		cpFloat Angle() const;

		void Angle( const cpFloat& rads );

		cpFloat AngleDeg();

		void AngleDeg( const cpFloat& angle );

		cpFloat AngVel() const;

		void AngVel( const cpFloat& angVel );

		cpFloat Torque() const;

		void Torque( const cpFloat& torque );

		cVect Rot() const;

		cpFloat VelLimit() const;

		void VelLimit( const cpFloat& speed );

		cpFloat AngVelLimit() const;

		void AngVelLimit( const cpFloat& speed );

		void UpdateVelocity( cVect gravity, cpFloat damping, cpFloat dt );

		void UpdatePosition( cpFloat dt );

		cVect Local2World( const cVect v );

		cVect World2Local( const cVect v );

		void ApplyImpulse( const cVect j, const cVect r );

		void ResetForces();

		void ApplyForce( const cVect f, const cVect r );

		cpFloat KineticEnergy();

		void * Data() const;

		void Data( void * data );

		void EachShape( ShapeIteratorFunc Func, void * data );

		virtual void OnEachShape( Shape * Shape, ShapeIterator * it );

		void EachConstraint( ConstraintIteratorFunc Func, void * data );

		virtual void OnEachConstraint( Constraint * Constraint, ConstraintIterator * it );

		void EachArbiter( ArbiterIteratorFunc Func, void * data );

		virtual void OnEachArbiter( Arbiter * Arbiter, ArbiterIterator * it );

		void VelocityFunc( BodyVelocityFunc func );

		void PositionFunc( BodyPositionFunc func );
	protected:
		friend class Space;

		static void BodyVelocityFuncWrapper( cpBody *body, cpVect gravity, cpFloat damping, cpFloat dt );

		static void BodyPositionFuncWrapper( cpBody* body, cpFloat dt );

		cpBody *				mBody;
		void *					mData;

		BodyVelocityFunc		mVelocityFunc;

		BodyPositionFunc		mPositionFunc;

		void SetData();
};

CP_NAMESPACE_END

#endif
