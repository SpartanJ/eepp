#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UICheckBox::UICheckBox( const UITextBox::CreateParams& Params ) :
	UITextBox( Params ),
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

	ApplyDefaultTheme();
}

UICheckBox::~UICheckBox() {
}

Uint32 UICheckBox::Type() const {
	return UI_TYPE_CHECKBOX;
}

bool UICheckBox::IsType( const Uint32& type ) const {
	return UICheckBox::Type() == type ? true : UITextBox::IsType( type );
}

void UICheckBox::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "checkbox" );

	mActiveButton->SetThemeControl	( Theme, "checkbox_active" );
	mInactiveButton->SetThemeControl( Theme, "checkbox_inactive" );

	DoAfterSetTheme();
}

void UICheckBox::DoAfterSetTheme() {
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

void UICheckBox::AutoSize() {
	UITextBox::AutoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->CenterVertical();
		mInactiveButton->CenterVertical();

		mSize.Width( (int)mTextCache->GetTextWidth() + mActiveButton->Size().Width() );
	}
}

void UICheckBox::OnSizeChange() {
	UITextBox::OnSizeChange();

	mActiveButton->CenterVertical();
	mInactiveButton->CenterVertical();
}

Uint32 UICheckBox::OnMessage( const UIMessage * Msg ) {
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

void UICheckBox::SwitchState() {
	Active( !mActive );
}

void UICheckBox::Active( const bool& active ) {
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

const bool& UICheckBox::Active() const {
	return mActive;
}

const bool& UICheckBox::IsActive() const {
	return Active();
}

void UICheckBox::Padding( const Recti& padding ) {
	mPadding = padding;
	mPadding.Left = mPadding.Left + mActiveButton->Size().Width();
}

UIControlAnim * UICheckBox::ActiveButton() const {
	return mActiveButton;
}

UIControlAnim * UICheckBox::InactiveButton() const {
	return mInactiveButton;
}

Uint32 UICheckBox::OnKeyDown( const UIEventKey& Event ) {
	UITextBox::OnKeyDown( Event );

	if ( Event.KeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			Active( !mActive );
		}
	}

	return 1;
}

void UICheckBox::OnAlphaChange() {
	UITextBox::OnAlphaChange();
	
	mActiveButton->Alpha( mAlpha );
	mInactiveButton->Alpha( mAlpha );
}

}}
