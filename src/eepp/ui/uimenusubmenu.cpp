#include <eepp/scene/actionmanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenusubmenu.hpp>

namespace EE { namespace UI {

UIMenuSubMenu* UIMenuSubMenu::New() {
	return eeNew( UIMenuSubMenu, () );
}

UIMenuSubMenu::UIMenuSubMenu() :
	UIMenuItem( "menu::submenu" ),
	mSubMenu( nullptr ),
	mArrow( nullptr ),
	mMaxTime( Milliseconds( 200.f ) ),
	mCurWait( nullptr ) {
	mArrow = UIWidget::NewWithTag( getElementTag() + "::arrow" );
	mArrow->setParent( this );
	mArrow->setFlags( UI_AUTO_SIZE );
	applyDefaultTheme();
	mArrow->addEventListener( Event::OnSizeChange, [this]( const Event* ) { onSizeChange(); } );
	mArrow->addEventListener( Event::OnMarginChange, [this]( const Event* ) { onSizeChange(); } );
	mArrow->setVisible( true );
	mArrow->setEnabled( false );
}

UIMenuSubMenu::~UIMenuSubMenu() {}

Uint32 UIMenuSubMenu::getType() const {
	return UI_TYPE_MENUSUBMENU;
}

bool UIMenuSubMenu::isType( const Uint32& type ) const {
	return UIMenuSubMenu::getType() == type ? true : UIMenuItem::isType( type );
}

void UIMenuSubMenu::setTheme( UITheme* Theme ) {
	UIMenuItem::setTheme( Theme );

	mArrow->setThemeSkin( "menuarrow" );

	Sizef skinSize( mArrow->getSkinSize() );
	if ( skinSize != Sizef::Zero ) {
		mArrow->setSize( skinSize );
	}

	onStateChange();
	onThemeLoaded();
}

void UIMenuSubMenu::onSizeChange() {
	UIMenuItem::onSizeChange();
	mArrow->setPosition(
		getSize().getWidth() - mArrow->getSize().getWidth() - mArrow->getLayoutMargin().Right, 0 );
	mArrow->centerVertical();
}

void UIMenuSubMenu::onAlphaChange() {
	UIMenuItem::onAlphaChange();
	mArrow->setAlpha( mAlpha );
}

UIWidget* UIMenuSubMenu::getExtraInnerWidget() const {
	return mArrow;
}

void UIMenuSubMenu::onStateChange() {
	UIMenuItem::onStateChange();
	onSizeChange();
}

void UIMenuSubMenu::setSubMenu( UIMenu* subMenu ) {
	if ( nullptr != mSubMenu && mSubMenu != subMenu ) {
		getActionManager()->removeActionsByTagFromTarget( this, String::hash( "subMenu" ) );
		mSubMenu->setOwnerNode( nullptr );
	}
	mSubMenu = subMenu;
	if ( nullptr != mSubMenu )
		mSubMenu->setOwnerNode( this );
}

UIMenu* UIMenuSubMenu::getSubMenu() const {
	return mSubMenu;
}

void UIMenuSubMenu::showSubMenu() {
	sendCommonEvent( Event::OnMenuShow );
	UIMenu* menu = getParent()->asType<UIMenu>();
	mSubMenu->setParent( menu->getParent() );
	Vector2f pos = getPixelsPosition();
	nodeToWorldTranslation( pos );
	pos.x += mSize.getWidth() + menu->getPadding().Right;
	UIMenu::findBestMenuPos( pos, mSubMenu, menu, this );
	mSubMenu->getParent()->worldToNode( pos );
	mSubMenu->setPosition( pos );
	if ( !mSubMenu->isVisible() ) {
		if ( menu->mCurrentSubMenu != nullptr && menu->mCurrentSubMenu != mSubMenu )
			menu->mCurrentSubMenu->hide();
		mSubMenu->show();
		menu->mCurrentSubMenu = mSubMenu;
	}
}

Uint32 UIMenuSubMenu::onMouseOver( const Vector2i& pos, const Uint32& flags ) {
	if ( nullptr == mCurWait ) {
		mCurWait = Actions::Runnable::New(
			[this] {
				if ( isMouseOver() && mSubMenu )
					showSubMenu();
				mCurWait = nullptr;
			},
			mMaxTime );
		mCurWait->setTag( String::hash( "subMenu" ) );
		runAction( mCurWait );
	}
	return UIMenuItem::onMouseOver( pos, flags );
}

Uint32 UIMenuSubMenu::onMouseLeave( const Vector2i& pos, const Uint32& flags ) {
	UIMenuItem::onMouseLeave( pos, flags );
	if ( nullptr != mCurWait ) {
		removeAction( mCurWait );
		mCurWait = nullptr;
	}
	return UIMenuItem::onMouseLeave( pos, flags );
}

Uint32 UIMenuSubMenu::onMouseClick( const Vector2i&, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) && !mSubMenu->isVisible() )
		showSubMenu();
	return 1;
}

UINode* UIMenuSubMenu::getArrow() const {
	return mArrow;
}

const Time& UIMenuSubMenu::getMouseOverTimeShowMenu() const {
	return mMaxTime;
}

void UIMenuSubMenu::setMouseOverTimeShowMenu( const Time& maxTime ) {
	mMaxTime = maxTime;
}

}} // namespace EE::UI
