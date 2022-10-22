#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uistate.hpp>

namespace EE { namespace UI {

static const char* UIStatesNames[] = { "normal",  "focus",		   "selected",		  "hover",
									   "pressed", "selectedhover", "selectedpressed", "disabled",
									   "checked", "focus-within" };

static const Uint32 UIStateFlags[] = {
	UIState::StateFlagNormal,	   UIState::StateFlagFocus,
	UIState::StateFlagSelected,	   UIState::StateFlagHover,
	UIState::StateFlagPressed,	   UIState::StateFlagSelectedHover,
	UIState::StateFlagFocusWithin, UIState::StateFlagSelectedPressed,
	UIState::StateFlagDisabled,	   UIState::StateFlagChecked };

const char* UIState::getStateName( const Uint32& State ) {
	return UIStatesNames[State];
}

int UIState::getStateNumber( const std::string& State ) {
	for ( int i = 0; i < UIState::StateCount; i++ ) {
		if ( State == UIStatesNames[i] ) {
			return UIStateFlags[i];
		}
	}

	return -1;
}

const char* UIState::getStateNameFromStateFlag( const Uint32& stateFlag ) {
	for ( int i = 0; i < UIState::StateCount; i++ ) {
		if ( stateFlag == UIStateFlags[i] ) {
			return UIStatesNames[i];
		}
	}

	return NULL;
}

const Uint32& UIState::getStateFlag( const Uint32& stateIndex ) {
	return UIStateFlags[stateIndex];
}

Uint32 UIState::getStateFlagFromName( const std::string& name ) {
	if ( name.empty() )
		return UIStateFlags[0];

	for ( size_t i = 0; i < eeARRAY_SIZE( UIStatesNames ); i++ ) {
		if ( UIStatesNames[i] == name )
			return UIStateFlags[i];
	}

	return eeINDEX_NOT_FOUND;
}

bool UIState::isStateName( const std::string& State ) {
	for ( int i = 0; i < UIState::StateCount; i++ ) {
		if ( State == UIStatesNames[i] ) {
			return true;
		}
	}

	return false;
}

UIState::UIState() :
	mState( StateFlagNormal ),
	mCurrentState( StateFlagNormal ),
	mPreviousState( StateFlagNormal ) {}

UIState::~UIState() {}

const Uint32& UIState::getState() const {
	return mState;
}

void UIState::setState( const Uint32& State ) {
	if ( mState != State ) {
		mState |= State;

		updateState();
	}
}

void UIState::pushState( const Uint32& State ) {
	if ( !( mState & ( 1 << State ) ) ) {
		mState |= ( 1 << State );

		updateState();
	}
}

void UIState::popState( const Uint32& State ) {
	if ( mState & ( 1 << State ) ) {
		mState &= ~( 1 << State );

		updateState();
	}
}

const Uint32& UIState::getCurrentState() const {
	return mCurrentState;
}

const Uint32& UIState::getPreviousState() const {
	return mPreviousState;
}

void UIState::onStateChange() {}

}} // namespace EE::UI
