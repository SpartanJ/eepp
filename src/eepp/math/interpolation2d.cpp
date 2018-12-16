#include <eepp/math/interpolation2d.hpp>
#include <eepp/math/easing.hpp>
using namespace EE::Math::easing;

namespace EE { namespace Math {

Interpolation2d::Interpolation2d() :
	mData(0),
	mType(Ease::Linear),
	mEnable(false),
	mUpdate(true),
	mLoop(false),
	mEnded(false),
	mTotDist(0.f),
	mCurPoint(0),
	mCurTime(Time::Zero),
	mSpeed(1.3f),
	mOnPathEndCallback(),
	mOnStepCallback()
{
}

Interpolation2d::Interpolation2d( std::vector<Point2d> points ) :
	mData(0),
	mType(Ease::Linear),
	mEnable(false),
	mUpdate(true),
	mLoop(false),
	mEnded(false),
	mTotDist(0.f),
	mCurPoint(0),
	mCurTime(Time::Zero),
	mSpeed(1.3f),
	mPoints(points),
	mOnPathEndCallback(),
	mOnStepCallback()
{
}

Interpolation2d::~Interpolation2d() {
}

Interpolation2d& Interpolation2d::start() {
	mEnable				= true;

	if ( mPoints.size() ) {
		mActP = &mPoints[ 0 ];

		if ( mPoints.size() > 1 )
			mNexP = &mPoints[ 1 ];

		mCurPos = mPoints[0].p;
	} else {
		mEnable = false;
	}

	return *this;
}

Interpolation2d& Interpolation2d::start( OnPathEndCallback PathEndCallback, OnStepCallback StepCallback ) {
	start();
	mOnPathEndCallback	= PathEndCallback;
	mOnStepCallback		= StepCallback;
	return *this;
}

Interpolation2d& Interpolation2d::stop() {
	mEnable = false;
	return *this;
}

Interpolation2d& Interpolation2d::setPathEndCallback( OnPathEndCallback PathEndCallback ) {
	mOnPathEndCallback = PathEndCallback;
	return *this;
}

Interpolation2d& Interpolation2d::setStepCallback( OnStepCallback StepCallback ) {
	mOnStepCallback = StepCallback;
	return *this;
}

Interpolation2d& Interpolation2d::reset() {
	mData = 0;
	mTotDist = 0.f;
	mEnable = false;
	mCurPoint = 0;
	mCurTime = Time::Zero;
	mUpdate = true;
	mEnded = false;
	mOnPathEndCallback = OnPathEndCallback();
	mOnStepCallback = OnStepCallback();

	if ( mPoints.size() )
		mCurPos = mPoints[0].p;

	return *this;
}

Interpolation2d& Interpolation2d::clear() {
	reset();
	mPoints.clear();

	return *this;
}

Interpolation2d& Interpolation2d::add( const Vector2f& pos, const Time& time ) {
	mPoints.push_back( Point2d( pos, time ) );

	if ( mPoints.size() >= 2 ) {
		mTotDist += mPoints[ mPoints.size() - 1 ].p.distance( mPoints[ mPoints.size() - 2 ].p );
	}

	return *this;
}

Interpolation2d& Interpolation2d::edit(const unsigned int& PointNum, const Vector2f& pos, const Time& time  ) {
	if ( PointNum < mPoints.size() ) {
		if ( 0 == PointNum ) {
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		} else {
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );
		}

		mPoints[ PointNum ] = Point2d( pos, time );

		if ( 0 == PointNum ) {
			if ( PointNum + (unsigned int)1 < mPoints.size() )
				mTotDist += mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		} else {
			mTotDist += mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );
		}
	}

	return *this;
}

Interpolation2d& Interpolation2d::erase( const unsigned int& PointNum ) {
	if ( PointNum < mPoints.size() && !mEnable ) {
		if ( 0 == PointNum ) {
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum + 1 ].p );
		} else {
			mTotDist -= mPoints[ PointNum ].p.distance( mPoints[ PointNum - 1 ].p );
		}

		mPoints.erase( mPoints.begin() + PointNum );
	}

	return *this;
}

Interpolation2d &Interpolation2d::wait( const Vector2f& pos, const Time& time ) {
	add( pos, time ).add( pos );
	return *this;
}

Interpolation2d &Interpolation2d::waitAndAdd( const Vector2f& pos, const Time& waitTime, const Time& addTime ) {
	add( pos, waitTime ).add( pos, addTime );
	return *this;
}

Interpolation2d& Interpolation2d::setSpeed( const Float& Speed ) {
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
			CurDist = mPoints[ mPoints.size() - 1 ].p.distance( mPoints[0].p );
			tdist += CurDist;

			mPoints[ mPoints.size() - 1 ].t = Milliseconds( CurDist * TotTime / tdist );
			TotTime = tdist * ( 1000.f / mSpeed );
		}

		for ( unsigned int i = 0; i < mPoints.size() - 1; i++) {
			CurDist = mPoints[i].p.distance( mPoints[i + 1].p );
			mPoints[i].t = Milliseconds( CurDist * TotTime / tdist );
		}
	}

	return *this;
}

const Vector2f& Interpolation2d::getPosition() {
	return mCurPos;
}

void Interpolation2d::update( const Time& Elapsed ) {
	if ( mEnable && mPoints.size() > 1 && mCurPoint != mPoints.size() ) {
		if ( mUpdate ) {
			mCurTime = Time::Zero;
			mActP = &mPoints[ mCurPoint ];

			if ( mCurPoint + 1 < mPoints.size() ) {
				mNexP = &mPoints[ mCurPoint + 1 ];

				if ( mOnStepCallback )
					mOnStepCallback(*this);
			} else {
				if ( mOnStepCallback )
					mOnStepCallback(*this);

				if ( mLoop ) {
					mNexP = &mPoints[ 0 ];

					if ( mOnPathEndCallback )
						mOnPathEndCallback(*this);
				} else {
					mEnable = false;
					mEnded = true;

					if ( mOnPathEndCallback ) {
						mOnPathEndCallback(*this);

						if ( !mEnable )
							mOnPathEndCallback = nullptr;
					}
					return;
				}
			}
			mUpdate = false;
		}

		mCurTime += Elapsed;

		mCurPos.x = easingCb[ mType ]( mCurTime.asMilliseconds(), mActP->p.x, ( mNexP->p.x - mActP->p.x ), mActP->t.asMilliseconds() );
		mCurPos.y = easingCb[ mType ]( mCurTime.asMilliseconds(), mActP->p.y, ( mNexP->p.y - mActP->p.y ), mActP->t.asMilliseconds() );

		if ( mCurTime >= mActP->t ) {
			mCurPos = mNexP->p;

			mUpdate = true;

			mCurPoint++;

			if ( mCurPoint == mPoints.size() && mLoop )
				mCurPoint = 0;
		}
	}
}

Interpolation2d& Interpolation2d::setDuration( const Time& TotTime ) {
	unsigned int i;
	Float tdist = mTotDist;

	if ( !mPoints.size() )
		return *this;

	if ( tdist == 0.0f ) {
		mPoints.clear();
		return *this;
	}

	if ( mLoop ) {
		tdist += mPoints[ mPoints.size() - 1 ].p.distance( mPoints[0].p );
		mPoints[ mPoints.size() - 1 ].t = Milliseconds( mPoints[ mPoints.size() - 1 ].p.distance( mPoints[0].p ) * TotTime.asMilliseconds() / tdist );
	}

	for (i = 0; i < mPoints.size() - 1; i++)
		mPoints[i].t = Milliseconds( mPoints[i].p.distance( mPoints[i + 1].p ) * TotTime.asMilliseconds() / tdist );

	return *this;
}

Interpolation2d& Interpolation2d::setType( Ease::Interpolation InterpolationType ) {
	mType = InterpolationType;
	return *this;
}

const int& Interpolation2d::getType() const {
	return mType;
}

UintPtr Interpolation2d::getData() const
{
	return mData;
}

void Interpolation2d::setData(const UintPtr & data)
{
	mData = data;
}

bool Interpolation2d::getLoop() const {
	return mLoop;
}

Interpolation2d& Interpolation2d::setLoop( const bool& loop ) {
	mLoop = loop;
	return *this;
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

const Uint32& Interpolation2d::getCurrentPositionIndex() const {
	return mCurPoint;
}

const std::vector<Point2d>& Interpolation2d::getPoints() const {
	return mPoints;
}

std::vector<Point2d> Interpolation2d::getReversePoints() {
	std::vector<Point2d> reversed;

	for ( auto it = mPoints.rbegin(); it != mPoints.rend(); ++it ) {
		reversed.push_back( *it );
	}

	return reversed;
}

const Float& Interpolation2d::getSpeed() const {
	return mSpeed;
}

const bool& Interpolation2d::isEnabled() const {
	return mEnable;
}

Interpolation2d& Interpolation2d::setEnabled( const bool& Enabled ) {
	mEnable = Enabled;
	return *this;
}

}}
