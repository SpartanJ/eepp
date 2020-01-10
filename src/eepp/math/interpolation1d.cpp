#include <eepp/math/easing.hpp>
#include <eepp/math/interpolation1d.hpp>
using namespace EE::Math::easing;

namespace EE { namespace Math {

Interpolation1d::Interpolation1d() :
	mData( 0 ),
	mType( Ease::Linear ),
	mEnable( false ),
	mUpdate( true ),
	mLoop( false ),
	mEnded( false ),
	mTotDist( 0.f ),
	mCurPos( 0.f ),
	mCurPoint( 0 ),
	mCurTime( Time::Zero ),
	mSpeed( 1.3f ),
	mActP( NULL ),
	mNexP( NULL ),
	mOnPathEndCallback(),
	mOnStepCallback() {}

Interpolation1d::Interpolation1d( std::vector<Point1d> points ) :
	mData( 0 ),
	mType( Ease::Linear ),
	mEnable( false ),
	mUpdate( true ),
	mLoop( false ),
	mEnded( false ),
	mTotDist( 0.f ),
	mCurPos( 0.f ),
	mCurPoint( 0 ),
	mCurTime( Time::Zero ),
	mSpeed( 1.3f ),
	mPoints( points ),
	mActP( NULL ),
	mNexP( NULL ),
	mOnPathEndCallback(),
	mOnStepCallback() {}

Interpolation1d::~Interpolation1d() {}

Interpolation1d& Interpolation1d::start() {
	mEnable = true;
	if ( mPoints.size() ) {
		mActP = &mPoints[0];

		if ( mPoints.size() > 1 )
			mNexP = &mPoints[1];

		mCurPos = mPoints[0].p;
	} else {
		mEnable = false;
	}

	return *this;
}

Interpolation1d& Interpolation1d::start( OnPathEndCallback PathEndCallback,
										 OnStepCallback StepCallback ) {
	start();
	mOnPathEndCallback = PathEndCallback;
	mOnStepCallback = StepCallback;
	return *this;
}

Interpolation1d& Interpolation1d::stop() {
	mEnable = false;
	return *this;
}

Interpolation1d& Interpolation1d::setPathEndCallback( OnPathEndCallback PathEndCallback ) {
	mOnPathEndCallback = PathEndCallback;
	return *this;
}

Interpolation1d& Interpolation1d::setStepCallback( OnStepCallback StepCallback ) {
	mOnStepCallback = StepCallback;
	return *this;
}

Interpolation1d& Interpolation1d::wait( const Float& pos, const Time& time ) {
	add( pos, time ).add( pos );
	return *this;
}

Interpolation1d& Interpolation1d::waitAndAdd( const EE::Float& pos,
											  const EE::System::Time& waitTime,
											  const EE::System::Time& addTime ) {
	add( pos, waitTime ).add( pos, addTime );
	return *this;
}

Interpolation1d& Interpolation1d::reset() {
	mData = 0;
	mTotDist = 0.f;
	mActP = mNexP = NULL;
	mEnable = false;
	mCurPoint = 0;
	mUpdate = true;
	mEnded = false;
	mCurTime = Time::Zero;
	mOnPathEndCallback = OnPathEndCallback();
	mOnStepCallback = OnStepCallback();

	if ( mPoints.size() )
		mCurPos = mPoints[0].p;

	return *this;
}

Interpolation1d& Interpolation1d::clear() {
	reset();
	mPoints.clear();
	return *this;
}

Interpolation1d& Interpolation1d::add( const Float& pos, const Time& time ) {
	mPoints.push_back( Point1d( pos, time ) );

	if ( mPoints.size() >= 2 )
		mTotDist += eeabs( mPoints[mPoints.size() - 1].p - mPoints[mPoints.size() - 2].p );

	return *this;
}

Interpolation1d& Interpolation1d::edit( const unsigned int& PointNum, const Float& pos,
										const Time& time ) {
	if ( PointNum < mPoints.size() ) {
		if ( 0 == PointNum ) {
			mTotDist -= eeabs( mPoints[PointNum].p - mPoints[PointNum + 1].p );
		} else {
			mTotDist -= eeabs( mPoints[PointNum].p - mPoints[PointNum - 1].p );
		}

		mPoints[PointNum] = Point1d( pos, time );

		if ( 0 == PointNum ) {
			if ( PointNum + 1 < mPoints.size() ) {
				mTotDist += eeabs( mPoints[PointNum].p - mPoints[PointNum + 1].p );
			}
		} else {
			mTotDist += eeabs( mPoints[PointNum].p - mPoints[PointNum - 1].p );
		}
	}

	return *this;
}

Interpolation1d& Interpolation1d::erase( const unsigned int& PointNum ) {
	if ( PointNum < mPoints.size() && !mEnable ) {
		if ( 0 == PointNum ) {
			mTotDist -= eeabs( mPoints[PointNum].p - mPoints[PointNum + 1].p );
		} else {
			mTotDist -= eeabs( mPoints[PointNum].p - mPoints[PointNum - 1].p );
		}

		mPoints.erase( mPoints.begin() + PointNum );
	}

	return *this;
}

const Float& Interpolation1d::getFinalPosition() {
	return mPoints[mPoints.size() - 1].p;
}

const Float& Interpolation1d::getPosition() {
	return mCurPos;
}

void Interpolation1d::update( const Time& Elapsed ) {
	if ( mEnable && mPoints.size() > 1 && mCurPoint != mPoints.size() ) {
		if ( mUpdate ) {
			mCurTime = Time::Zero;
			mActP = &mPoints[mCurPoint];

			if ( mCurPoint + 1 < mPoints.size() ) {
				mNexP = &mPoints[mCurPoint + 1];

				if ( mOnStepCallback )
					mOnStepCallback( *this );
			} else {
				if ( mOnStepCallback )
					mOnStepCallback( *this );

				if ( mLoop ) {
					mNexP = &mPoints[0];

					if ( mOnPathEndCallback )
						mOnPathEndCallback( *this );
				} else {
					mEnable = false;
					mEnded = true;

					if ( mOnPathEndCallback ) {
						mOnPathEndCallback( *this );

						if ( !mEnable )
							mOnPathEndCallback = nullptr;
					}
					return;
				}
			}

			mUpdate = false;
		}

		mCurTime += Elapsed;

		mCurPos = easingCb[mType]( mCurTime.asMilliseconds(), mActP->p, ( mNexP->p - mActP->p ),
								   mActP->t.asMilliseconds() );

		if ( mCurTime >= mActP->t ) {
			mCurPos = mNexP->p;

			mUpdate = true;

			mCurPoint++;

			if ( mCurPoint == mPoints.size() && mLoop )
				mCurPoint = 0;
		}
	}
}

Interpolation1d& Interpolation1d::setDuration( const Time& TotTime ) {
	Float tdist = mTotDist;

	if ( tdist == 0.0f ) {
		mPoints.clear();
		return *this;
	}

	if ( mLoop ) {
		tdist += eeabs( mPoints[mPoints.size() - 1].p - mPoints[0].p );
		mPoints[mPoints.size() - 1].t =
			Milliseconds( eeabs( mPoints[mPoints.size() - 1].p - mPoints[0].p ) *
						  TotTime.asMilliseconds() / tdist );
	}

	for ( unsigned int i = 0; i < mPoints.size() - 1; i++ ) {
		Float CurDist = eeabs( mPoints[i].p - mPoints[i + 1].p );
		mPoints[i].t = Milliseconds( CurDist * TotTime.asMilliseconds() / tdist );
	}

	return *this;
}

Interpolation1d& Interpolation1d::setSpeed( const Float Speed ) {
	Float tdist = mTotDist;
	mSpeed = Speed;
	Float CurDist;

	if ( mPoints.size() ) {
		if ( tdist == 0.0f ) {
			mPoints.clear();
			return *this;
		}

		Float TotTime = tdist * ( 1000.f / mSpeed );

		if ( mLoop ) {
			CurDist = eeabs( mPoints[mPoints.size() - 1].p - mPoints[0].p );
			tdist += CurDist;

			mPoints[mPoints.size() - 1].t = Milliseconds( CurDist * TotTime / tdist );
			TotTime = tdist * ( 1000.f / mSpeed );
		}

		for ( unsigned int i = 0; i < mPoints.size() - 1; i++ ) {
			CurDist = eeabs( mPoints[i].p - mPoints[i + 1].p );
			mPoints[i].t = Milliseconds( CurDist * TotTime / tdist );
		}
	}

	return *this;
}

Interpolation1d& Interpolation1d::setType( Ease::Interpolation InterpolationType ) {
	mType = InterpolationType;
	return *this;
}

const int& Interpolation1d::getType() const {
	return mType;
}

UintPtr Interpolation1d::getData() const {
	return mData;
}

void Interpolation1d::setData( const UintPtr& data ) {
	mData = data;
}

Float Interpolation1d::getCurrentProgress() {
	return mCurTime >= mActP->t ? 1.f : mCurTime.asMilliseconds() / mActP->t.asMilliseconds();
}

const bool& Interpolation1d::getLoop() const {
	return mLoop;
}

Interpolation1d& Interpolation1d::setLoop( const bool& loop ) {
	mLoop = loop;
	return *this;
}

const bool& Interpolation1d::ended() const {
	return mEnded;
}

Point1d* Interpolation1d::getCurrentActual() const {
	return mActP;
}

Point1d* Interpolation1d::getCurrentNext() const {
	return mNexP;
}

const Uint32& Interpolation1d::getCurrentPositionIndex() const {
	return mCurPoint;
}

const std::vector<Point1d>& Interpolation1d::getPoints() const {
	return mPoints;
}

std::vector<Point1d> Interpolation1d::getReversePoints() {
	std::vector<Point1d> reversed;

	for ( auto it = mPoints.rbegin(); it != mPoints.rend(); ++it ) {
		reversed.push_back( *it );
	}

	return reversed;
}

const Float& Interpolation1d::getSpeed() const {
	return mSpeed;
}

const bool& Interpolation1d::isEnabled() const {
	return mEnable;
}

Interpolation1d& Interpolation1d::setEnabled( const bool& Enabled ) {
	mEnable = Enabled;
	return *this;
}

}} // namespace EE::Math
