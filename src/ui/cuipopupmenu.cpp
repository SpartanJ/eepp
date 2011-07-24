#include "cuipopupmenu.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIPopUpMenu::cUIPopUpMenu( cUIPopUpMenu::CreateParams Params ) :
	cUIMenu( Params )
{
	mType = UI_TYPE_POPUPMENU;

	ApplyDefaultTheme();
}

cUIPopUpMenu::~cUIPopUpMenu() {
}

void cUIPopUpMenu::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "popupmenu" );
	DoAfterSetTheme();
}

bool cUIPopUpMenu::Show() {
	if ( !Visible() || 0.f == mAlpha ) {
		Enabled( true );
		Visible( true );

		ToFront();

		if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
			if ( 255.f == Alpha() )
				StartAlphaAnim( 0.f, 255.f, cUIThemeManager::instance()->ControlsFadeInTime() );
			else
				CreateFadeIn( cUIThemeManager::instance()->ControlsFadeInTime() );
		}

		SetFocus();

		return true;
	}

	return false;
}

bool cUIPopUpMenu::Hide() {
	if ( Visible() ) {
		if ( !FadingOut() ) {
			if ( NULL != mItemSelected )
				mItemSelected->SetSkinState( cUISkinState::StateNormal );

			mItemSelected		= NULL;
			mItemSelectedIndex	= eeINDEX_NOT_FOUND;

			if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
				DisableFadeOut( cUIThemeManager::instance()->ControlsFadeOutTime() );
			} else {
				Enabled( false );
				Visible( false );
			}
		}

		return true;
	}

	return false;
}

void cUIPopUpMenu::OnComplexControlFocusLoss() {
	Hide();

	cUIMenu::OnComplexControlFocusLoss();
}

Uint32 cUIPopUpMenu::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgMouseUp:
		{
			if ( !Msg->Sender()->IsTypeOrInheritsFrom( UI_TYPE_MENUSUBMENU ) && ( Msg->Flags() & EE_BUTTONS_LRM ) ) {
				SendCommonEvent( cUIEvent::EventOnHideByClick );

				cUIManager::instance()->MainControl()->SetFocus();

				Hide();
			}
		}
	}

	return cUIMenu::OnMessage( Msg );
}

}}
