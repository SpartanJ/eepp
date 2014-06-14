#ifndef EE_MATHCINTERPOLATION_H
#define EE_MATHCINTERPOLATION_H

#include <eepp/math/base.hpp>
#include <eepp/system/ctime.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace Math {

/** @brief The basic 1d point template. */
template <typename T>
class Point1d {
	public:
		Point1d() { p = 0; t = 0.f; }
		Point1d( const T& Pos, const Float& Time ) { p = Pos; t = Time; }
		T p;
		Float t;
};
typedef Point1d<Float> cPoint1df;

/** @brief A interpolation movement manager, used for movement interpolations. */
class EE_API cInterpolation {
	public:
		cInterpolation();

		~cInterpolation();

		typedef cb::Callback0<void> OnPathEndCallback;

		typedef cb::Callback0<void> OnStepCallback;

		/** Add a new point */
		void AddWaypoint( const Float Pos, const Float Time = 0 );

		/** Edit a point */
		bool EditWaypoint( const unsigned int& PointNum, const Float& NewPos, const Float NewTime = 0 );

		/** Erase a point */
		bool EraseWaypoint( const unsigned int& PointNum );

		/** Start the animation */
		void Start( OnPathEndCallback PathEndCallback = OnPathEndCallback(), OnStepCallback StepCallback = OnStepCallback() );

		/** Stop the animation */
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
		const Float& GetPos();

		/** @return The Current Real Position */
		const Float& GetRealPos() const;

		/** @return If movement interpolation is a loop */
		const bool&	Loop() const;

		/** Set if loop the movement interpolation */
		void Loop( const bool& loop );

		/** Clear all the points */
		void ClearWaypoints();

		/** @return If the animation ended */
		const bool& Ended() const;

		/** Set the current interpolation speed */
		void Speed( const Float Speed );

		/** Get the current interpolation speed */
		const Float& Speed() const;

		/** @return If enabled */
		const bool& Enabled() const;

		void Enabled( const bool& Enabled );

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void SetTotalTime( const cTime& TotTime );

		/** @return the vector of points */
		const std::vector<cPoint1df>& GetPoints() const;

		/** @return The Current Node */
		cPoint1df* GetCurrentActual() const;

		/** @return The Next Node */
		cPoint1df* GetCurrentNext() const;

		/** @return The Current Position in the vector */
		const unsigned int& GetCurrentPos() const;

		/** @return The path end position */
		const Float& GetEndPos();

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
		Float mCurPos;
		unsigned int mCurPoint;
		double mCurTime;

		Float mSpeed;

		std::vector<cPoint1df> mPoints;

		cPoint1df* mActP;
		cPoint1df* mNexP;

		OnPathEndCallback		mOnPathEndCallback;

		OnStepCallback			mOnStepCallback;
};

}}

#endif
