#ifndef EE_MATHCWAYPOINTS_H
#define EE_MATHCWAYPOINTS_H

#include <eepp/math/base.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/ctime.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace Math {

/** @brief The basic waypoint class. */
template <typename T>
class Waypoint {
	public:
		Waypoint() { p = Vector2<T>(0,0); t = 0; }
		Waypoint( const Vector2<T>& Pos, const eeFloat& Time ) { p = Pos; t = Time; }
		Vector2<T> p;
		eeFloat t;
};
typedef Waypoint<eeFloat> cWaypoint;

/** @brief A waypoint manager, used for movement interpolations. */
class EE_API cWaypoints {
	public:
		cWaypoints();

		~cWaypoints();

		typedef cb::Callback0<void> OnPathEndCallback;

		typedef cb::Callback0<void> OnStepCallback;

		/** Add a new waypoint */
		void AddWaypoint( const eeVector2f& Pos, const eeFloat& Time = 0.f );

		/** Edit a waypoint */
		bool EditWaypoint( const eeUint& PointNum, const eeVector2f& NewPos, const eeFloat& NewTime );

		/** Erase a waypoint */
		bool EraseWaypoint( const eeUint& PointNum );

		/** Start the animation ( will reset the current state, and start from the beginning )
		*	@param PathEndCallback An optional callback fired when the animation ends.
		*	@param StepCallback An optional callback that is fired every time that a step is completed.
		*/
		void Start( OnPathEndCallback PathEndCallback = OnPathEndCallback(), OnStepCallback StepCallback = OnStepCallback() );

		/** Stop the animation ( Enable = false ) */
		void Stop();

		/** Sets a path end callback */
		void SetPathEndCallback( OnPathEndCallback PathEndCallback );

		/** Sets a step callback */
		void SetStepCallback( OnStepCallback StepCallback );

		/** Update the movement interpolation */
		void Update( const cTime& Elapsed );

		/** Reset the class */
		void Reset();

		/** @return The Current Position */
		const eeVector2f& GetPos();

		/** @return If movement interpolation is a loop */
		bool Loop() const;

		/** Set if loop the movement interpolation */
		void Loop( const bool& loop );

		/** Clear all the waypoints */
		void ClearWaypoints();

		/** @return If the animation ended */
		bool Ended() const;

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void SetTotalTime( const eeDouble& TotTime );

		/** @return The Current Node */
		cWaypoint * GetCurrentActual() const;

		/** @return The Next Node */
		cWaypoint * GetCurrentNext() const;

		/** @return The Current Position in the vector */
		const Uint32& GetCurrentPos() const;

		/** @return the vector of waypoints */
		const std::vector<cWaypoint>& GetWaypoints() const;

		/** Set the current interpolation speed ( This will destroy the time of the interpolation and create one depending on the speed ) ( pixels per second ) */
		void Speed( const eeFloat& Speed );

		/** Get the current interpolation speed */
		const eeFloat& Speed() const;

		/** @return If enabled */
		const bool& Enabled() const;

		/** Set it enabled or not */
		void Enabled( const bool& Enabled );

		/** Set the type of interpolation to be used */
		void Type( Ease::Interpolation InterpolationType );

		/** @return The type of the interpolation */
		const eeInt& Type() const;
	protected:
		eeInt mType;
		bool mEnable;
		bool mUpdate;
		bool mLoop;
		bool mEnded;

		eeFloat mTotDist;
		eeVector2f mCurPos;
		Uint32 mCurPoint;
		eeDouble mCurTime;
		eeFloat mSpeed;

		cWaypoint* mActP;
		cWaypoint* mNexP;

		std::vector<cWaypoint> mPoints;

		OnPathEndCallback mOnPathEndCallback;

		OnStepCallback mOnStepCallback;
};

}}

#endif
