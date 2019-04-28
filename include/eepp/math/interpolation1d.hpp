#ifndef EE_MATHCINTERPOLATION_H
#define EE_MATHCINTERPOLATION_H

#include <eepp/core.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/system/time.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace Math {

/** @brief The basic 1d point template. */
template <typename T>
class tPoint1d {
	public:
		tPoint1d() { p = 0; t = 0.f; }
		tPoint1d( const T& pos, const Time& time ) { p = pos; t = time; }
		T p;
		Time t;
};
typedef tPoint1d<Float> Point1d;

/** @brief A interpolation movement manager, used for movement interpolations. */
class EE_API Interpolation1d {
	public:
		Interpolation1d();

		Interpolation1d( std::vector<Point1d> points );

		~Interpolation1d();

		typedef std::function<void(Interpolation1d&)> OnPathEndCallback;

		typedef std::function<void(Interpolation1d&)> OnStepCallback;

		/** Add a new point */
		Interpolation1d& add( const Float& pos, const Time& Time = Time::Zero );

		/** Edit a point */
		Interpolation1d& edit( const unsigned int& PointNum, const Float& pos, const Time& time = Time::Zero );

		/** Erase a point */
		Interpolation1d& erase( const unsigned int& PointNum );

		/** Same as add( pos, time ).add( pos ); */
		Interpolation1d& wait( const Float& pos, const Time& time );

		/** Same as add( pos, waitTime ).add( pos, addTime ); */
		Interpolation1d& waitAndAdd( const Float& pos, const Time& waitTime, const Time& addTime );

		/** Start the animation */
		Interpolation1d& start();

		/** Start the animation */
		Interpolation1d& start( OnPathEndCallback PathEndCallback, OnStepCallback StepCallback = OnStepCallback() );

		/** Stop the animation */
		Interpolation1d& stop();

		/** Sets a path end callback */
		Interpolation1d& setPathEndCallback( OnPathEndCallback PathEndCallback );

		/** Sets a step callback */
		Interpolation1d& setStepCallback( OnStepCallback StepCallback );

		/** Update the movement interpolation */
		void update( const Time& Elapsed );

		/** Reset the class */
		Interpolation1d& reset();

		/** @return The Current Position */
		const Float& getPosition();

		/** @return If movement interpolation is a loop */
		const bool&	getLoop() const;

		/** Set if loop the movement interpolation */
		Interpolation1d& setLoop( const bool& loop );

		/** Clear all the points */
		Interpolation1d& clear();

		/** @return If the animation ended */
		const bool& ended() const;

		/** Set the current interpolation speed */
		Interpolation1d& setSpeed( const Float speed );

		/** Get the current interpolation speed */
		const Float& getSpeed() const;

		/** @return If enabled */
		const bool& isEnabled() const;

		Interpolation1d& setEnabled( const bool& enabled );

		/** Instead if setting the time between each waypoint, this set a total time for all the movement interpolation. */
		Interpolation1d& setDuration( const Time& TotTime );

		/** @return the vector of points */
		const std::vector<Point1d>& getPoints() const;

		/** @return the vector of points reversed */
		std::vector<Point1d> getReversePoints();

		/** @return The Current Node */
		Point1d* getCurrentActual() const;

		/** @return The Next Node */
		Point1d* getCurrentNext() const;

		/** @return The Current Position in the vector */
		const unsigned int& getCurrentPositionIndex() const;

		/** @return The path end position */
		const Float& getFinalPosition();

		/** Set the type of interpolation to be used */
		Interpolation1d& setType( Ease::Interpolation InterpolationType );

		/** @return The type of the interpolation */
		const int& getType() const;

		UintPtr getData() const;

		void setData(const UintPtr & data);
	protected:
		UintPtr mData;
		int mType;
		bool mEnable;
		bool mUpdate;
		bool mLoop;
		bool mEnded;

		Float mTotDist;
		Float mCurPos;
		unsigned int mCurPoint;
		Time mCurTime;

		Float mSpeed;

		std::vector<Point1d> mPoints;

		Point1d* mActP;
		Point1d* mNexP;

		OnPathEndCallback		mOnPathEndCallback;

		OnStepCallback			mOnStepCallback;
};

}}

#endif
