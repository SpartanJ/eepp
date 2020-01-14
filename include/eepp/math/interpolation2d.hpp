#ifndef EE_MATHCWAYPOINTS_H
#define EE_MATHCWAYPOINTS_H

#include <eepp/core.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/time.hpp>
#include <vector>

using namespace EE::System;

namespace EE { namespace Math {

/** @brief The basic waypoint class. */
template <typename T> class tPoint2d {
  public:
	tPoint2d() {
		p = Vector2<T>( 0, 0 );
		t = Time::Zero;
	}
	tPoint2d( const Vector2<T>& pos, const Time& time ) {
		p = pos;
		t = time;
	}
	Vector2<T> p;
	Time t;
};
typedef tPoint2d<Float> Point2d;

/** @brief A waypoint manager, used for movement interpolations. */
class EE_API Interpolation2d {
  public:
	Interpolation2d();

	Interpolation2d( std::vector<Point2d> points );

	~Interpolation2d();

	typedef std::function<void( Interpolation2d& )> OnPathEndCallback;

	typedef std::function<void( Interpolation2d& )> OnStepCallback;

	/** Add a new waypoint */
	Interpolation2d& add( const Vector2f& pos, const Time& time = Time::Zero );

	/** Edit a waypoint */
	Interpolation2d& edit( const unsigned int& PointNum, const Vector2f& pos, const Time& time );

	/** Erase a waypoint */
	Interpolation2d& erase( const unsigned int& PointNum );

	/** Same as add( pos, time ).add( pos ); */
	Interpolation2d& wait( const Vector2f& pos, const Time& time );

	/** Same as add( pos, waitTime ).add( pos, addTime ); */
	Interpolation2d& waitAndAdd( const Vector2f& pos, const Time& waitTime, const Time& addTime );

	/** Start the animation ( will reset the current state, and start from the beginning ) */
	Interpolation2d& start();

	/** Start the animation ( will reset the current state, and start from the beginning )
	 *	@param PathEndCallback An optional callback fired when the animation ends.
	 *	@param StepCallback An optional callback that is fired every time that a step is completed.
	 */
	Interpolation2d& start( OnPathEndCallback PathEndCallback,
							OnStepCallback StepCallback = OnStepCallback() );

	/** Stop the animation ( Enable = false ) */
	Interpolation2d& stop();

	/** Sets a path end callback */
	Interpolation2d& setPathEndCallback( OnPathEndCallback PathEndCallback );

	/** Sets a step callback */
	Interpolation2d& setStepCallback( OnStepCallback StepCallback );

	/** Update the movement interpolation */
	void update( const Time& Elapsed );

	/** Reset the class */
	Interpolation2d& reset();

	/** @return The Current Position */
	const Vector2f& getPosition();

	/** @return If movement interpolation is a loop */
	bool getLoop() const;

	/** Set if loop the movement interpolation */
	Interpolation2d& setLoop( const bool& loop );

	/** Clear all the waypoints */
	Interpolation2d& clear();

	/** @return If the animation ended */
	bool ended() const;

	/** Instead if setting the time between every waypoing, this set a total time for all the
	 * movement interpolation. */
	Interpolation2d& setDuration( const Time& TotTime );

	/** @return The Current Node */
	Point2d* getCurrentActual() const;

	/** @return The Next Node */
	Point2d* getCurrentNext() const;

	/** @return The Current Position in the vector */
	const Uint32& getCurrentPositionIndex() const;

	/** @return the vector of waypoints */
	const std::vector<Point2d>& getPoints() const;

	/** @return the vector of waypoints reversed */
	std::vector<Point2d> getReversePoints();

	/** Set the current interpolation speed ( This will destroy the time of the interpolation and
	 * create one depending on the speed ) ( pixels per second ) */
	Interpolation2d& setSpeed( const Float& speed );

	/** Get the current interpolation speed */
	const Float& getSpeed() const;

	/** @return If enabled */
	const bool& isEnabled() const;

	/** Set it enabled or not */
	Interpolation2d& setEnabled( const bool& enabled );

	/** Set the type of interpolation to be used */
	Interpolation2d& setType( Ease::Interpolation InterpolationType );

	/** @return The type of the interpolation */
	const int& getType() const;

	UintPtr getData() const;

	void setData( const UintPtr& data );

	Float getCurrentProgress();

  protected:
	UintPtr mData;
	int mType;
	bool mEnable;
	bool mUpdate;
	bool mLoop;
	bool mEnded;

	Float mTotDist;
	Vector2f mCurPos;
	Uint32 mCurPoint;
	Time mCurTime;
	Float mSpeed;

	Point2d* mActP;
	Point2d* mNexP;

	std::vector<Point2d> mPoints;

	OnPathEndCallback mOnPathEndCallback;

	OnStepCallback mOnStepCallback;
};

}} // namespace EE::Math

#endif
