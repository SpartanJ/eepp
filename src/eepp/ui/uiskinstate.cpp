#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

UISkinState *UISkinState::New( UISkin * skin ) {
	return eeNew( UISkinState, (  skin ) );
}

UISkinState::UISkinState( UISkin * Skin ) :
	mSkin( Skin ),
	mCurState(0),
	mLastState(0)
{
	eeASSERT( NULL != mSkin );
}

UISkinState::~UISkinState() {
}

void UISkinState::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha ) {
	if ( NULL != mSkin )
		mSkin->draw( X, Y, Width, Height, Alpha, mCurState );
}

void UISkinState::stateBack( const Uint32& State ) {
	if ( ( StateFocus == State ) && StateMouseEnter == mCurState && StateNormal == mLastState ) {
		return;
	}

	if ( mCurState == StateSelected && ( State == StateMouseDown || State == StateFocus ) ) {
		return;
	}

	if ( !( mCurState == StateFocus && ( State == StateMouseEnter || State == StateMouseExit || State == StateMouseDown ) ) ) {
		mLastState 	= mCurState;
		mCurState 	= StateNormal;
	}
}

void UISkinState::setPrevState() {
	if ( StateMouseDown == mCurState ) {
		Uint32 State = mCurState;
		mCurState = mLastState;
		mLastState = State;
	}
}

const Uint32& UISkinState::getPrevState() const {
	return mLastState;
}

const Uint32& UISkinState::getState() const {
	return mCurState;
}

void UISkinState::setState( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	switch ( mSkin->getType() )
	{
		case UISkin::SkinSimple:
		case UISkin::SkinComplex:
			setStateTypeSimple( State );
			break;
		default:
			setStateTypeDefault( State );
	}
}

void UISkinState::setStateTypeSimple( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	if ( mCurState == State )
		return;

	if ( !mSkin->getColorDefault( State ) || stateExists( State ) ) {
		mSkin->stateNormalToState( State );

		mLastState	= mCurState;
		mCurState	= State;
	} else
		stateBack( State );
}

void UISkinState::setStateTypeDefault( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	if ( !mSkin->getColorDefault( State ) )
		mCurState = State;
	else
		stateBack( State );
}

UISkin * UISkinState::getSkin() const {
	return mSkin;
}

bool UISkinState::stateExists( const Uint32& State ) {
	return mSkin->stateExists( State );
}

}}

