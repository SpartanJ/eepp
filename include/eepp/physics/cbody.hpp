#ifndef EE_PHYSICS_CBODY_HPP
#define EE_PHYSICS_CBODY_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class cShape;
class cConstraint;
class cArbiter;

class CP_API cBody {
	public:
		typedef cb::Callback3<void, cBody *, cShape *, void * >			ShapeIteratorFunc;
		typedef cb::Callback3<void, cBody *, cConstraint *, void *>		ConstraintIteratorFunc;
		typedef cb::Callback3<void, cBody *, cArbiter *, void *>		ArbiterIteratorFunc;
		typedef cb::Callback4<void, cBody *, cVect, cpFloat, cpFloat>	BodyVelocityFunc;
		typedef cb::Callback2<void, cBody *, cpFloat>					BodyPositionFunc;

		class cShapeIterator {
			public:
				cShapeIterator( cBody * body, void * data, ShapeIteratorFunc func ) :
					Body( body ),
					Data( data ),
					Func( func )
				{}

				cBody *						Body;
				void *						Data;
				ShapeIteratorFunc			Func;
		};

		class cConstraintIterator {
			public:
				cConstraintIterator( cBody * body, void * data, ConstraintIteratorFunc func ) :
					Body( body ),
					Data( data ),
					Func( func )
				{}

				cBody *						Body;
				void *						Data;
				ConstraintIteratorFunc		Func;
		};

		class cArbiterIterator {
			public:
				cArbiterIterator( cBody * body, void * data, ArbiterIteratorFunc func ) :
					Body( body ),
					Data( data ),
					Func( func )
				{}

				cBody *						Body;
				void *						Data;
				ArbiterIteratorFunc			Func;
		};

		static cBody * New( cpFloat m, cpFloat i );

		static cBody * New( cpBody * body );

		static cBody * New();

		static void Free( cBody * body );

		cBody( cpBody * body );

		cBody( cpFloat m, cpFloat i );

		cBody();

		virtual ~cBody();

		void Activate();

		void ActivateStatic( cBody *body, cShape * filter );

		void Sleep();

		void SleepWithGroup( cBody * Group );

		bool IsSleeping();

		bool IsStatic();

		bool IsRogue();

		cpBody * Body() const;

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

		virtual void OnEachShape( cShape * Shape, cShapeIterator * it );

		void EachConstraint( ConstraintIteratorFunc Func, void * data );

		virtual void OnEachConstraint( cConstraint * Constraint, cConstraintIterator * it );

		void EachArbiter( ArbiterIteratorFunc Func, void * data );

		virtual void OnEachArbiter( cArbiter * Arbiter, cArbiterIterator * it );

		void VelocityFunc( BodyVelocityFunc func );

		void PositionFunc( BodyPositionFunc func );
	protected:
		friend class cSpace;

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
