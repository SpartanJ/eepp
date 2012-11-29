#include <eepp/ui/cuimenucheckbox.hpp>
#include <eepp/ui/cuimenu.hpp>

namespace EE { namespace UI {

cUIMenuCheckBox::cUIMenuCheckBox( cUIMenuCheckBox::CreateParams& Params ) :
	cUIMenuItem( Params ),
	mActive( false ),
	mSkinActive( NULL ),
	mSkinInactive( NULL )
{
	ApplyDefaultTheme();
}

cUIMenuCheckBox::~cUIMenuCheckBox() {
}

Uint32 cUIMenuCheckBox::Type() const {
	return UI_TYPE_MENUCHECKBOX;
}

bool cUIMenuCheckBox::IsType( const Uint32& type ) const {
	return cUIMenuCheckBox::Type() == type ? true : cUIMenuItem::IsType( type );
}

void cUIMenuCheckBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "menuitem" );

	mSkinActive		= Theme->GetByName( Theme->Abbr() + "_" + "menucheckbox_active" );
	mSkinInactive	= Theme->GetByName( Theme->Abbr() + "_" + "menucheckbox_inactive" );

	Active( mActive );

	DoAfterSetTheme();
}

const bool& cUIMenuCheckBox::Active() const {
	return mActive;
}

const bool& cUIMenuCheckBox::IsActive() const {
	return Active();
}

void cUIMenuCheckBox::Active( const bool& active ) {
	bool oActive = mActive;
	mActive = active;

	if ( mActive ) {
		if ( NULL != mSkinActive ) {
			if ( mSkinState->GetState() == cUISkinState::StateSelected )
				Icon( mSkinActive->GetSubTexture( cUISkinState::StateMouseEnter ) );
			else
				Icon( mSkinActive->GetSubTexture( cUISkinState::StateNormal ) );
		} else
			mIcon->SubTexture( NULL );
	} else {
		if ( NULL != mSkinInactive )
			if ( mSkinState->GetState() == cUISkinState::StateSelected )
				Icon( mSkinInactive->GetSubTexture( cUISkinState::StateMouseEnter ) );
			else
				Icon( mSkinInactive->GetSubTexture( cUISkinState::StateNormal ) );
		else
			mIcon->SubTexture( NULL );
	}

	if ( oActive != active ) {
		cUIMenu * Menu = reinterpret_cast<cUIMenu*> ( Parent() );

		if ( !Menu->CheckControlSize( this ) ) {
			if ( NULL != Icon()->SubTexture() ) {
				Padding( eeRecti( 0, 0, 0, 0 ) );
			}
		}

		OnValueChange();
	}
}

void cUIMenuCheckBox::SwitchActive() {
	Active( !mActive );
}

Uint32 cUIMenuCheckBox::OnMouseUp( const eeVector2i &Pos, const Uint32 Flags ) {
	cUIMenuItem::OnMouseUp( Pos, Flags );

	if ( Parent()->Visible() && ( Flags & EE_BUTTONS_LRM ) )
		SwitchActive();

	return 1;
}

void cUIMenuCheckBox::OnStateChange() {
	cUIMenuItem::OnStateChange();

	Active( mActive );
}

bool cUIMenuCheckBox::InheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

}}
