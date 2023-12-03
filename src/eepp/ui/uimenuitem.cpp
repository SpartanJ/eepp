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

UIMenuItem* UIMenuItem::setShortcutText( const String& text ) {
	if ( !text.empty() )
		createShortcutView();
	if ( mShortcutView )
		mShortcutView->setText( text );
	return this;
}

UITextView* UIMenuItem::getShortcutView() const {
	return mShortcutView;
}

void UIMenuItem::onSizeChange() {
	UIPushButton::onSizeChange();
	if ( mShortcutView ) {
		mShortcutView->setPosition( getSize().getWidth() - mShortcutView->getSize().getWidth() -
										mShortcutView->getLayoutMargin().Right,
									0 );
		mShortcutView->centerVertical();
	}
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

Uint32 UIMenuItem::onMouseClick( const Vector2i&, const Uint32& flags ) {
	if ( !getUISceneNode()->getWindow()->getInput()->isShiftPressed() &&
		 ( flags & EE_BUTTON_LMASK ) && ( !mOnShouldCloseCb || mOnShouldCloseCb( this ) ) ) {
		getParent()->asType<UIMenu>()->backpropagateHide();
	}
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

void UIMenuItem::createShortcutView() {
	if ( mShortcutView )
		return;
	mShortcutView = UITextView::NewWithTag( mTag + "::shortcut" );
	mShortcutView->setParent( this )->setVisible( true )->setEnabled( false );
	mShortcutView->setFlags( UI_AUTO_SIZE | UI_HALIGN_RIGHT );
	auto cb = [this]( const Event* ) { onSizeChange(); };
	mShortcutView->addEventListener( Event::OnPaddingChange, cb );
	mShortcutView->addEventListener( Event::OnMarginChange, cb );
	mShortcutView->addEventListener( Event::OnSizeChange, cb );
}

}} // namespace EE::UI
