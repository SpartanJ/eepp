#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UIRadioButton::UIRadioButton( const UITextBox::CreateParams& Params ) :
	UITextBox( Params ),
	mActiveButton(NULL),
	mInactiveButton(NULL),
	mActive( false )
{
	UIControlAnim::CreateParams ButtonParams( Params );

	ButtonParams.Parent( this );
	ButtonParams.PosSet( Vector2i( 0, 0 ) );
	ButtonParams.Size = Sizei( 16, 16 );

	mActiveButton 	= eeNew( UIControlAnim, ( ButtonParams ) );
	mActiveButton->Visible( false );
	mActiveButton->Enabled( true );

	mInactiveButton = eeNew( UIControlAnim, ( ButtonParams ) );
	mInactiveButton->Visible( true );
	mInactiveButton->Enabled( true );

	Padding( Recti(0,0,0,0) );

	AutoActivate();

	ApplyDefaultTheme();
}

UIRadioButton::~UIRadioButton() {
}

Uint32 UIRadioButton::Type() const {
	return UI_TYPE_RADIOBUTTON;
}

bool UIRadioButton::IsType( const Uint32& type ) const {
	return UIRadioButton::Type() == type ? true : UITextBox::IsType( type );
}

void UIRadioButton::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "radiobutton" );

	mActiveButton->SetThemeControl	( Theme, "radiobutton_active" );
	mInactiveButton->SetThemeControl( Theme, "radiobutton_inactive" );

	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = mActiveButton->GetSkin();

	if ( tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mActiveButton->Size( tSubTexture->RealSize() );
			mActiveButton->CenterVertical();
		}
	}

	tSkin = mInactiveButton->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mInactiveButton->Size( tSubTexture->RealSize() );
			mInactiveButton->CenterVertical();
		}
	}

	Padding( Recti(0,0,0,0) );
}

void UIRadioButton::AutoSize() {
	UITextBox::AutoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->CenterVertical();
		mInactiveButton->CenterVertical();

		mSize.width( (int)mTextCache->GetTextWidth() + mActiveButton->Size().width() );
	}
}

void UIRadioButton::OnSizeChange() {
	UITextBox::OnSizeChange();

	mActiveButton->CenterVertical();
	mInactiveButton->CenterVertical();
}

Uint32 UIRadioButton::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgClick: {
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				SwitchState();
			}

			if ( Msg->Sender() == mActiveButton || Msg->Sender() == mInactiveButton ) {
				SendMouseEvent( UIEvent::EventMouseClick, UIManager::instance()->GetMousePos(), UIManager::instance()->PressTrigger() );
			}

			return 1;
		}
	}

	return 0;
}

void UIRadioButton::SwitchState() {
	Active( !mActive );
}

void UIRadioButton::Active( const bool& active ) {
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
		UIControl * tChild = mParentCtrl->ChildGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->IsType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->Active() )
						tRB->Active( false );
				}
			}

			tChild = tChild->NextGet();
		}
	}
}

bool UIRadioButton::CheckActives() {
	if ( NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->ChildGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->IsType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

					if ( tRB->Active() )
						return true;
				}
			}

			tChild = tChild->NextGet();
		}
	}

	return false;
}

void UIRadioButton::AutoActivate() {
	eeASSERT( NULL != mParentCtrl );

	if ( NULL != mParentCtrl ) {
		UIControl * tChild = mParentCtrl->ChildGetFirst();

		while ( NULL != tChild ) {
			if ( tChild->IsType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton * tRB = reinterpret_cast<UIRadioButton*> ( tChild );

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

const bool& UIRadioButton::Active() const {
	return mActive;
}

const bool& UIRadioButton::IsActive() const {
	return Active();
}

void UIRadioButton::Padding( const Recti& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->Size().width();
}

UIControlAnim * UIRadioButton::ActiveButton() const {
	return mActiveButton;
}

UIControlAnim * UIRadioButton::InactiveButton() const {
	return mInactiveButton;
}

Uint32 UIRadioButton::OnKeyDown( const UIEventKey& Event ) {
	if ( Event.KeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			Active( true );
		}
	}

	return UITextBox::OnKeyDown( Event );
}

void UIRadioButton::OnAlphaChange() {
	UITextBox::OnAlphaChange();
	
	mActiveButton->Alpha( mAlpha );
	mInactiveButton->Alpha( mAlpha );
}

}}
