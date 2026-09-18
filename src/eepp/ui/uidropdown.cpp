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
	if ( mFlags & UI_AUTO_SIZING )
		return;

	mFlags |= UI_AUTO_SIZING;

	Float max = eemax<Float>( PixelDensity::dpToPxI( getSkinSize().getHeight() ),
							  mTextCache.getLineSpacing() );

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( eeceil( max + mPaddingPx.Top + mPaddingPx.Bottom ) );
	} else if ( ( mFlags & UI_AUTO_SIZE ) && 0 == getSize().getHeight() && max > 0 ) {
		setInternalPixelsHeight( eeceil( max ) );
	}

	mFlags &= ~UI_AUTO_SIZING;
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

	// The placement is decided in screen pixels, where the field, the popup and the scene bounds
	// are directly comparable. Deriving it from the node's own dp position previously mixed
	// coordinate spaces (the candidate point was expressed in the parent's space but converted
	// through the field's own nodeToWorld, shifting the test rectangle by the field's offset), so
	// the "fits below" check failed for any field away from its parent's origin and the popup was
	// flipped above the field even when there was no room there.
	const Rectf field( getScreenRect() );
	const Sizef popUpSize( widget->getPixelsSize() );
	const Rectf sceneBounds( getUISceneNode()->getWorldBounds() );

	Float x = center ? field.Left + eefloor( ( field.getWidth() - popUpSize.getWidth() ) * 0.5f )
					 : field.Left;

	// Prefer below the field, fall back to above it, and only then clamp: the list is never placed
	// partially off screen when the scene has room for it on either side.
	Float y = field.Bottom;
	if ( y + popUpSize.getHeight() > sceneBounds.Bottom ) {
		Float above = field.Top - popUpSize.getHeight();
		y = above >= sceneBounds.Top
				? above
				: eemax( sceneBounds.Top, sceneBounds.Bottom - popUpSize.getHeight() );
	}

	// Keep the popup inside the scene horizontally as well; a list wider than its field used to
	// run off the right edge.
	x = eeclamp( x, sceneBounds.Left,
				 eemax( sceneBounds.Left, sceneBounds.Right - popUpSize.getWidth() ) );

	// World coordinates are pixels and worldToNode already converts back to dp, which is what
	// setPosition expects; applying the density a second time would shift the popup.
	Vector2f pos( x, y );
	widget->getParent()->worldToNode( pos );

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

void UIDropDown::onPopUpFocusLoss() {
	if ( NULL == getEventDispatcher() )
		return;

	bool friendIsFocus = NULL != mFriendNode && mFriendNode == getEventDispatcher()->getFocusNode();
	bool isChildFocus = isChild( getEventDispatcher()->getFocusNode() );
	bool isRelatedWidget =
		std::find( mRelatedWidgets.begin(), mRelatedWidgets.end(),
				   getEventDispatcher()->getFocusNode() ) != mRelatedWidgets.end();

	if ( getEventDispatcher()->getFocusNode() != this && !isChildFocus && !friendIsFocus &&
		 !isRelatedWidget ) {
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
