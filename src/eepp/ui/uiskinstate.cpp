#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>

namespace EE { namespace UI {

UISkinState* UISkinState::New( UISkin* skin ) {
	return eeNew( UISkinState, ( skin ) );
}

UISkinState::UISkinState( UISkin* Skin ) : mSkin( Skin ), mCurrentColor( Color::White ) {
	eeASSERT( NULL != mSkin );
}

UISkinState::~UISkinState() {}

UISkin* UISkinState::getSkin() const {
	return mSkin;
}

bool UISkinState::stateExists( const Uint32& State ) const {
	return mSkin->hasDrawableState( State );
}

void UISkinState::setStateColor( const Uint32& state, const Color& color ) {
	mColors[state] = color;

	if ( mCurrentState == state )
		mCurrentColor = color;
}

Color UISkinState::getStateColor( const Uint32& state ) const {
	auto it = mColors.find( state );

	if ( it != mColors.end() )
		return it->second;

	return mSkin->getColor();
}

bool UISkinState::hasStateColor( const Uint32& state ) const {
	return mColors.find( state ) != mColors.end();
}

void UISkinState::draw( const Float& X, const Float& Y, const Float& Width, const Float& Height,
						const Uint32& Alpha ) {
	if ( NULL != mSkin ) {
		Color color = mSkin->getColor();
		mSkin->setState( mCurrentState );
		mSkin->setColor( mCurrentColor );

		if ( Alpha != 255 )
			mSkin->setAlpha( Alpha * mCurrentColor.a / 255 );

		mSkin->draw( Vector2f( X, Y ), Sizef( Width, Height ) );
		mSkin->setColor( color );
	}
}

void UISkinState::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag( i ) ) == getStateFlag( i ) ) {
			if ( stateExists( getStateFlag( i ) ) ) {
				mPreviousState = mCurrentState;
				mCurrentState = getStateFlag( i );
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

void UISkinState::onStateChange() {
	mCurrentColor = getStateColor( mCurrentState );
}

}} // namespace EE::UI
