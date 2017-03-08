#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMenuSubMenu * UIMenuSubMenu::New() {
	return eeNew( UIMenuSubMenu, () );
}

UIMenuSubMenu::UIMenuSubMenu() :
	UIMenuItem(),
	mSubMenu( NULL ),
	mSkinArrow( NULL ),
	mArrow( NULL ),
	mTimeOver( 0.f ),
	mMaxTime( 200.f ),
	mCbId( 0 ),
	mCbId2( 0 )
{
	mArrow = UIImage::New();
	mArrow->setParent( this );
	mArrow->setFlags( UI_AUTO_SIZE );
	mArrow->setVisible( true );
	mArrow->setEnabled( false );

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

	mSkinArrow		= Theme->getByName( Theme->getAbbr() + "_" + "menuarrow" );

	onStateChange();
}

void UIMenuSubMenu::onSizeChange() {
	UIMenuItem::onSizeChange();

	mArrow->setPosition( getParent()->getSize().getWidth() - mArrow->getSize().getWidth() - PixelDensity::dpToPxI( 1 ), 0 );
	mArrow->centerVertical();
}

void UIMenuSubMenu::onStateChange() {
	UIMenuItem::onStateChange();

	if ( NULL != mSkinArrow ) {
		if ( mSkinState->getState() == UISkinState::StateSelected )
			mArrow->setSubTexture( mSkinArrow->getSubTexture( UISkinState::StateMouseEnter ) );
		else
			mArrow->setSubTexture( mSkinArrow->getSubTexture( UISkinState::StateNormal ) );

		onSizeChange();
	}
}

void UIMenuSubMenu::setSubMenu( UIMenu * SubMenu ) {
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

UIMenu * UIMenuSubMenu::getSubMenu() const {
	return mSubMenu;
}

Uint32 UIMenuSubMenu::onMouseMove( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::onMouseMove( Pos, Flags );

	if ( NULL != mSubMenu && !mSubMenu->isVisible() ) {
		mTimeOver += getElapsed().asMilliseconds();

		if ( mTimeOver >= mMaxTime ) {
			showSubMenu();
		}
	}

	return 1;
}

void UIMenuSubMenu::showSubMenu() {
	mSubMenu->setParent( getParent()->getParent() );

	Vector2i Pos = this->getRealPosition();
	controlToScreen( Pos );
	Pos.x += mRealSize.getWidth() + reinterpret_cast<UIMenu*> ( getParent() )->getPadding().Right;

	UIMenu::fixMenuPos( Pos, mSubMenu, reinterpret_cast<UIMenu*> ( getParent() ), this );

	mSubMenu->getParent()->worldToControl( Pos );
	mSubMenu->setPosition( Pos );

	if ( !mSubMenu->isVisible() ) {
		mSubMenu->show();
	}
}

Uint32 UIMenuSubMenu::onMouseExit( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::onMouseExit( Pos, Flags );

	mTimeOver = 0;

	return 1;
}

UIImage * UIMenuSubMenu::getArrow() const {
	return mArrow;
}

void UIMenuSubMenu::onSubMenuFocusLoss( const UIEvent * Event ) {
	UIControl * FocusCtrl = UIManager::instance()->getFocusControl();

	if ( getParent() != FocusCtrl && !getParent()->isParentOf( FocusCtrl ) ) {
		getParent()->setFocus();
	}

	if ( mSubMenu->mClickHide ) {
		reinterpret_cast<UIMenu *>( getParent() )->hide();

		mSubMenu->mClickHide = false;
	}
}

void UIMenuSubMenu::onHideByClick( const UIEvent * Event ) {
	UIMenu * tMenu = reinterpret_cast<UIMenu *>( getParent() );

	tMenu->mClickHide = true;
	tMenu->hide();
}

bool UIMenuSubMenu::inheritsFrom( const Uint32 Type ) {
	if ( Type == UI_TYPE_MENUITEM )
		return true;

	return false;
}

Float UIMenuSubMenu::getMouseOverTimeShowMenu() const {
	return mMaxTime;
}

void UIMenuSubMenu::setMouseOverTimeShowMenu(const Float & maxTime) {
	mMaxTime = maxTime;
}

}}
