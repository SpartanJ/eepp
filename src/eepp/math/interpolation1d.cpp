#include <eepp/math/interpolation1d.hpp>
#include <eepp/math/easing.hpp>
using namespace EE::Math::easing;

namespace EE { namespace Math {

Interpolation1d::Interpolation1d() :
	mType(Ease::Linear),
	mEnable(false),
	mUpdate(true),
	mLoop(false),
	mEnded(false),
	mTotDist(0.f),
	mCurPos(0.f),
	mCurPoint(0),
	mCurTime(0),
	mSpeed(1.3f),
	mActP(NULL),
	mNexP(NULL),
	mOnPathEndCallback(NULL),
	mOnStepCallback(NULL)
{
}

Interpolation1d::~Interpolation1d() {
}

void Interpolation1d::start( OnPathEndCallback PathEndCallback, OnStepCallback StepCallback) {
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

void Interpolation1d::stop() {
	mEnable	= false;
}

void Interpolation1d::setPathEndCallback( OnPathEndCallback PathEndCallback ) {
	mOnPathEndCallback = PathEndCallback;
}

void Interpolation1d::setStepCallback( OnStepCallback StepCallback ) {
	mOnStepCallback = StepCallback;
}

void Interpolation1d::reset() {
	mTotDist = 0.f;
	mActP = mNexP = NULL;
	mEnable	= false;
	mCurPoint = 0;
	mUpdate	= true;
	mEnded = false;
	mCurTime = 0;
	mOnPathEndCallback = NULL;

	if ( mPoints.size() )
		mCurPos = mPoints[0].p;
}

void Interpolation1d::clearWaypoints() {
	reset();
	mPoints.clear();
}

void Interpolation1d::addWaypoint( const Float Pos, const Float Time ) {
	mPoints.push_back( Point1d( Pos, Time ) );

	if ( mPoints.size() >= 2 )
		mTotDist += eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[ mPoints.size() - 2 ].p );
}

bool Interpolation1d::editWaypoint( const unsigned int& PointNum, const Float& NewPos, const Float NewTime ) {
	if ( PointNum < mPoints.size() ) {
		if ( 0 == PointNum )
			mTotDist -= eeabs( mPoints[ PointNum ].p - mPoints[ PointNum + 1 ].p );
		else
			mTotDist -= eeabs( mPoints[ PointNum ].p - mPoints[ PointNum - 1 ].p );

		mPoints[ PointNum ] = Point1d( NewPos, NewTime );

		if ( 0 == PointNum ) {
			if ( PointNum + 1 < mPoints.size() )
				mTotDist += eeabs( mPoints[ PointNum ].p - mPoints[ PointNum + 1 ].p );
		}
		else
			mTotDist += eeabs( mPoints[ PointNum ].p - mPoints[ PointNum - 1 ].p );

		return true;
	}
	return false;
}

bool Interpolation1d::eraseWaypoint( const unsigned int& PointNum ) {
	if ( PointNum < mPoints.size() && !mEnable ) {
		if ( 0 == PointNum )
			mTotDist -= eeabs( mPoints[ PointNum ].p - mPoints[ PointNum + 1 ].p );
		else
			mTotDist -= eeabs( mPoints[ PointNum ].p - mPoints[ PointNum - 1 ].p );

		mPoints.erase( mPoints.begin() + PointNum );

		return true;
	}
	return false;
}

const Float& Interpolation1d::getEndPos() {
	return mPoints[ mPoints.size() - 1 ].p;
}

const Float& Interpolation1d::getPos() {
	return mCurPos;
}

const Float& Interpolation1d::getRealPos() const {
	return mCurPos;
}

void Interpolation1d::update( const Time& Elapsed ) {
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

		mCurPos = easingCb[ mType ]( mCurTime, mActP->p, ( mNexP->p - mActP->p ), mActP->t );

		if ( mCurTime >= mActP->t ) {
			mCurPos = mNexP->p;

			mUpdate = true;

			mCurPoint++;

			if ( mCurPoint == mPoints.size() && mLoop )
				mCurPoint = 0;
		}
	}
}

void Interpolation1d::setTotalTime( const Time & TotTime ) {
	Float tdist = mTotDist;

	if ( tdist == 0.0f ) {
		mPoints.clear();
		return;
	}

	if ( mLoop ) {
		tdist += eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p );
		mPoints[ mPoints.size() - 1 ].t = eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p ) * TotTime.asMilliseconds() / tdist;
	}

	for ( unsigned int i = 0; i < mPoints.size() - 1; i++) {
		Float CurDist = eeabs( mPoints[i].p - mPoints[i + 1].p );
		mPoints[i].t = CurDist * TotTime.asMilliseconds() / tdist;
	}
}

void Interpolation1d::setSpeed( const Float Speed ) {
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
			CurDist = eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p );
			tdist += CurDist;

			mPoints[ mPoints.size() - 1 ].t = CurDist * TotTime / tdist;
			TotTime = tdist * ( 1000.f / mSpeed );
		}

		for ( unsigned int i = 0; i < mPoints.size() - 1; i++) {
			CurDist = eeabs( mPoints[i].p - mPoints[i + 1].p );
			mPoints[i].t = CurDist * TotTime / tdist;
		}
	}
}

void Interpolation1d::setType( Ease::Interpolation InterpolationType ) {
	mType = InterpolationType;
}

const int& Interpolation1d::getType() const {
	return mType;
}

const bool& Interpolation1d::getLoop() const {
	return mLoop;
}

void Interpolation1d::setLoop( const bool& loop ) {
	mLoop = loop;
}

const bool& Interpolation1d::ended() const {
	return mEnded;
}

Point1d * Interpolation1d::getCurrentActual() const {
	return mActP;
}

Point1d * Interpolation1d::getCurrentNext() const {
	return mNexP;
}

const Uint32& Interpolation1d::getCurrentPos() const {
	return mCurPoint;
}

const std::vector<Point1d>& Interpolation1d::getPoints() const {
	return mPoints;
}

const Float& Interpolation1d::getSpeed() const {
	return mSpeed;
}

const bool& Interpolation1d::isEnabled() const {
	return mEnable;
}

void Interpolation1d::setEnabled( const bool& Enabled ) {
	mEnable = Enabled;
}

}}
