#include <eepp/ui/uistate.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

static const char * UIStatesNames[] = {
	"normal",
	"focus",
	"selected",
	"hover",
	"pressed",
	"selectedhover",
	"selectedpressed",
	"disabled"
};

static const Uint32 UIStateFlags[] = {
	UIState::StateFlagNormal,
	UIState::StateFlagFocus,
	UIState::StateFlagSelected,
	UIState::StateFlagHover,
	UIState::StateFlagPressed,
	UIState::StateFlagSelectedHover,
	UIState::StateFlagSelectedPressed,
	UIState::StateFlagDisabled
};

const char * UIState::getSkinStateName( const Uint32& State ) {
	return UIStatesNames[ State ];
}

int UIState::getStateNumber( const std::string& State ) {
	for ( int i = 0; i < UIState::StateCount; i++ ) {
		if ( State == UIStatesNames[i] ) {
			return UIStateFlags[i];
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
	mState(StateFlagNormal),
	mCurrentState(StateFlagNormal)
{
	eeASSERT( NULL != mSkin );
}

UIState::~UIState() {
}

void UIState::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha ) {
	if ( NULL != mSkin ) {
		mSkin->setState( mCurrentState );
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
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & UIStateFlags[i] ) == UIStateFlags[i] ) {
			if ( mSkin->hasDrawableState( UIStateFlags[i] ) ) {
				mCurrentState = UIStateFlags[i];
				return;
			}
		}
	}

	mCurrentState = StateFlagNormal;
}

}}

