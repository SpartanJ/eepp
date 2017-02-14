#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenu.hpp>

namespace EE { namespace UI {

UIMenuCheckBox::UIMenuCheckBox( UIMenuCheckBox::CreateParams& Params ) :
	UIMenuItem( Params ),
	mActive( false ),
	mSkinActive( NULL ),
	mSkinInactive( NULL )
{
	ApplyDefaultTheme();
}

UIMenuCheckBox::~UIMenuCheckBox() {
}

Uint32 UIMenuCheckBox::Type() const {
	return UI_TYPE_MENUCHECKBOX;
}

bool UIMenuCheckBox::IsType( const Uint32& type ) const {
	return UIMenuCheckBox::Type() == type ? true : UIMenuItem::IsType( type );
}

void UIMenuCheckBox::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "menuitem" );

	mSkinActive		= Theme->getByName( Theme->Abbr() + "_" + "menucheckbox_active" );
	mSkinInactive	= Theme->getByName( Theme->Abbr() + "_" + "menucheckbox_inactive" );

	Active( mActive );

	DoAfterSetTheme();
}

const bool& UIMenuCheckBox::Active() const {
	return mActive;
}

const bool& UIMenuCheckBox::IsActive() const {
	return Active();
}

void UIMenuCheckBox::Active( const bool& active ) {
	bool oActive = mActive;
	mActive = active;

	if ( mActive ) {
		if ( NULL != mSkinActive ) {
			if ( mSkinState->GetState() == UISkinState::StateSelected )
				Icon( mSkinActive->GetSubTexture( UISkinState::StateMouseEnter ) );
			else
				Icon( mSkinActive->GetSubTexture( UISkinState::StateNormal ) );
		} else
			mIcon->SubTexture( NULL );
	} else {
		if ( NULL != mSkinInactive )
			if ( mSkinState->GetState() == UISkinState::StateSelected )
				Icon( mSkinInactive->GetSubTexture( UISkinState::StateMouseEnter ) );
			else
				Icon( mSkinInactive->GetSubTexture( UISkinState::StateNormal ) );
		else
			mIcon->SubTexture( NULL );
	}

	if ( oActive != active ) {
		UIMenu * Menu = reinterpret_cast<UIMenu*> ( Parent() );

		if ( !Menu->CheckControlSize( this ) ) {
			if ( NULL != Icon()->SubTexture() ) {
				Padding( Recti( 0, 0, 0, 0 ) );
			}
		}

		OnValueChange();
	}
}

void UIMenuCheckBox::SwitchActive() {
	Active( !mActive );
}

Uint32 UIMenuCheckBox::OnMouseUp( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::OnMouseUp( Pos, Flags );

	if ( Parent()->Visible() && ( Flags & EE_BUTTONS_LRM ) )
		SwitchActive();

	return 1;
}

void UIMenuCheckBox::OnStateChange() {
	UIMenuItem::OnStateChange();

	Active( mActive );
}

bool UIMenuCheckBox::InheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

}}
