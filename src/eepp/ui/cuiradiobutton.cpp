#include <eepp/ui/cuiradiobutton.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/graphics/csubtexture.hpp>
#include <eepp/graphics/ctextcache.hpp>

namespace EE { namespace UI {

cUIRadioButton::cUIRadioButton( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params ),
	mActiveButton(NULL),
	mInactiveButton(NULL),
	mActive( false )
{
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

Uint32 cUIRadioButton::Type() const {
	return UI_TYPE_RADIOBUTTON;
}

bool cUIRadioButton::IsType( const Uint32& type ) const {
	return cUIRadioButton::Type() == type ? true : cUITextBox::IsType( type );
}

void cUIRadioButton::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "radiobutton" );

	mActiveButton->SetThemeControl	( Theme, "radiobutton_active" );
	mInactiveButton->SetThemeControl( Theme, "radiobutton_inactive" );

	cSubTexture * tSubTexture = NULL;
	cUISkin * tSkin = mActiveButton->GetSkin();

	if ( tSkin ) {
		tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mActiveButton->Size( tSubTexture->RealSize() );
			mActiveButton->CenterVertical();
		}
	}

	tSkin = mInactiveButton->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mInactiveButton->Size( tSubTexture->RealSize() );
			mInactiveButton->CenterVertical();
		}
	}

	Padding( eeRecti(0,0,0,0) );
}

void cUIRadioButton::AutoSize() {
	cUITextBox::AutoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->CenterVertical();
		mInactiveButton->CenterVertical();

		mSize.Width( (int)mTextCache->GetTextWidth() + mActiveButton->Size().Width() );
	}
}

void cUIRadioButton::OnSizeChange() {
	cUITextBox::OnSizeChange();

	mActiveButton->CenterVertical();
	mInactiveButton->CenterVertical();
}

Uint32 cUIRadioButton::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick: {
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				SwitchState();
			}

			if ( Msg->Sender() == mActiveButton || Msg->Sender() == mInactiveButton ) {
				SendMouseEvent( cUIEvent::EventMouseClick, cUIManager::instance()->GetMousePos(), cUIManager::instance()->PressTrigger() );
			}

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
	if ( Event.KeyCode() == KEY_SPACE ) {
		if ( Sys::GetTicks() - mLastTick > 250 ) {
			mLastTick = Sys::GetTicks();

			Active( true );
		}
	}

	return cUITextBox::OnKeyDown( Event );
}

void cUIRadioButton::OnAlphaChange() {
	cUITextBox::OnAlphaChange();
	
	mActiveButton->Alpha( mAlpha );
	mInactiveButton->Alpha( mAlpha );
}

}}
