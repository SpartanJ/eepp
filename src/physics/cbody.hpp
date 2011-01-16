#ifndef EE_PHYSICS_CBODY_HPP
#define EE_PHYSICS_CBODY_HPP

#include "base.hpp"

CP_NAMESPACE_BEGIN

class CP_API cBody {
	public:
		static cBody * New( cpFloat m, cpFloat i );

		static cBody * New( cpBody * body );

		static cBody * New();

		static void Free( cBody * body );

		cBody( cpBody * body );

		cBody( cpFloat m, cpFloat i );

		cBody();

		~cBody();

		void Activate();

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

		cVect Rot() const;

		cpFloat VelLimit() const;

		void VelLimit( const cpFloat& speed );

		cpFloat AngVelLimit() const;

		void AngVelLimit( const cpFloat& speed );

		void Slew( cVect pos, cpFloat dt );

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
	protected:
		friend class cSpace;

		cpBody *		mBody;
		void *			mData;

		void SetData();
};

CP_NAMESPACE_END

#endif
