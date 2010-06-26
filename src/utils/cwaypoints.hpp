#ifndef EE_UTILSCWAYPOINTS_H
#define EE_UTILSCWAYPOINTS_H

#include "base.hpp"
#include "vector2.hpp"

namespace EE { namespace Utils {

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

		typedef boost::function0<void> OnPathEndCallback;

		/** Add a new waypoint */
		void AddWaypoint( const eeVector2f& Pos, const eeFloat& Time = 0.f );

		/** Edit a waypoint */
		bool EditWaypoint( const eeUint& PointNum, const eeVector2f& NewPos, const eeFloat& NewTime );

		/** Erase a waypoint */
		bool EraseWaypoint( const eeUint& PointNum );

		/** Start the animation ( will reset the current state, and start from the beginning )
		@param PathEndCallback An optional callback fired when the animation ends.
		*/
		void Start( OnPathEndCallback PathEndCallback = NULL );

		/** Stop the animation ( Enable = false ) */
		void Stop();

		/** Update the movement interpolation */
		void Update( const eeFloat& Elapsed );

		/** Reset the class */
		void Reset();

		/** @return The Current Position */
		const eeVector2f& GetPos();

		/** @return If movement interpolation is a loop */
		bool Loop() const { return mLoop; }
		/** Set if loop the movement interpolation */
		void Loop( const bool& loop ) { mLoop = loop; }

		/** Clear all the waypoints */
		void ClearWaypoints();

		/** @return If the animation ended */
		bool Ended() const { return mEnded; }

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void SetTotalTime( const eeFloat& TotTime );

		/** @return The Current Node */
		cWaypoint* GetCurrentActual() const	{ return mActP; }

		/** @return The Next Node */
		cWaypoint* GetCurrentNext() const { return mNexP; }

		/** @return The Current Position in the vector */
		const Uint32 GetCurrentPos() const { return mCurPoint; }

		/** @return the vector of waypoints */
		const std::vector<cWaypoint>* const GetWaypoints() { return &mPoints; }

		/** Set the current interpolation speed ( This will destroy the time of the interpolation and create one depending on the speed ) */
		void Speed( const eeFloat& Speed );

		/** Get the current interpolation speed */
		const eeFloat Speed() const { return mSpeed; }

		/** @return If enabled */
		bool& Enabled() { return mEnable; }

		/** Set it enabled or not */
		void Enabled( const bool& Enabled ) { mEnable = Enabled; }

		/** Set the type of interpolation to be used */
		void Type( EE_INTERPOLATION InterpolationType );

		/** @return The type of the interpolation */
		const eeInt Type() const;
	protected:
		eeInt mType;
		bool mEnable;
		bool mUpdate;
		bool mLoop;
		bool mEnded;

		eeFloat mTotDist;
		eeVector2f mCurPos;
		Uint32 mCurPoint;
		eeFloat mCurTime;
		eeFloat mSpeed;

		cWaypoint* mActP;
		cWaypoint* mNexP;

		std::vector<cWaypoint> mPoints;

		OnPathEndCallback mOnPathEndCallback;
};

}}

#endif
