#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMenuSubMenu::UIMenuSubMenu( UIMenuSubMenu::CreateParams& Params ) :
	UIMenuItem( Params ),
	mSubMenu( NULL ),
	mSkinArrow( NULL ),
	mArrow( NULL ),
	mTimeOver( 0.f ),
	mMaxTime( Params.MouseOverTimeShowMenu ),
	mCbId( 0 ),
	mCbId2( 0 )
{
	UIGfx::CreateParams GfxParams;
	GfxParams.Parent( this );
	GfxParams.SubTexture = NULL;
	GfxParams.Flags = UI_AUTO_SIZE;
	mArrow = eeNew( UIGfx, ( GfxParams ) );
	mArrow->Visible( true );
	mArrow->Enabled( false );

	SubMenu( Params.SubMenu );

	ApplyDefaultTheme();
}

UIMenuSubMenu::~UIMenuSubMenu() {
}

Uint32 UIMenuSubMenu::Type() const {
	return UI_TYPE_MENUSUBMENU;
}

bool UIMenuSubMenu::IsType( const Uint32& type ) const {
	return UIMenuSubMenu::Type() == type ? true : UIMenuItem::IsType( type );
}

void UIMenuSubMenu::SetTheme( UITheme * Theme ) {
	UIMenuItem::SetTheme( Theme );

	mSkinArrow		= Theme->GetByName( Theme->Abbr() + "_" + "menuarrow" );

	OnStateChange();
}

void UIMenuSubMenu::OnSizeChange() {
	UIMenuItem::OnSizeChange();

	mArrow->Pos( Parent()->Size().Width() - mArrow->Size().Width() - 1, 0 );
	mArrow->CenterVertical();
}

void UIMenuSubMenu::OnStateChange() {
	UIMenuItem::OnStateChange();

	if ( NULL != mSkinArrow ) {
		if ( mSkinState->GetState() == UISkinState::StateSelected )
			mArrow->SubTexture( mSkinArrow->GetSubTexture( UISkinState::StateMouseEnter ) );
		else
			mArrow->SubTexture( mSkinArrow->GetSubTexture( UISkinState::StateNormal ) );

		OnSizeChange();
	}
}

void UIMenuSubMenu::SubMenu( UIMenu * SubMenu ) {
	if ( NULL != mSubMenu && mSubMenu != SubMenu ) {
		mSubMenu->RemoveEventListener( mCbId );
		mSubMenu->RemoveEventListener( mCbId2 );
	}

	mSubMenu = SubMenu;

	if ( NULL != mSubMenu ) {
		mCbId	= mSubMenu->AddEventListener( UIEvent::EventOnEnabledChange, cb::Make1( this, &UIMenuSubMenu::OnSubMenuFocusLoss ) );
		mCbId2	= mSubMenu->AddEventListener( UIEvent::EventOnHideByClick, cb::Make1( this, &UIMenuSubMenu::OnHideByClick ) );
	}
}

UIMenu * UIMenuSubMenu::SubMenu() const {
	return mSubMenu;
}

Uint32 UIMenuSubMenu::OnMouseMove( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::OnMouseMove( Pos, Flags );

	if ( NULL != mSubMenu && !mSubMenu->Visible() ) {
		mTimeOver += UIManager::instance()->Elapsed().AsMilliseconds();

		if ( mTimeOver >= mMaxTime ) {
			ShowSubMenu();
		}
	}

	return 1;
}

void UIMenuSubMenu::ShowSubMenu() {
	mSubMenu->Parent( Parent()->Parent() );

	Vector2i Pos = this->Pos();
	ControlToScreen( Pos );
	Pos.x += mSize.Width() + reinterpret_cast<UIMenu*> ( Parent() )->Padding().Right;

	UIMenu::FixMenuPos( Pos, mSubMenu, reinterpret_cast<UIMenu*> ( Parent() ), this );

	mSubMenu->Parent()->WorldToControl( Pos );
	mSubMenu->Pos( Pos );

	if ( !mSubMenu->Visible() ) {
		mSubMenu->Show();
	}
}

Uint32 UIMenuSubMenu::OnMouseExit( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::OnMouseExit( Pos, Flags );

	mTimeOver = 0;

	return 1;
}

UIGfx * UIMenuSubMenu::Arrow() const {
	return mArrow;
}

void UIMenuSubMenu::OnSubMenuFocusLoss( const UIEvent * Event ) {
	UIControl * FocusCtrl = UIManager::instance()->FocusControl();

	if ( Parent() != FocusCtrl && !Parent()->IsParentOf( FocusCtrl ) ) {
		Parent()->SetFocus();
	}

	if ( mSubMenu->mClickHide ) {
		reinterpret_cast<UIMenu *>( Parent() )->Hide();

		mSubMenu->mClickHide = false;
	}
}

void UIMenuSubMenu::OnHideByClick( const UIEvent * Event ) {
	UIMenu * tMenu = reinterpret_cast<UIMenu *>( Parent() );

	tMenu->mClickHide = true;
	tMenu->Hide();
}

bool UIMenuSubMenu::InheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

}}
