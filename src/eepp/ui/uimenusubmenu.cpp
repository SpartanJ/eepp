#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/scene/scenenode.hpp>

namespace EE { namespace UI {

UIMenuSubMenu * UIMenuSubMenu::New() {
	return eeNew( UIMenuSubMenu, () );
}

UIMenuSubMenu::UIMenuSubMenu() :
	UIMenuItem(),
	mSubMenu( NULL ),
	mArrow( NULL ),
	mTimeOver( 0.f ),
	mMaxTime( 200.f ),
	mCbId( 0 ),
	mCbId2( 0 )
{
	mArrow = UINode::New();
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

	mArrow->setThemeSkin( "menuarrow" );
	mArrow->setSize( mArrow->getSkinSize() );

	onStateChange();
}

void UIMenuSubMenu::onSizeChange() {
	UIMenuItem::onSizeChange();

	mArrow->setPosition( getParent()->getSize().getWidth() - mArrow->getSize().getWidth() - PixelDensity::dpToPx( 1 ), 0 );
	mArrow->centerVertical();
}

void UIMenuSubMenu::onAlphaChange() {
	UIMenuItem::onAlphaChange();

	mArrow->setAlpha( mAlpha );
}

void UIMenuSubMenu::onStateChange() {
	UIMenuItem::onStateChange();

	onSizeChange();
}

void UIMenuSubMenu::setSubMenu( UIMenu * SubMenu ) {
	if ( NULL != mSubMenu && mSubMenu != SubMenu ) {
		mSubMenu->removeEventListener( mCbId );
		mSubMenu->removeEventListener( mCbId2 );
	}

	mSubMenu = SubMenu;

	if ( NULL != mSubMenu ) {
		mCbId	= mSubMenu->addEventListener( Event::OnEnabledChange, cb::Make1( this, &UIMenuSubMenu::onSubMenuFocusLoss ) );
		mCbId2	= mSubMenu->addEventListener( Event::OnHideByClick, cb::Make1( this, &UIMenuSubMenu::onHideByClick ) );
	}
}

UIMenu * UIMenuSubMenu::getSubMenu() const {
	return mSubMenu;
}

Uint32 UIMenuSubMenu::onMouseMove( const Vector2i &Pos, const Uint32 Flags ) {
	UIMenuItem::onMouseMove( Pos, Flags );

	if ( NULL != mSceneNode && NULL != mSubMenu && !mSubMenu->isVisible() ) {
		mTimeOver += mSceneNode->getElapsed().asMilliseconds();

		if ( mTimeOver >= mMaxTime ) {
			showSubMenu();
		}
	}

	return 1;
}

void UIMenuSubMenu::showSubMenu() {
	mSubMenu->setParent( getParent()->getParent() );

	Vector2f Pos = getRealPosition();
	nodeToWorldTranslation( Pos );
	Pos.x += mSize.getWidth() + reinterpret_cast<UIMenu*> ( getParent() )->getPadding().Right;

	UIMenu::fixMenuPos( Pos, mSubMenu, reinterpret_cast<UIMenu*> ( getParent() ), this );

	mSubMenu->getParent()->worldToNode( Pos );
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

UINode * UIMenuSubMenu::getArrow() const {
	return mArrow;
}

void UIMenuSubMenu::onSubMenuFocusLoss( const Event * Event ) {
	if ( NULL != getEventDispatcher() ) {
		Node * FocusCtrl = getEventDispatcher()->getFocusControl();

		if ( getParent() != FocusCtrl && !getParent()->isParentOf( FocusCtrl ) ) {
			getParent()->setFocus();
		}
	}

	if ( mSubMenu->mClickHide ) {
		reinterpret_cast<UIMenu *>( getParent() )->hide();

		mSubMenu->mClickHide = false;
	}
}

void UIMenuSubMenu::onHideByClick( const Event * Event ) {
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
