#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uidropdown.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIDropDown::MenuWidthMode UIDropDown::menuWidthModeFromString( std::string_view str ) {
	if ( "contents" == str || "fit-to-contents" == str )
		return MenuWidthMode::Contents;
	if ( "contents-centered" == str || "fit-to-contents-centered" == str )
		return MenuWidthMode::ContentsCentered;
	if ( "expand-if-needed" == str || "fit-to-drop-down-expand-if-needed" == str )
		return MenuWidthMode::ExpandIfNeeded;
	if ( "expand-if-needed-centered" == str || "fit-to-drop-down-expand-if-needed-centered" == str )
		return MenuWidthMode::ExpandIfNeededCentered;
	return MenuWidthMode::DropDown; // "dropdown"
}

std::string UIDropDown::menuWidthModeToString( MenuWidthMode rule ) {
	switch ( rule ) {
		case MenuWidthMode::DropDown:
			return "dropdown";
		case MenuWidthMode::Contents:
			return "contents";
		case MenuWidthMode::ContentsCentered:
			return "contents-centered";
		case MenuWidthMode::ExpandIfNeeded:
			return "expand-if-needed";
		case MenuWidthMode::ExpandIfNeededCentered:
			return "expand-if-needed-centered";
	}
	return "dropdown";
}

UIDropDown::UIDropDown( const std::string& tag ) : UITextInput( tag ) {
	mEnabledCreateContextMenu = false;
	setClipType( ClipType::ContentBox );
	setFlags( UI_AUTO_SIZE | UI_AUTO_PADDING | UI_SCROLLABLE );
	unsetFlags( UI_TEXT_SELECTION_ENABLED );
	setAllowEditing( false );
}

UIDropDown::~UIDropDown() {}

Uint32 UIDropDown::getType() const {
	return UI_TYPE_DROPDOWN;
}

bool UIDropDown::isType( const Uint32& type ) const {
	return UIDropDown::getType() == type ? true : UITextInput::isType( type );
}

void UIDropDown::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "dropdownlist" );
	onThemeLoaded();
}

void UIDropDown::onSizeChange() {
	onAutoSize();
	UITextInput::onSizeChange();
}

void UIDropDown::onThemeLoaded() {
	autoPadding();
	onAutoSize();
}

void UIDropDown::setFriendNode( UINode* friendNode ) {
	mFriendNode = friendNode;
}

void UIDropDown::onAutoSize() {
	Float max = eemax<Float>( PixelDensity::dpToPxI( getSkinSize().getHeight() ),
							  mTextCache.getLineSpacing() );

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( eeceil( max + mPaddingPx.Top + mPaddingPx.Bottom ) );
	} else if ( ( ( mFlags & UI_AUTO_SIZE ) || 0 == getSize().getHeight() ) && max > 0 ) {
		setInternalPixelsHeight( eeceil( max ) );
	}
}

UIWidget* UIDropDown::getPopUpWidget() const {
	return nullptr;
}

Uint32 UIDropDown::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( ( Flags & EE_BUTTON_LMASK ) && NULL == mFriendNode )
		showList();

	if ( NULL != mFriendNode ) {
		UITextInput::onMouseClick( Pos, Flags );
	}

	return 1;
}

UIDropDown* UIDropDown::showList() {
	return this;
}

Float UIDropDown::getPopUpWidth( Float contentsWidth ) const {
	Float width = NULL != mFriendNode ? mFriendNode->getSize().getWidth() : getSize().getWidth();

	if ( mStyleConfig.menuWidthRule == MenuWidthMode::Contents ||
		 mStyleConfig.menuWidthRule == MenuWidthMode::ContentsCentered ) {
		width = contentsWidth;
	}

	if ( ( mStyleConfig.menuWidthRule == MenuWidthMode::ExpandIfNeeded ||
		   mStyleConfig.menuWidthRule == MenuWidthMode::ExpandIfNeededCentered ) &&
		 contentsWidth > width ) {
		width = contentsWidth;
	}

	return width;
}

void UIDropDown::alignPopUp( UIWidget* widget ) {
	if ( !mStyleConfig.PopUpToRoot )
		widget->setParent( getWindowContainer() );
	else
		widget->setParent( getUISceneNode()->getRoot() );

	widget->toFront();

	bool center = mStyleConfig.menuWidthRule == MenuWidthMode::ContentsCentered ||
				  mStyleConfig.menuWidthRule == MenuWidthMode::ExpandIfNeededCentered;

	Float width = widget->getSize().getWidth();
	Float offsetX = center ? eefloor( ( getSize().getWidth() - width ) * 0.5f ) : 0;

	Vector2f pos( mDpPos.x + offsetX, mDpPos.y + getSize().getHeight() );
	Vector2f posCpy( pos );
	nodeToWorld( posCpy );

	if ( !getUISceneNode()->getWorldBounds().contains( Rectf( posCpy, widget->getSize() ) ) ) {
		pos = Vector2f( mDpPos.x + offsetX, mDpPos.y - widget->getSize().getHeight() );
	}

	if ( mStyleConfig.PopUpToRoot ) {
		getParent()->nodeToWorld( pos );
		pos = PixelDensity::pxToDp( pos );
	} else {
		Node* parentNode = getParent();
		Node* rp = getWindowContainer();
		while ( rp != parentNode ) {
			pos += parentNode->getPosition();
			parentNode = parentNode->getParent();
		}
	}

	widget->setPosition( pos );
	show();
	widget->setFocus();
}

bool UIDropDown::getPopUpToRoot() const {
	return mStyleConfig.PopUpToRoot;
}

UIDropDown* UIDropDown::setPopUpToRoot( bool popUpToRoot ) {
	mStyleConfig.PopUpToRoot = popUpToRoot;
	return this;
}

Uint32 UIDropDown::getMaxNumVisibleItems() const {
	return mStyleConfig.MaxNumVisibleItems;
}

UIDropDown* UIDropDown::setMaxNumVisibleItems( const Uint32& maxNumVisibleItems ) {
	mStyleConfig.MaxNumVisibleItems = maxNumVisibleItems;
	return this;
}

const UIDropDown::StyleConfig& UIDropDown::getStyleConfig() const {
	return mStyleConfig;
}

UIDropDown* UIDropDown::setStyleConfig( const StyleConfig& styleConfig ) {
	mStyleConfig = styleConfig;

	setMaxNumVisibleItems( mStyleConfig.MaxNumVisibleItems );
	setPopUpToRoot( mStyleConfig.PopUpToRoot );
	setMenuWidthMode( mStyleConfig.menuWidthRule );
	return this;
}

void UIDropDown::onWidgetClear( const Event* ) {
	setText( "" );
	sendCommonEvent( Event::OnClear );
}

void UIDropDown::onItemKeyDown( const Event* Event ) {
	const KeyEvent* KEvent = reinterpret_cast<const KeyEvent*>( Event );

	if ( KEvent->getKeyCode() == KEY_RETURN )
		onItemClicked( Event );
	else if ( KEvent->getKeyCode() == KEY_ESCAPE ) {
		hide();
		setFocus();
	}
}

void UIDropDown::onPopUpFocusLoss( const Event* ) {
	if ( NULL == getEventDispatcher() )
		return;

	bool frienIsFocus = NULL != mFriendNode && mFriendNode == getEventDispatcher()->getFocusNode();
	bool isChildFocus = isChild( getEventDispatcher()->getFocusNode() );

	if ( getEventDispatcher()->getFocusNode() != this && !isChildFocus && !frienIsFocus ) {
		hide();
	}
}

void UIDropDown::onItemClicked( const Event* ) {
	hide();
	setFocus();
}

void UIDropDown::onItemSelected( const Event* ) {}

void UIDropDown::show() {
	UIWidget* widget = getPopUpWidget();
	if ( NULL == widget )
		return;

	widget->setEnabled( true );
	widget->setVisible( true );

	if ( NULL != getUISceneNode() &&
		 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		widget->runAction( Actions::Sequence::New(
			Actions::Fade::New( 255.f == widget->getAlpha() ? 0.f : widget->getAlpha(), 255.f,
								getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
			Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) ) );
	}
}

void UIDropDown::hide() {
	UIWidget* widget = getPopUpWidget();
	if ( NULL == widget )
		return;

	if ( NULL != getUISceneNode() &&
		 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		widget->runAction( Actions::Sequence::New(
			Actions::FadeOut::New( getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
			Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
	} else {
		widget->setEnabled( false );
		widget->setVisible( false );
	}
}

Uint32 UIDropDown::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	if ( getParent()->isType( UI_TYPE_COMBOBOX ) ) {
		return UITextInput::onMouseOver( position, flags );
	} else {
		return UITextView::onMouseOver( position, flags );
	}
}

Uint32 UIDropDown::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	if ( getParent()->isType( UI_TYPE_COMBOBOX ) ) {
		return UITextInput::onMouseLeave( position, flags );
	} else {
		return UITextView::onMouseLeave( position, flags );
	}
}

Uint32 UIDropDown::onKeyDown( const KeyEvent& Event ) {
	return UITextInput::onKeyDown( Event );
}

bool UIDropDown::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::PopUpToRoot:
			setPopUpToRoot( attribute.asBool() );
			break;
		case PropertyId::MaxVisibleItems:
			setMaxNumVisibleItems( attribute.asUint() );
			break;
		case PropertyId::MenuWidthMode:
			setMenuWidthMode( menuWidthModeFromString( attribute.getValue() ) );
			break;
		default:
			return UITextInput::applyProperty( attribute );
	}

	return true;
}

std::string UIDropDown::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::PopUpToRoot:
			return mStyleConfig.PopUpToRoot ? "true" : "false";
		case PropertyId::MaxVisibleItems:
			return String::toString( mStyleConfig.MaxNumVisibleItems );
		case PropertyId::MenuWidthMode:
			return menuWidthModeToString( mStyleConfig.menuWidthRule );
		default:
			return UITextInput::getPropertyString( propertyDef, propertyIndex );
	}

	return "";
}

std::vector<PropertyId> UIDropDown::getPropertiesImplemented() const {
	auto props = UITextInput::getPropertiesImplemented();
	auto local = { PropertyId::PopUpToRoot, PropertyId::MaxVisibleItems,
				   PropertyId::MenuWidthMode };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

UIDropDown* UIDropDown::setMenuWidthMode( MenuWidthMode rule ) {
	mStyleConfig.menuWidthRule = rule;
	return this;
}

UIDropDown::MenuWidthMode UIDropDown::getMenuWidthMode() const {
	return mStyleConfig.menuWidthRule;
}

}} // namespace EE::UI
