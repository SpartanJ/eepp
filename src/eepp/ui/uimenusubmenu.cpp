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
	GfxParams.setParent( this );
	GfxParams.SubTexture = NULL;
	GfxParams.Flags = UI_AUTO_SIZE;
	mArrow = eeNew( UIGfx, ( GfxParams ) );
	mArrow->visible( true );
	mArrow->enabled( false );

	subMenu( Params.SubMenu );

	applyDefaultTheme();
}

UIMenuSubMenu::~UIMenuSubMenu() {
}

Uint32 UIMenuSubMenu::getType() const {
	return UI_TYPE_MENUSUBMENU;
}

bool UIMenuSubMenu::isType( const Uint32& type ) const {
	return UIMenuSubMenu::getType() == type ? true : UIMenuItem::isType( type );
}

void UIMenuSubMenu::setTheme( UITheme * Theme ) {
	UIMenuItem::setTheme( Theme );

	mSkinArrow		= Theme->getByName( Theme->abbr() + "_" + "menuarrow" );

	onStateChange();
}

void UIMenuSubMenu::onSizeChange() {
	UIMenuItem::onSizeChange();

	mArrow->position( parent()->size().getWidth() - mArrow->size().getWidth() - 1, 0 );
	mArrow->centerVertical();
}

void UIMenuSubMenu::onStateChange() {
	UIMenuItem::onStateChange();

	if ( NULL != mSkinArrow ) {
		if ( mSkinState->getState() == UISkinState::StateSelected )
			mArrow->subTexture( mSkinArrow->getSubTexture( UISkinState::StateMouseEnter ) );
		else
			mArrow->subTexture( mSkinArrow->getSubTexture( UISkinState::StateNormal ) );

		onSizeChange();
	}
}

void UIMenuSubMenu::subMenu( UIMenu * SubMenu ) {
	if ( NULL != mSubMenu && mSubMenu != SubMenu ) {
		mSubMenu->removeEventListener( mCbId );
		mSubMenu->removeEventListener( mCbId2 );
	}

	mSubMenu = SubMenu;

	if ( NULL != mSubMenu ) {
		mCbId	= mSubMenu->addEventListener( UIEvent::EventOnEnabledChange, cb::Make1( this, &UIMenuSubMenu::onSubMenuFocusLoss ) );
		mCbId2	= mSubMenu->addEventListener( UIEvent::EventOnHideByClick, cb::Make1( this, &UIMenuSubMenu::onHideByClick ) );
	}
}

UIMenu * UIMenuSubMenu::subMenu() const {
	return mSubMenu;
}

Uint32 UIMenuSubMenu::onMouseMove( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::onMouseMove( Pos, Flags );

	if ( NULL != mSubMenu && !mSubMenu->visible() ) {
		mTimeOver += UIManager::instance()->elapsed().asMilliseconds();

		if ( mTimeOver >= mMaxTime ) {
			showSubMenu();
		}
	}

	return 1;
}

void UIMenuSubMenu::showSubMenu() {
	mSubMenu->parent( parent()->parent() );

	Vector2i Pos = this->position();
	controlToScreen( Pos );
	Pos.x += mSize.getWidth() + reinterpret_cast<UIMenu*> ( parent() )->Padding().Right;

	UIMenu::FixMenuPos( Pos, mSubMenu, reinterpret_cast<UIMenu*> ( parent() ), this );

	mSubMenu->parent()->worldToControl( Pos );
	mSubMenu->position( Pos );

	if ( !mSubMenu->visible() ) {
		mSubMenu->show();
	}
}

Uint32 UIMenuSubMenu::onMouseExit( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::onMouseExit( Pos, Flags );

	mTimeOver = 0;

	return 1;
}

UIGfx * UIMenuSubMenu::getArrow() const {
	return mArrow;
}

void UIMenuSubMenu::onSubMenuFocusLoss( const UIEvent * Event ) {
	UIControl * FocusCtrl = UIManager::instance()->focusControl();

	if ( parent() != FocusCtrl && !parent()->isParentOf( FocusCtrl ) ) {
		parent()->setFocus();
	}

	if ( mSubMenu->mClickHide ) {
		reinterpret_cast<UIMenu *>( parent() )->hide();

		mSubMenu->mClickHide = false;
	}
}

void UIMenuSubMenu::onHideByClick( const UIEvent * Event ) {
	UIMenu * tMenu = reinterpret_cast<UIMenu *>( parent() );

	tMenu->mClickHide = true;
	tMenu->hide();
}

bool UIMenuSubMenu::inheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

}}
