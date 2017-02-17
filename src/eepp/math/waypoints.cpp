#include <eepp/math/waypoints.hpp>
#include <eepp/math/easing.hpp>
using namespace EE::Math::easing;

namespace EE { namespace Math {

Waypoints::Waypoints() :
	mType(Ease::Linear),
	mEnable(false),
	mUpdate(true),
	mLoop(false),
	mEnded(false),
	mTotDist(0.f),
	mCurPoint(0),
	mCurTime(0.0f),
	mSpeed(1.3f),
	mOnPathEndCallback(NULL),
	mOnStepCallback(NULL)
{
}

Waypoints::~Waypoints() {
}

void Waypoints::start( OnPathEndCallback PathEndCallback, OnStepCallback StepCallback ) {
	mEnable				= true;
	mOnPathEndCallback	= PathEndCallback;
	mOnStepCallback		= StepCallback;

	if ( mPoints.size() ) {
		mActP = &mPoints[ 0 ];

		if ( mPoints.size() > 1 )
			mNexP = &mPoints[ 1 ];

		mCurPos = mPoints[0].p;
	} else {
		mEnable = false;
	}
}

void Waypoints::stop() {
	mEnable = false;
}

void Waypoints::setPathEndCallback( OnPathEndCallback PathEndCallback ) {
	mOnPathEndCallback = PathEndCallback;
}

void Waypoints::setStepCallback( OnStepCallback StepCallback ) {
	mOnStepCallback = StepCallback;
}

void Waypoints::reset() {
	mTotDist = 0.f;
	mEnable = false;
	mCurPoint = 0;
	mCurTime = 0;
	mUpdate = true;
	mEnded = false;

	if ( mPoints.size() )
		mCurPos = mPoints[0].p;
}

void Waypoints::clearWaypoints() {
	reset();
	mPoints.clear();
}

void Waypoints::addWaypoint( const Vector2f& Pos, const Float& Time ) {
	mPoints.push_back( Waypoint(Pos, Time) );

	if ( mPoints.size() >= 2 )
	{
		mTotDist += mPoints[ mPoints.size() - 1 ].p.distance( mPoints[ mPoints.size() - 2 ].p );
	}
}

bool Waypoints::editWaypoint( const unsigned int& PointNum, const Vector2f& NewPos, const Float& NewTime  ) {
	if ( PointNum < mPoints.size() ) {
		if ( 0 == PointNum )
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		else
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );

		mPoints[ PointNum ] = Waypoint( NewPos, NewTime );

		if ( 0 == PointNum ) {
			if ( PointNum + (unsigned int)1 < mPoints.size() )
				mTotDist += mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		} else
			mTotDist += mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );

		return true;
	}
	return false;
}

bool Waypoints::eraseWaypoint( const unsigned int& PointNum ) {
	if ( PointNum < mPoints.size() && !mEnable ) {
		if ( 0 == PointNum )
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		else
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );

		mPoints.erase( mPoints.begin() + PointNum );

		return true;
	}
	return false;
}

void Waypoints::setSpeed( const Float& Speed ) {
	Float tdist = mTotDist;
	mSpeed = Speed;
	Float CurDist;

	if ( mPoints.size() ) {
		if ( tdist == 0.0f ) {
			mPoints.clear();
			return;
		}

		Float TotTime = tdist * ( 1000.f / mSpeed );

		if ( mLoop ) {
			CurDist = mPoints[ mPoints.size() - 1 ].p.distance( mPoints[0].p );
			tdist += CurDist;

			mPoints[ mPoints.size() - 1 ].t = CurDist * TotTime / tdist;
			TotTime = tdist * ( 1000.f / mSpeed );
		}

		for ( unsigned int i = 0; i < mPoints.size() - 1; i++) {
			CurDist = mPoints[i].p.distance( mPoints[i + 1].p );
			mPoints[i].t = CurDist * TotTime / tdist;
		}
	}
}

const Vector2f& Waypoints::getPos() {
	return mCurPos;
}

void Waypoints::update( const Time& Elapsed ) {
	if ( mEnable && mPoints.size() > 1 && mCurPoint != mPoints.size() ) {
		if ( mUpdate ) {
			mCurTime = 0;
			mActP = &mPoints[ mCurPoint ];

			if ( mCurPoint + 1 < mPoints.size() ) {
				mNexP = &mPoints[ mCurPoint + 1 ];

				if ( mOnStepCallback.IsSet() )
					mOnStepCallback();
			} else {
				if ( mOnStepCallback.IsSet() )
					mOnStepCallback();

				if ( mLoop ) {
					mNexP = &mPoints[ 0 ];

					if ( mOnPathEndCallback.IsSet() )
						mOnPathEndCallback();
				} else {
					mEnable = false;
					mEnded = true;

					if ( mOnPathEndCallback.IsSet() ) {
						mOnPathEndCallback();
						mOnPathEndCallback.Reset();
					}
					return;
				}
			}
			mUpdate = false;
		}

		mCurTime += Elapsed.asMilliseconds();

		mCurPos.x = easingCb[ mType ]( mCurTime, mActP->p.x, ( mNexP->p.x - mActP->p.x ), mActP->t );
		mCurPos.y = easingCb[ mType ]( mCurTime, mActP->p.y, ( mNexP->p.y - mActP->p.y ), mActP->t );

		if ( mCurTime >= mActP->t ) {
			mCurPos = mNexP->p;

			mUpdate = true;

			mCurPoint++;

			if ( mCurPoint == mPoints.size() && mLoop )
				mCurPoint = 0;
		}
	}
}

void Waypoints::setTotalTime( const Time& TotTime ) {
	unsigned int i;
	Float tdist = mTotDist;

	if ( !mPoints.size() )
		return;

	if ( tdist == 0.0f ) {
		mPoints.clear();
		return;
	}

	if ( mLoop ) {
		tdist += mPoints[ mPoints.size() - 1 ].p.distance( mPoints[0].p );
		mPoints[ mPoints.size() - 1 ].t = mPoints[ mPoints.size() - 1 ].p.distance( mPoints[0].p ) * TotTime.asMilliseconds() / tdist;
	}

	for (i = 0; i < mPoints.size() - 1; i++)
		mPoints[i].t = mPoints[i].p.distance( mPoints[i + 1].p ) * TotTime.asMilliseconds() / tdist;
}

void Waypoints::setType( Ease::Interpolation InterpolationType ) {
	mType = InterpolationType;
}

const int& Waypoints::getType() const {
	return mType;
}

bool Waypoints::getLoop() const {
	return mLoop;
}

void Waypoints::setLoop( const bool& loop ) {
	mLoop = loop;
}

bool Waypoints::ended() const {
	return mEnded;
}

Waypoint * Waypoints::getCurrentActual() const {
	return mActP;
}

Waypoint * Waypoints::getCurrentNext() const {
	return mNexP;
}

const Uint32& Waypoints::getCurrentPos() const {
	return mCurPoint;
}

const std::vector<Waypoint>& Waypoints::getWaypoints() const {
	return mPoints;
}

const Float& Waypoints::getSpeed() const {
	return mSpeed;
}

const bool& Waypoints::isEnabled() const {
	return mEnable;
}

void Waypoints::setEnabled( const bool& Enabled ) {
	mEnable = Enabled;
}

}}
