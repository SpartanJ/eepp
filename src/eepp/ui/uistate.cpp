#include <eepp/ui/uistate.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

static const char * UIStatesNames[] = {
	"normal",
	"focus",
	"selected",
	"hover",
	"pressed",
	"disabled"
};

const char * UIState::getSkinStateName( const Uint32& State ) {
	return UIStatesNames[ State ];
}

int UIState::getStateNumber( const std::string& State ) {
	for ( int i = 0; i < UIState::StateCount; i++ ) {
		if ( State == UIStatesNames[i] ) {
			return i;
		}
	}

	return -1;
}

bool UIState::isStateName( const std::string& State ) {
	for ( int i = 0; i < UIState::StateCount; i++ ) {
		if ( State == UIStatesNames[i] ) {
			return true;
		}
	}

	return false;
}

UIState * UIState::New( UISkin * skin ) {
	return eeNew( UIState, (  skin ) );
}

UIState::UIState( UISkin * Skin ) :
	mSkin( Skin ),
	mState(1 << StateNormal),
	mCurrentState(StateNormal)
{
	eeASSERT( NULL != mSkin );
}

UIState::~UIState() {
}

void UIState::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha ) {
	if ( NULL != mSkin ) {
		mSkin->setState( 1 << mCurrentState );
		mSkin->setAlpha( Alpha );
		mSkin->draw( Vector2f( X, Y ), Sizef( Width, Height ) );
	}
}

const Uint32& UIState::getState() const {
	return mState;
}

void UIState::setState( const Uint32& State ) {
	if ( mState != State ) {
		mState |= State;

		updateState();
	}
}

void UIState::pushState(const Uint32 & State) {
	if ( !( mState & ( 1 << State ) ) ) {
		mState |= ( 1 << State );

		updateState();
	}
}

void UIState::popState(const Uint32 & State) {
	if ( mState & ( 1 << State ) ) {
		mState &= ~( 1 << State );

		updateState();
	}
}

UISkin * UIState::getSkin() const {
	return mSkin;
}

bool UIState::stateExists( const Uint32& State ) {
	return mSkin->hasDrawableState( 1 << State );
}

Uint32 UIState::getCurrentState() const {
	return mCurrentState;
}

void UIState::updateState() {
	for ( int i = StateCount - 1; i >= 0; i-- ) {
		if ( ( mState & ( 1 << i ) ) && stateExists( i ) ) {
			mCurrentState = i;
			return;
		}
	}

	mCurrentState = 0;
}

}}

