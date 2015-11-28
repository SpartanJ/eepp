#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uiskin.hpp>

namespace EE { namespace UI {

UISkinState::UISkinState( UISkin * Skin ) :
	mSkin( Skin ),
	mCurState(0),
	mLastState(0)
{
	eeASSERT( NULL != mSkin );
}

UISkinState::~UISkinState() {
}

void UISkinState::Draw( const Float& X, const Float& Y, const Float& Width, const Float& Height, const Uint32& Alpha ) {
	if ( NULL != mSkin )
		mSkin->Draw( X, Y, Width, Height, Alpha, mCurState );
}

void UISkinState::StateBack( const Uint32& State ) {
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

void UISkinState::SetPrevState() {
	if ( StateMouseDown == mCurState ) {
		Uint32 State = mCurState;
		mCurState = mLastState;
		mLastState = State;
	}
}

const Uint32& UISkinState::GetPrevState() const {
	return mLastState;
}

const Uint32& UISkinState::GetState() const {
	return mCurState;
}

void UISkinState::SetState( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	switch ( mSkin->GetType() )
	{
		case UISkin::SkinSimple:
		case UISkin::SkinComplex:
			SetStateTypeSimple( State );
			break;
		default:
			SetStateTypeDefault( State );
	}
}

void UISkinState::SetStateTypeSimple( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	if ( mCurState == State )
		return;

	if ( !mSkin->GetColorDefault( State ) || NULL != mSkin->GetSubTexture( State ) ) {
		mSkin->StateNormalToState( State );

		mLastState	= mCurState;
		mCurState	= State;
	} else
		StateBack( State );
}

void UISkinState::SetStateTypeDefault( const Uint32& State ) {
	eeASSERT ( State < UISkinState::StateCount );

	if ( !mSkin->GetColorDefault( State ) )
		mCurState = State;
	else
		StateBack( State );
}

UISkin * UISkinState::GetSkin() const {
	return mSkin;
}

bool UISkinState::StateExists( const Uint32& State ) {
	switch ( mSkin->GetType() )
	{
		case UISkin::SkinSimple:
		case UISkin::SkinComplex:
			return NULL != mSkin->GetSubTexture( State );
	}

	return true;
}

}}

