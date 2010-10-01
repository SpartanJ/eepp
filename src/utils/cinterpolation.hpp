#ifndef EE_UTILSCINTERPOLATION_H
#define EE_UTILSCINTERPOLATION_H

#include "base.hpp"

namespace EE { namespace Utils {

/** @brief The basic 1d point template. */
template <typename T>
class Point1d {
	public:
		Point1d() { p = 0; t = 0.f; }
		Point1d( const T& Pos, const eeFloat& Time ) { p = Pos; t = Time; }
		T p;
		eeFloat t;
};
typedef Point1d<eeFloat> cPoint1df;

/** @brief A interpolation movement manager, used for movement interpolations. */
class EE_API cInterpolation {
	public:
		cInterpolation();

		~cInterpolation();

		typedef cb::Callback0<void> OnPathEndCallback;

		/** Add a new point */
		void AddWaypoint( const eeFloat Pos, const eeFloat Time = 0 );

		/** Edit a point */
		bool EditWaypoint( const eeUint PointNum, const eeFloat NewPos, const eeFloat NewTime = 0 );

		/** Erase a point */
		bool EraseWaypoint( const eeUint PointNum );

		/** Start the animation */
		void Start( OnPathEndCallback PathEndCallback = OnPathEndCallback() );

		/** Stop the animation */
		void Stop();

		/** Update the movement interpolation */
		void Update( const eeFloat& Elapsed );

		/** Reset the class */
		void Reset();

		/** @return The Current Position */
		const eeFloat& GetPos();

		/** @return The Current Real Position */
		const eeFloat& GetRealPos() const;

		/** @return If movement interpolation is a loop */
		const bool&	Loop() const { return mLoop; }

		/** Set if loop the movement interpolation */
		void Loop( const bool& loop ) { mLoop = loop; }

		/** Clear all the points */
		void ClearWaypoints();

		/** @return If the animation ended */
		const bool& Ended() const { return mEnded; }

		/** Set the current interpolation speed */
		void Speed( const eeFloat speed );

		/** Get the current interpolation speed */
		const eeFloat& Speed() const { return mSpeed; }

		/** @return If enabled */
		const bool& Enabled() { return mEnable; }

		void Enabled( const bool enabled ) { mEnable = enabled; }

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void SetTotalTime( const eeFloat TotTime );

		/** @return the vector of points */
		const std::vector<cPoint1df>& GetPoints() const { return mPoints; }

		/** @return The Current Node */
		cPoint1df* GetCurrentActual() const	{ return mActP; }

		/** @return The Next Node */
		cPoint1df* GetCurrentNext() const { return mNexP; }

		/** @return The Current Position in the vector */
		const eeUint& GetCurrentPos() const { return mCurPoint; }

		/** @return The path end position */
		const eeFloat& GetEndPos();

		/** Set the type of interpolation to be used */
		void Type( EE_INTERPOLATION InterpolationType );

		/** @return The type of the interpolation */
		const eeInt& Type() const;
	protected:
		eeInt mType;
		bool mEnable;
		bool mUpdate;
		bool mLoop;
		bool mEnded;

		eeFloat mTotDist;
		eeFloat mCurPos;
		eeUint mCurPoint;
		eeFloat mCurTime;

		eeFloat mSpeed;

		std::vector<cPoint1df> mPoints;

		cPoint1df* mActP;
		cPoint1df* mNexP;

		OnPathEndCallback		mOnPathEndCallback;
};

}}

#endif
