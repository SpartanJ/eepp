#ifndef EE_MATHCINTERPOLATION_H
#define EE_MATHCINTERPOLATION_H

#include <eepp/math/base.hpp>
#include <eepp/system/time.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace Math {

/** @brief The basic 1d point template. */
template <typename T>
class tPoint1d {
	public:
		tPoint1d() { p = 0; t = 0.f; }
		tPoint1d( const T& Pos, const Float& Time ) { p = Pos; t = Time; }
		T p;
		Float t;
};
typedef tPoint1d<Float> Point1d;

/** @brief A interpolation movement manager, used for movement interpolations. */
class EE_API Interpolation {
	public:
		Interpolation();

		~Interpolation();

		typedef cb::Callback0<void> OnPathEndCallback;

		typedef cb::Callback0<void> OnStepCallback;

		/** Add a new point */
		void addWaypoint( const Float Pos, const Float Time = 0 );

		/** Edit a point */
		bool editWaypoint( const unsigned int& PointNum, const Float& NewPos, const Float NewTime = 0 );

		/** Erase a point */
		bool eraseWaypoint( const unsigned int& PointNum );

		/** Start the animation */
		void start( OnPathEndCallback PathEndCallback = OnPathEndCallback(), OnStepCallback StepCallback = OnStepCallback() );

		/** Stop the animation */
		void stop();

		/** Sets a path end callback */
		void setPathEndCallback( OnPathEndCallback PathEndCallback );

		/** Sets a step callback */
		void setStepCallback( OnStepCallback StepCallback );

		/** Update the movement interpolation */
		void update( const Time& Elapsed );

		/** Reset the class */
		void reset();

		/** @return The Current Position */
		const Float& getPos();

		/** @return The Current Real Position */
		const Float& getRealPos() const;

		/** @return If movement interpolation is a loop */
		const bool&	getLoop() const;

		/** Set if loop the movement interpolation */
		void setLoop( const bool& loop );

		/** Clear all the points */
		void clearWaypoints();

		/** @return If the animation ended */
		const bool& ended() const;

		/** Set the current interpolation speed */
		void setSpeed( const Float speed );

		/** Get the current interpolation speed */
		const Float& getSpeed() const;

		/** @return If enabled */
		const bool& isEnabled() const;

		void setEnabled( const bool& enabled );

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void setTotalTime( const Time& TotTime );

		/** @return the vector of points */
		const std::vector<Point1d>& getPoints() const;

		/** @return The Current Node */
		Point1d* getCurrentActual() const;

		/** @return The Next Node */
		Point1d* getCurrentNext() const;

		/** @return The Current Position in the vector */
		const unsigned int& getCurrentPos() const;

		/** @return The path end position */
		const Float& getEndPos();

		/** Set the type of interpolation to be used */
		void setType( Ease::Interpolation InterpolationType );

		/** @return The type of the interpolation */
		const int& getType() const;
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

		std::vector<Point1d> mPoints;

		Point1d* mActP;
		Point1d* mNexP;

		OnPathEndCallback		mOnPathEndCallback;

		OnStepCallback			mOnStepCallback;
};

}}

#endif
