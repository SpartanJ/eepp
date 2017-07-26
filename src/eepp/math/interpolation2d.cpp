#include <eepp/math/interpolation2d.hpp>
#include <eepp/math/easing.hpp>
using namespace EE::Math::easing;

namespace EE { namespace Math {

Interpolation2d::Interpolation2d() :
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

Interpolation2d::~Interpolation2d() {
}

void Interpolation2d::start( OnPathEndCallback PathEndCallback, OnStepCallback StepCallback ) {
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

void Interpolation2d::stop() {
	mEnable = false;
}

void Interpolation2d::setPathEndCallback( OnPathEndCallback PathEndCallback ) {
	mOnPathEndCallback = PathEndCallback;
}

void Interpolation2d::setStepCallback( OnStepCallback StepCallback ) {
	mOnStepCallback = StepCallback;
}

void Interpolation2d::reset() {
	mTotDist = 0.f;
	mEnable = false;
	mCurPoint = 0;
	mCurTime = 0;
	mUpdate = true;
	mEnded = false;

	if ( mPoints.size() )
		mCurPos = mPoints[0].p;
}

void Interpolation2d::clearWaypoints() {
	reset();
	mPoints.clear();
}

void Interpolation2d::addWaypoint( const Vector2f& Pos, const Float& Time ) {
	mPoints.push_back( Point2d(Pos, Time) );

	if ( mPoints.size() >= 2 )
	{
		mTotDist += mPoints[ mPoints.size() - 1 ].p.distance( mPoints[ mPoints.size() - 2 ].p );
	}
}

bool Interpolation2d::editWaypoint( const unsigned int& PointNum, const Vector2f& NewPos, const Float& NewTime  ) {
	if ( PointNum < mPoints.size() ) {
		if ( 0 == PointNum )
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		else
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );

		mPoints[ PointNum ] = Point2d( NewPos, NewTime );

		if ( 0 == PointNum ) {
			if ( PointNum + (unsigned int)1 < mPoints.size() )
				mTotDist += mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		} else
			mTotDist += mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );

		return true;
	}
	return false;
}

bool Interpolation2d::eraseWaypoint( const unsigned int& PointNum ) {
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

void Interpolation2d::setSpeed( const Float& Speed ) {
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

const Vector2f& Interpolation2d::getPos() {
	return mCurPos;
}

void Interpolation2d::update( const Time& Elapsed ) {
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

						if ( !mEnable )
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

void Interpolation2d::setTotalTime( const Time& TotTime ) {
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

void Interpolation2d::setType( Ease::Interpolation InterpolationType ) {
	mType = InterpolationType;
}

const int& Interpolation2d::getType() const {
	return mType;
}

bool Interpolation2d::getLoop() const {
	return mLoop;
}

void Interpolation2d::setLoop( const bool& loop ) {
	mLoop = loop;
}

bool Interpolation2d::ended() const {
	return mEnded;
}

Point2d * Interpolation2d::getCurrentActual() const {
	return mActP;
}

Point2d * Interpolation2d::getCurrentNext() const {
	return mNexP;
}

const Uint32& Interpolation2d::getCurrentPos() const {
	return mCurPoint;
}

const std::vector<Point2d>& Interpolation2d::getWaypoints() const {
	return mPoints;
}

const Float& Interpolation2d::getSpeed() const {
	return mSpeed;
}

const bool& Interpolation2d::isEnabled() const {
	return mEnable;
}

void Interpolation2d::setEnabled( const bool& Enabled ) {
	mEnable = Enabled;
}

}}
