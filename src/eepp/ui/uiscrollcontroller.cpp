#include <cmath>
#include <eepp/ui/uiscrollcontroller.hpp>

namespace EE { namespace UI {

static Vector2f clampScrollPosition( const Vector2f& position, const Vector2f& maxPosition ) {
	return { eeclamp( position.x, 0.f, eemax( 0.f, maxPosition.x ) ),
			 eeclamp( position.y, 0.f, eemax( 0.f, maxPosition.y ) ) };
}

static bool isNearlyZero( Float value, Float epsilon ) {
	return std::abs( value ) <= epsilon;
}

UIScrollController::UIScrollController() = default;

void UIScrollController::setPosition( const Vector2f& position, const Vector2f& maxPosition ) {
	mPosition = clampScrollPosition( position, maxPosition );
	mTarget = mPosition;
	mVelocity = Vector2f::Zero;
	mSmoothTweenCount = 0;
	mMode = Mode::Idle;
}

bool UIScrollController::scrollBy( const Vector2f& delta, const Vector2f& currentPosition,
								   const Vector2f& maxPosition, bool smooth, Float durationScale ) {
	if ( mMode != Mode::Smooth ) {
		mPosition = clampScrollPosition( currentPosition, maxPosition );
		mTarget = mPosition;
	}

	const Vector2f oldTarget( mTarget );
	mTarget = clampScrollPosition( mTarget + delta, maxPosition );
	mVelocity = Vector2f::Zero;

	if ( oldTarget == mTarget )
		return false;

	if ( smooth ) {
		const Vector2f effectiveDelta( mTarget - oldTarget );
		const Float requestedDistance = std::sqrt( delta.x * delta.x + delta.y * delta.y );
		const Float effectiveDistance =
			std::sqrt( effectiveDelta.x * effectiveDelta.x + effectiveDelta.y * effectiveDelta.y );
		const Float duration =
			mWheelScrollDuration.asSeconds() * eeclamp( durationScale, 0.f, 1.f ) *
			( requestedDistance > 0.f ? effectiveDistance / requestedDistance : 1.f );

		if ( mSmoothTweenCount > 0 && mSmoothTweens[mSmoothTweenCount - 1].elapsed == 0.f &&
			 mSmoothTweens[mSmoothTweenCount - 1].duration == duration ) {
			mSmoothTweens[mSmoothTweenCount - 1].delta += effectiveDelta;
		} else if ( mSmoothTweenCount < mSmoothTweens.size() ) {
			mSmoothTweens[mSmoothTweenCount++] = { effectiveDelta, 0.f, duration };
		} else {
			mSmoothTweens[mSmoothTweenCount - 1].delta += effectiveDelta;
		}
		mMode = Mode::Smooth;
	} else {
		mPosition = mTarget;
		mSmoothTweenCount = 0;
		mMode = Mode::Idle;
	}

	return true;
}

void UIScrollController::beginDrag( const Vector2f& position, const Vector2f& maxPosition ) {
	mPosition = clampScrollPosition( position, maxPosition );
	mTarget = mPosition;
	mVelocity = Vector2f::Zero;
	mSmoothTweenCount = 0;
	mMode = Mode::Dragging;
}

bool UIScrollController::dragBy( const Vector2f& delta, const Time& elapsed,
								 const Vector2f& maxPosition ) {
	if ( mMode != Mode::Dragging )
		beginDrag( mPosition, maxPosition );

	const Vector2f oldPosition( mPosition );
	mPosition = clampScrollPosition( mPosition + delta, maxPosition );
	mTarget = mPosition;

	const Float seconds = eemax<Float>( 0.f, elapsed.asSeconds() );
	if ( seconds > 0.f ) {
		const Vector2f instantaneousVelocity( ( mPosition - oldPosition ) / seconds );
		const Float blend = 1.f - std::exp( -20.f * seconds );
		mVelocity += ( instantaneousVelocity - mVelocity ) * blend;
	}

	return oldPosition != mPosition;
}

void UIScrollController::endDrag() {
	if ( mMode != Mode::Dragging )
		return;

	if ( isNearlyZero( mVelocity.x, 1.f ) && isNearlyZero( mVelocity.y, 1.f ) ) {
		stop();
	} else {
		mMode = Mode::Momentum;
	}
}

bool UIScrollController::update( const Time& elapsed, const Vector2f& maxPosition ) {
	if ( mMode == Mode::Idle || mMode == Mode::Dragging )
		return false;

	const Float seconds = eemax<Float>( 0.f, elapsed.asSeconds() );
	if ( seconds <= 0.f )
		return false;

	const Vector2f oldPosition( mPosition );
	if ( mMode == Mode::Smooth ) {
		mTarget = clampScrollPosition( mTarget, maxPosition );
		Uint32 activeTweens = 0;
		for ( Uint32 i = 0; i < mSmoothTweenCount; ++i ) {
			auto& tween = mSmoothTweens[i];
			const Float oldProgress =
				tween.duration > 0.f ? eeclamp( tween.elapsed / tween.duration, 0.f, 1.f ) : 1.f;
			tween.elapsed += seconds;
			const Float progress =
				tween.duration > 0.f ? eeclamp( tween.elapsed / tween.duration, 0.f, 1.f ) : 1.f;
			const Float oldEased = 1.f - ( 1.f - oldProgress ) * ( 1.f - oldProgress );
			const Float eased = 1.f - ( 1.f - progress ) * ( 1.f - progress );
			mPosition += tween.delta * ( eased - oldEased );

			if ( progress < 1.f )
				mSmoothTweens[activeTweens++] = tween;
		}
		mSmoothTweenCount = activeTweens;
		mPosition = clampScrollPosition( mPosition, maxPosition );

		if ( mSmoothTweenCount == 0 ) {
			mPosition = mTarget;
			mMode = Mode::Idle;
		}
	} else {
		const Vector2f decay( std::exp( -mDragDeceleration.x * seconds ),
							  std::exp( -mDragDeceleration.y * seconds ) );
		const Vector2f displacement( mVelocity.x * ( 1.f - decay.x ) / mDragDeceleration.x,
									 mVelocity.y * ( 1.f - decay.y ) / mDragDeceleration.y );
		const Vector2f unclampedPosition( mPosition + displacement );
		mPosition = clampScrollPosition( unclampedPosition, maxPosition );
		mTarget = mPosition;

		if ( mPosition.x != unclampedPosition.x )
			mVelocity.x = 0.f;
		else
			mVelocity.x *= decay.x;

		if ( mPosition.y != unclampedPosition.y )
			mVelocity.y = 0.f;
		else
			mVelocity.y *= decay.y;

		if ( isNearlyZero( mVelocity.x, 1.f ) )
			mVelocity.x = 0.f;
		if ( isNearlyZero( mVelocity.y, 1.f ) )
			mVelocity.y = 0.f;
		if ( mVelocity == Vector2f::Zero )
			mMode = Mode::Idle;
	}

	return oldPosition != mPosition;
}

void UIScrollController::stop() {
	mTarget = mPosition;
	mVelocity = Vector2f::Zero;
	mSmoothTweenCount = 0;
	mMode = Mode::Idle;
}

bool UIScrollController::isActive() const {
	return mMode != Mode::Idle;
}

bool UIScrollController::isDragging() const {
	return mMode == Mode::Dragging;
}

const Vector2f& UIScrollController::getPosition() const {
	return mPosition;
}

const Vector2f& UIScrollController::getTarget() const {
	return mTarget;
}

const Vector2f& UIScrollController::getVelocity() const {
	return mVelocity;
}

Time UIScrollController::getWheelScrollDuration() const {
	return mWheelScrollDuration;
}

void UIScrollController::setWheelScrollDuration( const Time& duration ) {
	mWheelScrollDuration = Seconds( eemax( 0.001f, static_cast<Float>( duration.asSeconds() ) ) );
}

Vector2f UIScrollController::getDragDeceleration() const {
	return mDragDeceleration;
}

void UIScrollController::setDragDeceleration( const Vector2f& deceleration ) {
	mDragDeceleration = { eemax( 0.01f, deceleration.x ), eemax( 0.01f, deceleration.y ) };
}

}} // namespace EE::UI
