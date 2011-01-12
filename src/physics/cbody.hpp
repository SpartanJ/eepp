#ifndef EE_PHYSICS_CBODY_HPP
#define EE_PHYSICS_CBODY_HPP

#include "base.hpp"

namespace EE { namespace Physics {

class cBody {
	public:
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

		cpVect Pos() const;

		void Pos( const cpVect& pos );

		cpVect Vel() const;

		void Vel( const cpVect& vel );

		cpVect Force() const;

		void Force( const cpVect& force );

		cpFloat Angle() const;

		void Angle( const cpFloat& rads );

		cpFloat AngleDeg();

		void AngleDeg( const cpFloat& angle );

		cpFloat AngVel() const;

		void AngVel( const cpFloat& angVel );

		cpVect Rot() const;

		cpFloat VelLimit() const;

		void VelLimit( const cpFloat& speed );

		cpFloat AngVelLimit() const;

		void AngVelLimit( const cpFloat& speed );

		void Slew( cpVect pos, cpFloat dt );

		void UpdateVelocity( cpVect gravity, cpFloat damping, cpFloat dt );

		void UpdatePosition( cpFloat dt );

		cpVect Local2World( const cpVect v );

		cpVect World2Local( const cpVect v );

		void ApplyImpulse( const cpVect j, const cpVect r );

		void ResetForces();

		void ApplyForce( const cpVect f, const cpVect r );

		cpFloat KineticEnergy();
	protected:
		cpBody *		mBody;
};

}}

#endif
