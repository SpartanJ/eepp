#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

UISkinState * UISkinState::New( UISkin * skin ) {
	return eeNew( UISkinState, (  skin ) );
}

UISkinState::UISkinState( UISkin * Skin ) :
	mSkin( Skin )
{
	eeASSERT( NULL != mSkin );
}

UISkinState::~UISkinState() {
}


UISkin * UISkinState::getSkin() const {
	return mSkin;
}

bool UISkinState::stateExists( const Uint32& State ) const {
	return mSkin->hasDrawableState( State );
}

void UISkinState::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha ) {
	if ( NULL != mSkin ) {
		mSkin->setState( mCurrentState );
		mSkin->setAlpha( Alpha );
		mSkin->draw( Vector2f( X, Y ), Sizef( Width, Height ) );
	}
}

void UISkinState::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag(i) ) == getStateFlag(i) ) {
			if ( stateExists( getStateFlag(i) ) ) {
				mCurrentState = getStateFlag(i);
				onStateChange();
				return;
			}
		}
	}

	Uint32 currentState = mCurrentState;

	mCurrentState = StateFlagNormal;

	if ( currentState != StateFlagNormal ) {
		onStateChange();
	}
}

}}
