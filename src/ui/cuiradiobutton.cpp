#include "cuiradiobutton.hpp"

namespace EE { namespace UI {

cUIRadioButton::cUIRadioButton( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params ),
	mActiveButton(NULL),
	mInactiveButton(NULL),
	mActive( false )
{
	mType |= UI_TYPE_GET(UI_TYPE_RADIOBUTTON);

	cUITextBox::CreateParams ButtonParams( Params );

	ButtonParams.Parent( this );
	ButtonParams.PosSet( eeVector2i( 0, 0 ) );
	ButtonParams.Size = eeSize( 16, 16 );

	mActiveButton 	= eeNew( cUIPushButton, ( ButtonParams ) );
	mActiveButton->Visible( false );
	mActiveButton->Enabled( true );

	mInactiveButton = eeNew( cUIPushButton, ( ButtonParams ) );
	mInactiveButton->Visible( true );
	mInactiveButton->Enabled( true );
	
	Padding( eeRectf(0,0,0,0) );

	AutoActivate();
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
		tShape = tSkin->GetShape( cUISkin::StateNormal );

		if ( NULL != tShape ) {
			mActiveButton->Size( tShape->RealSize() );
			mActiveButton->CenterVertical();
		}
	}

	tSkin = mInactiveButton->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkin::StateNormal );

		if ( NULL != tShape ) {
			mInactiveButton->Size( tShape->RealSize() );
			mInactiveButton->CenterVertical();
		}
	}

	Padding( eeRectf(0,0,0,0) );
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
	return IsActive();
}

void cUIRadioButton::Padding( const eeRectf& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->Size().Width();
}

}}
