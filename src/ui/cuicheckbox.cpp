#include "cuicheckbox.hpp"

namespace EE { namespace UI {

cUICheckBox::cUICheckBox( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params ),
	mActive( false )
{
	mType |= UI_TYPE_GET(UI_TYPE_CHECKBOX);

	cUIControlAnim::CreateParams ButtonParams( Params );

	ButtonParams.Parent( this );
	ButtonParams.PosSet( eeVector2i( 0, 0 ) );
	ButtonParams.Size = eeSize( 16, 16 );

	mActiveButton 	= eeNew( cUIControlAnim, ( ButtonParams ) );
	mActiveButton->Visible( false );
	mActiveButton->Enabled( true );

	mInactiveButton = eeNew( cUIControlAnim, ( ButtonParams ) );
	mInactiveButton->Visible( true );
	mInactiveButton->Enabled( true );

	Padding( eeRecti(0,0,0,0) );

	ApplyDefaultTheme();
}

cUICheckBox::~cUICheckBox() {
}

void cUICheckBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "checkbox" );

	mActiveButton->ForceThemeSkin	( Theme, "checkbox_active" );
	mInactiveButton->ForceThemeSkin	( Theme, "checkbox_inactive" );

	cShape * tShape = NULL;
	cUISkin * tSkin = mActiveButton->GetSkin();

	if ( tSkin ) {
		tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			mActiveButton->Size( tShape->RealSize() );
			mActiveButton->CenterVertical();
		}
	}

	tSkin = mInactiveButton->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			mInactiveButton->Size( tShape->RealSize() );
			mInactiveButton->CenterVertical();
		}
	}

	Padding( eeRecti(0,0,0,0) );
}

void cUICheckBox::OnSizeChange() {
	cUITextBox::OnSizeChange();

	mActiveButton->CenterVertical();
	mInactiveButton->CenterVertical();
}

Uint32 cUICheckBox::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick: {
			if ( Msg->Flags() & EE_BUTTON_LMASK )
				SwitchState();

			return 1;
		}
	}

	return 0;
}

void cUICheckBox::SwitchState() {
	Active( !mActive );
}

void cUICheckBox::Active( const bool& active ) {
	if ( !active ) {
		mActiveButton->Visible( false );
		mInactiveButton->Visible( true );

		mActive = false;
	} else {
		mActiveButton->Visible( true );
		mInactiveButton->Visible( false );

		mActive = true;
	}

	OnValueChange();
}

const bool& cUICheckBox::Active() const {
	return mActive;
}

const bool& cUICheckBox::IsActive() const {
	return Active();
}

void cUICheckBox::Padding( const eeRecti& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->Size().Width();
}

cUIControlAnim * cUICheckBox::ActiveButton() const {
	return mActiveButton;
}

cUIControlAnim * cUICheckBox::InactiveButton() const {
	return mInactiveButton;
}

Uint32 cUICheckBox::OnKeyDown( const cUIEventKey& Event ) {
	cUITextBox::OnKeyDown( Event );

	if ( Event.KeyCode() == KEY_SPACE ) {
		if ( eeGetTicks() - mLastTick > 250 ) {
			mLastTick = eeGetTicks();

			Active( !mActive );
		}
	}

	return 1;
}

void cUICheckBox::OnAlphaChange() {
	cUITextBox::OnAlphaChange();
	
	mActiveButton->Alpha( mAlpha );
	mInactiveButton->Alpha( mAlpha );
}

}}
