#include "cuimenusubmenu.hpp"
#include "cuimenu.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIMenuSubMenu::cUIMenuSubMenu( cUIMenuSubMenu::CreateParams& Params ) :
	cUIMenuItem( Params ),
	mSubMenu( NULL ),
	mSkinArrow( NULL ),
	mArrow( NULL ),
	mTimeOver( 0.f ),
	mMaxTime( Params.MouseOverTimeShowMenu ),
	mCbId( 0 ),
	mCbId2( 0 )
{
	mType |= UI_TYPE_GET( UI_TYPE_MENUSUBMENU );

	cUIGfx::CreateParams GfxParams;
	GfxParams.Parent( this );
	GfxParams.Shape = NULL;
	GfxParams.Flags = UI_AUTO_SIZE;
	mArrow = eeNew( cUIGfx, ( GfxParams ) );
	mArrow->Visible( true );
	mArrow->Enabled( false );

	SubMenu( Params.SubMenu );

	ApplyDefaultTheme();
}

cUIMenuSubMenu::~cUIMenuSubMenu() {
}

void cUIMenuSubMenu::SetTheme( cUITheme * Theme ) {
	cUIMenuItem::SetTheme( Theme );

	mSkinArrow		= Theme->GetByName( Theme->Abbr() + "_" + "menuarrow" );

	OnStateChange();
}

void cUIMenuSubMenu::OnSizeChange() {
	cUIMenuItem::OnSizeChange();

	mArrow->Pos( Parent()->Size().Width() - mArrow->Size().Width() - 1, 0 );
	mArrow->CenterVertical();
}

void cUIMenuSubMenu::OnStateChange() {
	cUIMenuItem::OnStateChange();

	if ( NULL != mSkinArrow ) {
		if ( mSkinState->GetState() == cUISkinState::StateSelected )
			mArrow->Shape( mSkinArrow->GetShape( cUISkinState::StateMouseEnter ) );
		else
			mArrow->Shape( mSkinArrow->GetShape( cUISkinState::StateNormal ) );

		OnSizeChange();
	}
}

void cUIMenuSubMenu::SubMenu( cUIMenu * SubMenu ) {
	if ( NULL != mSubMenu && mSubMenu != SubMenu ) {
		mSubMenu->RemoveEventListener( mCbId );
		mSubMenu->RemoveEventListener( mCbId2 );
	}

	mSubMenu = SubMenu;

	if ( NULL != mSubMenu ) {
		mCbId	= mSubMenu->AddEventListener( cUIEvent::EventOnEnabledChange, cb::Make1( this, &cUIMenuSubMenu::OnSubMenuFocusLoss ) );
		mCbId2	= mSubMenu->AddEventListener( cUIEvent::EventOnHideByClick, cb::Make1( this, &cUIMenuSubMenu::OnHideByClick ) );
	}
}

cUIMenu * cUIMenuSubMenu::SubMenu() const {
	return mSubMenu;
}

Uint32 cUIMenuSubMenu::OnMouseMove( const eeVector2i &Pos, Uint32 Flags ) {
	cUIMenuItem::OnMouseMove( Pos, Flags );

	if ( NULL != mSubMenu && !mSubMenu->Visible() ) {
		mTimeOver += cUIManager::instance()->Elapsed();

		if ( mTimeOver >= mMaxTime ) {
			ShowSubMenu();
		}
	}

	return 1;
}

void cUIMenuSubMenu::ShowSubMenu() {
	mSubMenu->Parent( Parent()->Parent() );

	eeVector2i Pos = this->Pos();
	ControlToScreen( Pos );
	Pos.x += mSize.Width() + reinterpret_cast<cUIMenu*> ( Parent() )->Padding().Right;

	cUIMenu::FixMenuPos( Pos, mSubMenu, reinterpret_cast<cUIMenu*> ( Parent() ), this );

	mSubMenu->Parent()->ScreenToControl( Pos );
	mSubMenu->Pos( Pos );

	if ( !mSubMenu->Visible() ) {
		mSubMenu->Show();
	}
}

Uint32 cUIMenuSubMenu::OnMouseExit( const eeVector2i &Pos, Uint32 Flags ) {
	cUIMenuItem::OnMouseExit( Pos, Flags );

	mTimeOver = 0;

	return 1;
}

cUIGfx * cUIMenuSubMenu::Arrow() const {
	return mArrow;
}

void cUIMenuSubMenu::OnSubMenuFocusLoss( const cUIEvent * Event ) {
	cUIControl * FocusCtrl = cUIManager::instance()->FocusControl();

	if ( Parent() != FocusCtrl && !Parent()->IsParentOf( FocusCtrl ) ) {
		Parent()->SetFocus();
	}

	if ( mSubMenu->mClickHide ) {
		reinterpret_cast<cUIMenu *>( Parent() )->Hide();

		mSubMenu->mClickHide = false;
	}
}

void cUIMenuSubMenu::OnHideByClick( const cUIEvent * Event ) {
	cUIMenu * tMenu = reinterpret_cast<cUIMenu *>( Parent() );

	tMenu->mClickHide = true;
	tMenu->Hide();
}

}}
