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
class tPoint2d {
	public:
		tPoint2d() { p = Vector2<T>(0,0); t = 0; }
		tPoint2d( const Vector2<T>& Pos, const Float& Time ) { p = Pos; t = Time; }
		Vector2<T> p;
		Float t;
};
typedef tPoint2d<Float> Point2d;

/** @brief A waypoint manager, used for movement interpolations. */
class EE_API Interpolation2d {
	public:
		Interpolation2d();

		~Interpolation2d();

		typedef cb::Callback0<void> OnPathEndCallback;

		typedef cb::Callback0<void> OnStepCallback;

		/** Add a new waypoint */
		void addWaypoint( const Vector2f& Pos, const Float& Time = 0.f );

		/** Edit a waypoint */
		bool editWaypoint( const unsigned int& PointNum, const Vector2f& NewPos, const Float& NewTime );

		/** Erase a waypoint */
		bool eraseWaypoint( const unsigned int& PointNum );

		/** Start the animation ( will reset the current state, and start from the beginning )
		*	@param PathEndCallback An optional callback fired when the animation ends.
		*	@param StepCallback An optional callback that is fired every time that a step is completed.
		*/
		void start( OnPathEndCallback PathEndCallback = OnPathEndCallback(), OnStepCallback StepCallback = OnStepCallback() );

		/** Stop the animation ( Enable = false ) */
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
		const Vector2f& getPos();

		/** @return If movement interpolation is a loop */
		bool getLoop() const;

		/** Set if loop the movement interpolation */
		void setLoop( const bool& loop );

		/** Clear all the waypoints */
		void clearWaypoints();

		/** @return If the animation ended */
		bool ended() const;

		/** Instead if setting the time between every waypoing, this set a total time for all the movement interpolation. */
		void setTotalTime( const Time & TotTime );

		/** @return The Current Node */
		Point2d * getCurrentActual() const;

		/** @return The Next Node */
		Point2d * getCurrentNext() const;

		/** @return The Current Position in the vector */
		const Uint32& getCurrentPos() const;

		/** @return the vector of waypoints */
		const std::vector<Point2d>& getWaypoints() const;

		/** Set the current interpolation speed ( This will destroy the time of the interpolation and create one depending on the speed ) ( pixels per second ) */
		void setSpeed( const Float& speed );

		/** Get the current interpolation speed */
		const Float& getSpeed() const;

		/** @return If enabled */
		const bool& isEnabled() const;

		/** Set it enabled or not */
		void setEnabled( const bool& enabled );

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
		Vector2f mCurPos;
		Uint32 mCurPoint;
		double mCurTime;
		Float mSpeed;

		Point2d* mActP;
		Point2d* mNexP;

		std::vector<Point2d> mPoints;

		OnPathEndCallback mOnPathEndCallback;

		OnStepCallback mOnStepCallback;
};

}}

#endif
