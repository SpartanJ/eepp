#include "cinterpolation.hpp"
#include "easing.hpp"
using namespace EE::Utils::easing;

namespace EE { namespace Utils {

cInterpolation::cInterpolation() :
	mType(LINEAR),
	mEnable(false),
	mUpdate(true),
	mLoop(false),
	mEnded(false),
	mTotDist(0.f),
	mCurPoint(0),
	mCurTime(0),
	mSpeed(1.3f),
	mActP(NULL),
	mNexP(NULL),
	mOnPathEndCallback(NULL),
	mOnStepCallback(NULL)
{
}

cInterpolation::~cInterpolation() {
}

void cInterpolation::Start( OnPathEndCallback PathEndCallback, OnStepCallback StepCallback) {
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

void cInterpolation::Stop() {
	mEnable	= false;
}

void cInterpolation::SetPathEndCallback( OnPathEndCallback PathEndCallback ) {
	mOnPathEndCallback = PathEndCallback;
}

void cInterpolation::SetStepCallback( OnStepCallback StepCallback ) {
	mOnStepCallback = StepCallback;
}

void cInterpolation::Reset() {
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

void cInterpolation::ClearWaypoints() {
	Reset();
	mPoints.clear();
}

void cInterpolation::AddWaypoint( const eeFloat Pos, const eeFloat Time ) {
	mPoints.push_back( cPoint1df( Pos, Time ) );

	if ( mPoints.size() >= 2 )
		mTotDist += eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[ mPoints.size() - 2 ].p );
}

bool cInterpolation::EditWaypoint( const eeUint PointNum, const eeFloat NewPos, const eeFloat NewTime ) {
	if ( PointNum < mPoints.size() ) {
		if ( 0 == PointNum )
			mTotDist -= eeabs( mPoints[ PointNum ].p - mPoints[ PointNum + 1 ].p );
		else
			mTotDist -= eeabs( mPoints[ PointNum ].p - mPoints[ PointNum - 1 ].p );

		mPoints[ PointNum ] = cPoint1df( NewPos, NewTime );

		if ( 0 == PointNum ) {
			if ( PointNum + (eeUint)1 < mPoints.size() )
				mTotDist += eeabs( mPoints[ PointNum ].p - mPoints[ PointNum + 1 ].p );
		}
		else
			mTotDist += eeabs( mPoints[ PointNum ].p - mPoints[ PointNum - 1 ].p );

		return true;
	}
	return false;
}

bool cInterpolation::EraseWaypoint( const eeUint PointNum ) {
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

const eeFloat& cInterpolation::GetEndPos() {
	return mPoints[ mPoints.size() - 1 ].p;
}

const eeFloat& cInterpolation::GetPos() {
	return mCurPos;
}

const eeFloat& cInterpolation::GetRealPos() const {
	return mCurPos;
}

void cInterpolation::Update( const eeFloat& Elapsed ) {
	if ( mEnable && mPoints.size() > 1 && mCurPoint != mPoints.size() ) {
		if ( mUpdate ) {
			mCurTime = 0;
			mActP = &mPoints[ mCurPoint ];

			if ( mCurPoint + (eeUint)1 < mPoints.size() ) {
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

		mCurTime += Elapsed;

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

void cInterpolation::SetTotalTime( const eeFloat TotTime ) {
	eeFloat tdist = mTotDist;

	if ( tdist == 0.0f ) {
		mPoints.clear();
		return;
	}

	if ( mLoop ) {
		tdist += eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p );
		mPoints[ mPoints.size() - 1 ].t = eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p ) * TotTime / tdist;
	}

	for ( eeUint i = 0; i < mPoints.size() - 1; i++) {
		eeFloat CurDist = eeabs( mPoints[i].p - mPoints[i + 1].p );
		mPoints[i].t = CurDist * TotTime / tdist;
	}
}

void cInterpolation::Speed( const eeFloat speed ) {
	eeFloat tdist	= mTotDist;
	mSpeed	= speed;

	if ( mPoints.size() ) {
		if ( tdist == 0.0f ) {
			mPoints.clear();
			return;
		}

		eeFloat TotTime = tdist * mSpeed;
		if ( mLoop ) {
			tdist += eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p );
			mPoints[ mPoints.size() - 1 ].t = eeabs( mPoints[ mPoints.size() - 1 ].p - mPoints[0].p ) * TotTime / tdist;
			TotTime = tdist * mSpeed;
		}

		for ( eeUint i = 0; i < mPoints.size() - 1; i++) {
			eeFloat CurDist = eeabs( mPoints[i].p - mPoints[i + 1].p );
			mPoints[i].t = CurDist * TotTime / tdist;
		}
	}
}

void cInterpolation::Type( EE_INTERPOLATION InterpolationType ) {
	mType = InterpolationType;
}

const eeInt& cInterpolation::Type() const {
	return mType;
}

}}
