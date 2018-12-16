#ifndef EE_PHYSICS_CBODY_HPP
#define EE_PHYSICS_CBODY_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class Shape;
class Constraint;
class Arbiter;

class CP_API Body {
	public:
		typedef std::function<void( Body *, Shape *, void * )>			ShapeIteratorFunc;
		typedef std::function<void( Body *, Constraint *, void *)>		ConstraintIteratorFunc;
		typedef std::function<void( Body *, Arbiter *, void *)>		ArbiterIteratorFunc;
		typedef std::function<void( Body *, cVect, cpFloat, cpFloat)>	BodyVelocityFunc;
		typedef std::function<void( Body *, cpFloat )>					BodyPositionFunc;

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

		cpFloat getMass() const;

		void setMass( const cpFloat& mass );

		cpFloat getMoment() const;

		void setMoment( const cpFloat& i );

		cVect getPos() const;

		void setPos( const cVect& pos );

		cVect getVel() const;

		void setVel( const cVect& vel );

		cVect getForce() const;

		void setForce( const cVect& force );

		cpFloat getAngle() const;

		void setAngle( const cpFloat& rads );

		cpFloat getAngleDeg();

		void setAngleDeg( const cpFloat& angle );

		cpFloat getAngVel() const;

		void setAngVel( const cpFloat& angVel );

		cpFloat getTorque() const;

		void setTorque( const cpFloat& torque );

		cVect getRot() const;

		cpFloat getVelLimit() const;

		void setVelLimit( const cpFloat& speed );

		cpFloat getAngVelLimit() const;

		void setAngVelLimit( const cpFloat& speed );

		void updateVelocity( cVect gravity, cpFloat damping, cpFloat dt );

		void updatePosition( cpFloat dt );

		cVect local2World( const cVect v );

		cVect world2Local( const cVect v );

		void applyImpulse( const cVect j, const cVect r );

		void resetForces();

		void applyForce( const cVect f, const cVect r );

		cpFloat kineticEnergy();

		void * getData() const;

		void setData( void * data );

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
