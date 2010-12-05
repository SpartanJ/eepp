#include "cuiradiobutton.hpp"

namespace EE { namespace UI {

cUIRadioButton::cUIRadioButton( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params ),
	mActiveButton(NULL),
	mInactiveButton(NULL),
	mActive( false )
{
	mType |= UI_TYPE_GET(UI_TYPE_RADIOBUTTON);

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

	AutoActivate();

	ApplyDefaultTheme();
}

cUIRadioButton::~cUIRadioButton() {
}

void cUIRadioButton::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "radiobutton" );

	mActiveButton->ForceThemeSkin	( Theme, "radiobutton_active" );
	mInactiveButton->ForceThemeSkin	( Theme, "radiobutton_inactive" );

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

void cUIRadioButton::OnSizeChange() {
	cUITextBox::OnSizeChange();

	mActiveButton->CenterVertical();
	mInactiveButton->CenterVertical();
}

Uint32 cUIRadioButton::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick: {
			if ( Msg->Flags() & EE_BUTTON_LMASK )
				SwitchState();

			return 1;
		}
	}

	return 0;
}

void cUIRadioButton::SwitchState() {
	Active( !mActive );
}

void cUIRadioButton::Active( const bool& active ) {
	if ( !active ) {
		if ( CheckActives() ) {
			mActiveButton->Visible( false );
			mInactiveButton->Visible( true );

			mActive = false;

			OnValueChange();
		}
	} else {
		mActiveButton->Visible( true );
		mInactiveButton->Visible( false );

		mActive = true;

		OnValueChange();
	}

	if ( active && NULL != mParentCtrl ) {
		cUIControl * tChild = mParentCtrl->ChildGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->IsType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					cUIRadioButton * tRB = reinterpret_cast<cUIRadioButton*> ( tChild );

					if ( tRB->Active() )
						tRB->Active( false );
				}
			}

			tChild = tChild->NextGet();
		}
	}
}

bool cUIRadioButton::CheckActives() {
	if ( NULL != mParentCtrl ) {
		cUIControl * tChild = mParentCtrl->ChildGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->IsType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					cUIRadioButton * tRB = reinterpret_cast<cUIRadioButton*> ( tChild );

					if ( tRB->Active() )
						return true;
				}
			}

			tChild = tChild->NextGet();
		}
	}

	return false;
}

void cUIRadioButton::AutoActivate() {
	eeASSERT( NULL != mParentCtrl );

	if ( NULL != mParentCtrl ) {
		cUIControl * tChild = mParentCtrl->ChildGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->IsType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					cUIRadioButton * tRB = reinterpret_cast<cUIRadioButton*> ( tChild );

					if ( tRB->Active() ) {
						return;
					}
				}
			}

			tChild = tChild->NextGet();
		}
	}

	Active( true );
}

const bool& cUIRadioButton::Active() const {
	return mActive;
}

const bool& cUIRadioButton::IsActive() const {
	return Active();
}

void cUIRadioButton::Padding( const eeRecti& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->Size().Width();
}

cUIControlAnim * cUIRadioButton::ActiveButton() const {
	return mActiveButton;
}

cUIControlAnim * cUIRadioButton::InactiveButton() const {
	return mInactiveButton;
}

Uint32 cUIRadioButton::OnKeyDown( const cUIEventKey& Event ) {
	cUITextBox::OnKeyDown( Event );

	if ( Event.KeyCode() == KEY_SPACE ) {
		if ( eeGetTicks() - mLastTick > 250 ) {
			mLastTick = eeGetTicks();

			Active( true );
		}
	}

	return 1;
}

void cUIRadioButton::OnAlphaChange() {
	cUITextBox::OnAlphaChange();
	
	mActiveButton->Alpha( mAlpha );
	mInactiveButton->Alpha( mAlpha );
}

}}
