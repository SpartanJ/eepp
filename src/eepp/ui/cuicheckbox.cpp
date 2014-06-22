#include <eepp/ui/cuicheckbox.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/graphics/subtexture.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

cUICheckBox::cUICheckBox( const cUITextBox::CreateParams& Params ) :
	cUITextBox( Params ),
	mActive( false )
{
	cUIControlAnim::CreateParams ButtonParams( Params );

	ButtonParams.Parent( this );
	ButtonParams.PosSet( Vector2i( 0, 0 ) );
	ButtonParams.Size = Sizei( 16, 16 );

	mActiveButton 	= eeNew( cUIControlAnim, ( ButtonParams ) );
	mActiveButton->Visible( false );
	mActiveButton->Enabled( true );

	mInactiveButton = eeNew( cUIControlAnim, ( ButtonParams ) );
	mInactiveButton->Visible( true );
	mInactiveButton->Enabled( true );

	Padding( Recti(0,0,0,0) );

	ApplyDefaultTheme();
}

cUICheckBox::~cUICheckBox() {
}

Uint32 cUICheckBox::Type() const {
	return UI_TYPE_CHECKBOX;
}

bool cUICheckBox::IsType( const Uint32& type ) const {
	return cUICheckBox::Type() == type ? true : cUITextBox::IsType( type );
}

void cUICheckBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "checkbox" );

	mActiveButton->SetThemeControl	( Theme, "checkbox_active" );
	mInactiveButton->SetThemeControl( Theme, "checkbox_inactive" );

	DoAfterSetTheme();
}

void cUICheckBox::DoAfterSetTheme() {
	SubTexture * tSubTexture = NULL;
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

	Padding( Recti(0,0,0,0) );
}

void cUICheckBox::AutoSize() {
	cUITextBox::AutoSize();

	if ( mFlags & UI_AUTO_SIZE ) {
		mActiveButton->CenterVertical();
		mInactiveButton->CenterVertical();

		mSize.Width( (int)mTextCache->GetTextWidth() + mActiveButton->Size().Width() );
	}
}

void cUICheckBox::OnSizeChange() {
	cUITextBox::OnSizeChange();

	mActiveButton->CenterVertical();
	mInactiveButton->CenterVertical();
}

Uint32 cUICheckBox::OnMessage( const cUIMessage * Msg ) {
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

void cUICheckBox::Padding( const Recti& padding ) {
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
		if ( Sys::GetTicks() - mLastTick > 250 ) {
			mLastTick = Sys::GetTicks();

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
