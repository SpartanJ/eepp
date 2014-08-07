#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIPopUpMenu::UIPopUpMenu( UIPopUpMenu::CreateParams Params ) :
	UIMenu( Params )
{
	ApplyDefaultTheme();
}

UIPopUpMenu::~UIPopUpMenu() {
	OnClose();
}

Uint32 UIPopUpMenu::Type() const {
	return UI_TYPE_POPUPMENU;
}

bool UIPopUpMenu::IsType( const Uint32& type ) const {
	return UIPopUpMenu::Type() == type ? true : UIMenu::IsType( type );
}

void UIPopUpMenu::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "popupmenu" );
	DoAfterSetTheme();
}

bool UIPopUpMenu::Show() {
	if ( !Visible() || 0.f == mAlpha ) {
		#ifdef EE_PLATFORM_TOUCH
		mTE.Restart();
		#endif

		Enabled( true );
		Visible( true );

		ToFront();

		if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
			StartAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->ControlsFadeInTime() );
		}

		SetFocus();

		return true;
	}

	return false;
}

bool UIPopUpMenu::Hide() {
	if ( Visible() ) {
		if ( !FadingOut() ) {
			if ( NULL != mItemSelected )
				mItemSelected->SetSkinState( UISkinState::StateNormal );

			mItemSelected		= NULL;
			mItemSelectedIndex	= eeINDEX_NOT_FOUND;

			if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
				DisableFadeOut( UIThemeManager::instance()->ControlsFadeOutTime() );
			} else {
				Enabled( false );
				Visible( false );
			}
		}

		return true;
	}

	return false;
}

void UIPopUpMenu::OnComplexControlFocusLoss() {
	Hide();

	UIMenu::OnComplexControlFocusLoss();
}

Uint32 UIPopUpMenu::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgMouseUp:
		{
			#ifdef EE_PLATFORM_TOUCH
			if ( mTE.Elapsed().AsMilliseconds() > 250.f ) {
			#endif
				if ( !Msg->Sender()->IsType( UI_TYPE_MENUSUBMENU ) && ( Msg->Flags() & EE_BUTTONS_LRM ) ) {
					SendCommonEvent( UIEvent::EventOnHideByClick );

					if ( Visible() )
						UIManager::instance()->MainControl()->SetFocus();

					Hide();
				}
			#ifdef EE_PLATFORM_TOUCH
			}
			#endif
		}
	}

	return UIMenu::OnMessage( Msg );
}

}}
