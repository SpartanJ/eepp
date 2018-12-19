#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

UISkinState *UISkinState::New( UISkin * skin ) {
	return eeNew( UISkinState, (  skin ) );
}

UISkinState::UISkinState( UISkin * Skin ) :
	mSkin( Skin ),
	mState(1 << StateNormal),
	mCurrentState(StateNormal)
{
	eeASSERT( NULL != mSkin );
}

UISkinState::~UISkinState() {
}

void UISkinState::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha ) {
	if ( NULL != mSkin )
		mSkin->draw( X, Y, Width, Height, Alpha, mCurrentState );
}

const Uint32& UISkinState::getState() const {
	return mState;
}

void UISkinState::setState( const Uint32& State ) {
	if ( !( mState & ( 1 << State ) ) ) {
		mState |= ( 1 << State );

		updateState();
	}
}

void UISkinState::unsetState(const Uint32 & State) {
	if ( mState & ( 1 << State ) ) {
		mState &= ~( 1 << State );

		updateState();
	}
}

UISkin * UISkinState::getSkin() const {
	return mSkin;
}

bool UISkinState::stateExists( const Uint32& State ) {
	return mSkin->stateExists( State );
}

Uint32 UISkinState::getCurrentState() const {
	return mCurrentState;
}

void UISkinState::updateState() {
	for ( int i = StateCount - 1; i >= 0; i-- ) {
		if ( ( mState & ( 1 << i ) ) && stateExists( i ) ) {
			mCurrentState = i;
			return;
		}
	}

	mCurrentState = 0;
}

}}

