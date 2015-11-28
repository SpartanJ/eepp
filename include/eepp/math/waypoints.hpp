#ifndef EE_MATHCWAYPOINTS_H
#define EE_MATHCWAYPOINTS_H

#include <eepp/math/base.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/time.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace Math {

/** @brief The basic waypoint class. */
template <typename T>
class tWaypoint {
	public:
		tWaypoint() { p = Vector2<T>(0,0); t = 0; }
		tWaypoint( const Vector2<T>& Pos, const Float& Time ) { p = Pos; t = Time; }
		Vector2<T> p;
		Float t;
};
typedef tWaypoint<Float> Waypoint;

/** @brief A waypoint manager, used for movement interpolations. */
class EE_API Waypoints {
	public:
		Waypoints();

		~Waypoints();

		typedef cb::Callback0<void> OnPathEndCallback;

		typedef cb::Callback0<void> OnStepCallback;

		/** Add a new waypoint */
		void AddWaypoint( const Vector2f& Pos, const Float& Time = 0.f );

		/** Edit a waypoint */
		bool EditWaypoint( const unsigned int& PointNum, const Vector2f& NewPos, const Float& NewTime );

		/** Erase a waypoint */
		bool EraseWaypoint( const unsigned int& PointNum );

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
		void Update( const Time& Elapsed );

		/** Reset the class */
		void Reset();

		/** @return The Current Position */
		const Vector2f& GetPos();

		/** @return If movement interpolation is a loop */
		bool Loop() const;

		/** Set if loop the movement interpolation */
		void Loop( const bool& loop );

		/** Clear all the waypoints */
		void ClearWaypoints();

		/** @return If the animation ended */
		bool Ended() const;

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void SetTotalTime( const Time & TotTime );

		/** @return The Current Node */
		Waypoint * GetCurrentActual() const;

		/** @return The Next Node */
		Waypoint * GetCurrentNext() const;

		/** @return The Current Position in the vector */
		const Uint32& GetCurrentPos() const;

		/** @return the vector of waypoints */
		const std::vector<Waypoint>& GetWaypoints() const;

		/** Set the current interpolation speed ( This will destroy the time of the interpolation and create one depending on the speed ) ( pixels per second ) */
		void Speed( const Float& Speed );

		/** Get the current interpolation speed */
		const Float& Speed() const;

		/** @return If enabled */
		const bool& Enabled() const;

		/** Set it enabled or not */
		void Enabled( const bool& Enabled );

		/** Set the type of interpolation to be used */
		void Type( Ease::Interpolation InterpolationType );

		/** @return The type of the interpolation */
		const int& Type() const;
	protected:
		int mType;
		bool mEnable;
		bool mUpdate;
		bool mLoop;
		bool mEnded;

		Float mTotDist;
		Vector2f mCurPos;
		Uint32 mCurPoint;
		double mCurTime;
		Float mSpeed;

		Waypoint* mActP;
		Waypoint* mNexP;

		std::vector<Waypoint> mPoints;

		OnPathEndCallback mOnPathEndCallback;

		OnStepCallback mOnStepCallback;
};

}}

#endif
