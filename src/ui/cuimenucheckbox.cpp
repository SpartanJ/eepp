#include "cuimenucheckbox.hpp"
#include "cuimenu.hpp"

namespace EE { namespace UI {

cUIMenuCheckBox::cUIMenuCheckBox( cUIMenuCheckBox::CreateParams& Params ) :
	cUIMenuItem( Params ),
	mActive( false ),
	mSkinActive( NULL ),
	mSkinInactive( NULL )
{
	mType |= UI_TYPE_GET( UI_TYPE_MENUCHECKBOX );

	ApplyDefaultTheme();
}

cUIMenuCheckBox::~cUIMenuCheckBox() {
}

void cUIMenuCheckBox::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "menuitem" );

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
			if ( mSkinState->GetState() == cUISkinState::StateMouseEnter )
				Icon( mSkinActive->GetShape( cUISkinState::StateMouseEnter ) );
			else
				Icon( mSkinActive->GetShape( cUISkinState::StateNormal ) );
		} else
			mIcon->Shape( NULL );
	} else {
		if ( NULL != mSkinInactive )
			if ( mSkinState->GetState() == cUISkinState::StateMouseEnter )
				Icon( mSkinInactive->GetShape( cUISkinState::StateMouseEnter ) );
			else
				Icon( mSkinInactive->GetShape( cUISkinState::StateNormal ) );
		else
			mIcon->Shape( NULL );
	}

	if ( oActive != active ) {
		cUIMenu * Menu = reinterpret_cast<cUIMenu*> ( Parent() );

		if ( !Menu->CheckControlSize( this ) ) {
			if ( NULL != Icon()->Shape() ) {
				Padding( eeRecti( 0, 0, 0, 0 ) );
			}
		}

		OnValueChange();
	}
}

void cUIMenuCheckBox::SwitchActive() {
	Active( !mActive );
}

Uint32 cUIMenuCheckBox::OnMouseClick( const eeVector2i &Pos, Uint32 Flags ) {
	cUIPushButton::OnMouseClick( Pos, Flags );
	SwitchActive();

	return 1;
}

void cUIMenuCheckBox::OnStateChange() {
	cUIMenuItem::OnStateChange();

	Active( mActive );
}

}}
