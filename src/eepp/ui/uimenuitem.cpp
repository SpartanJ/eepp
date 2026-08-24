#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

UIMenuItem* UIMenuItem::New() {
	return eeNew( UIMenuItem, () );
}

UIMenuItem::UIMenuItem( const std::string& tag ) : UIPushButton( tag ), mShortcutView( NULL ) {
	getIcon();
	unsetFlags( UI_AUTO_SIZE );
	setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
	mTextBox->setElementTag( mTag + "::text" );
	applyDefaultTheme();
}

UIMenuItem::UIMenuItem() : UIMenuItem( "menu::item" ) {}

UIMenuItem::~UIMenuItem() {}

Uint32 UIMenuItem::getType() const {
	return UI_TYPE_MENUITEM;
}

bool UIMenuItem::isType( const Uint32& type ) const {
	return UIMenuItem::getType() == type ? true : UIPushButton::isType( type );
}

void UIMenuItem::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "menuitem" );
	onThemeLoaded();
}

void UIMenuItem::activate() {
	if ( !isEnabled() || nullptr == getParent() || !getParent()->isType( UI_TYPE_MENU ) )
		return;

	UIMenu* menu = getParent()->asType<UIMenu>();
	Input* input = nullptr != getUISceneNode() ? getInput() : nullptr;
	if ( ( nullptr == input || !input->isShiftPressed() ) &&
		 ( !mOnShouldCloseCb || mOnShouldCloseCb( this ) ) ) {
		menu->backpropagateHide();
	}

	Event itemEvent( this, Event::OnItemClicked );
	menu->sendEvent( &itemEvent );
}

UIMenuItem* UIMenuItem::setShortcutText( const String& text ) {
	mShortcut = {};
	if ( !text.empty() && nullptr != getUISceneNode() )
		mShortcut = KeyBindings::toShortcut( getInput(), text.toUtf8() );
	if ( !text.empty() )
		createShortcutView();
	if ( mShortcutView )
		mShortcutView->setText( text );
	return this;
}

const KeyBindings::Shortcut& UIMenuItem::getShortcut() const {
	if ( mShortcut.empty() && nullptr != mShortcutView && !mShortcutView->getText().empty() &&
		 nullptr != getUISceneNode() ) {
		mShortcut = KeyBindings::toShortcut( getInput(), mShortcutView->getText().toUtf8() );
	}
	return mShortcut;
}

UITextView* UIMenuItem::getShortcutView() const {
	return mShortcutView;
}

void UIMenuItem::onSizeChange() {
	UIPushButton::onSizeChange();
	refreshShortcut();
}

void UIMenuItem::onStateChange() {
	UIPushButton::onStateChange();
	refreshShortcut();
}

void UIMenuItem::onLayoutUpdate() {
	refreshShortcut();
}

Uint32 UIMenuItem::onMouseOver( const Vector2i& pos, const Uint32& flags ) {
	UIPushButton::onMouseOver( pos, flags );
	getParent()->asType<UIMenu>()->setItemSelected( this );
	return 1;
}

Uint32 UIMenuItem::onMouseLeave( const Vector2i& pos, const Uint32& flags ) {
	UIPushButton::onMouseLeave( pos, flags );
	if ( getParent()->asType<UIMenu>()->getItemSelected() == this )
		getParent()->asType<UIMenu>()->unselectSelected();
	return 1;
}

UIWidget* UIMenuItem::getExtraInnerWidget() const {
	return mShortcutView;
}

UIMenuItem::OnShouldCloseCb UIMenuItem::getOnShouldCloseCb() const {
	return mOnShouldCloseCb;
}

UIMenuItem* UIMenuItem::setOnShouldCloseCb( const OnShouldCloseCb& onShouldCloseCb ) {
	mOnShouldCloseCb = onShouldCloseCb;
	return this;
}

MenuRole UIMenuItem::getMenuRole() const {
	return mMenuRole;
}

UIMenuItem* UIMenuItem::setMenuRole( MenuRole role ) {
	mMenuRole = role;
	return this;
}

void UIMenuItem::createShortcutView() {
	if ( mShortcutView )
		return;
	mShortcutView = UITextView::NewWithTag( mTag + "::shortcut" );
	mShortcutView->setParent( this )->setVisible( true )->setEnabled( false );
	mShortcutView->setFlags( UI_AUTO_SIZE | UI_HALIGN_RIGHT );
	auto cb = [this]( const Event* ) { onSizeChange(); };
	mShortcutView->on( Event::OnPaddingChange, cb );
	mShortcutView->on( Event::OnMarginChange, cb );
	mShortcutView->on( Event::OnSizeChange, cb );
}

void UIMenuItem::refreshShortcut() {
	if ( mShortcutView == nullptr )
		return;

	mShortcutView->setPosition( getSize().getWidth() - mShortcutView->getSize().getWidth() -
									mShortcutView->getLayoutMargin().Right,
								0 );
	mShortcutView->centerVertical();
}

}} // namespace EE::UI
