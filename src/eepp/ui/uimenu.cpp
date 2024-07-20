#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIMenu* UIMenu::New() {
	return eeNew( UIMenu, () );
}

UIMenu::UIMenu() :
	UIWidget( "menu" ),
	mMaxWidth( 0 ),
	mNextPosY( 0 ),
	mBiggestIcon( 0 ),
	mItemSelected( nullptr ),
	mItemSelectedIndex( eeINDEX_NOT_FOUND ),
	mResizing( false ),
	mOwnerNode( nullptr ) {
	mFlags |= UI_AUTO_SIZE;

	onSizeChange();

	applyDefaultTheme();
}

UIMenu::~UIMenu() {}

Uint32 UIMenu::getType() const {
	return UI_TYPE_MENU;
}

bool UIMenu::isType( const Uint32& type ) const {
	return UIMenu::getType() == type ? true : UIWidget::isType( type );
}

void UIMenu::setTheme( UITheme* theme ) {
	UIWidget::setTheme( theme );
	setThemeSkin( theme, "menu" );
	onThemeLoaded();
}

void UIMenu::onThemeLoaded() {
	UIWidget::onThemeLoaded();
	autoPadding();
	onSizeChange();
}

void UIMenu::onPaddingChange() {
	widgetsSetPos();
}

UIMenuItem* UIMenu::createMenuItem( const String& text, Drawable* icon,
									const String& shortcutText ) {
	UIMenuItem* widget = UIMenuItem::New();
	widget->setHorizontalAlign( UI_HALIGN_LEFT );
	widget->setParent( this );
	widget->setIconMinimumSize( mIconMinSize );
	widget->setIcon( icon );
	widget->setText( text );
	widget->setShortcutText( shortcutText );
	return widget;
}

UIMenuItem* UIMenu::add( const String& text, Drawable* icon, const String& shortcutText ) {
	UIMenuItem* menuItem = createMenuItem( text, icon, shortcutText );
	add( menuItem );
	return menuItem;
}

UIMenuCheckBox* UIMenu::createMenuCheckBox( const String& text, const bool& active,
											const String& shortcutText ) {
	UIMenuCheckBox* widget = UIMenuCheckBox::New();
	widget->setHorizontalAlign( UI_HALIGN_LEFT );
	widget->setParent( this );
	widget->setIconMinimumSize( mIconMinSize );
	widget->setText( text );
	widget->setShortcutText( shortcutText );
	if ( active )
		widget->setActive( active );
	return widget;
}

UIMenuCheckBox* UIMenu::addCheckBox( const String& text, const bool& active,
									 const String& shortcutText ) {
	UIMenuCheckBox* chkBox = createMenuCheckBox( text, active, shortcutText );
	add( chkBox );
	return chkBox;
}

UIMenuRadioButton* UIMenu::createMenuRadioButton( const String& text, const bool& active ) {
	UIMenuRadioButton* widget = UIMenuRadioButton::New();
	widget->setHorizontalAlign( UI_HALIGN_LEFT );
	widget->setParent( this );
	widget->setIconMinimumSize( mIconMinSize );
	widget->setText( text );
	if ( active )
		widget->setActive( active );
	return widget;
}

UIMenuRadioButton* UIMenu::addRadioButton( const String& text, const bool& active ) {
	UIMenuRadioButton* radioButton = createMenuRadioButton( text, active );
	add( radioButton );
	return radioButton;
}

UIMenuSubMenu* UIMenu::createSubMenu( const String& text, Drawable* icon, UIMenu* subMenu ) {
	UIMenuSubMenu* menu = UIMenuSubMenu::New();
	menu->setHorizontalAlign( UI_HALIGN_LEFT );
	menu->setParent( this );
	menu->setIconMinimumSize( mIconMinSize );
	menu->setIcon( icon );
	menu->setText( text );
	menu->setSubMenu( subMenu );
	return menu;
}

UIMenuSubMenu* UIMenu::addSubMenu( const String& text, Drawable* icon, UIMenu* subMenu ) {
	UIMenuSubMenu* menu = createSubMenu( text, icon, subMenu );
	add( menu );
	return menu;
}

bool UIMenu::widgetCheckSize( UIWidget* widget, const bool& resize ) {
	if ( widget->isType( UI_TYPE_MENUITEM ) ) {
		UIMenuItem* tItem = widget->asType<UIMenuItem>();
		if ( nullptr != tItem->getIcon() && tItem->getIcon()->getLayoutMargin().Left +
													tItem->getIcon()->getLayoutMargin().Right +
													tItem->getIcon()->getSize().getWidth() >
												(Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->getIcon()->getLayoutMargin().Left +
						   tItem->getIcon()->getLayoutMargin().Right +
						   tItem->getIcon()->getSize().getWidth();
		}
		if ( mFlags & UI_AUTO_SIZE ) {
			Sizef contentSize( tItem->getContentSize() );
			if ( contentSize.getWidth() > (Int32)mMaxWidth ) {
				mMaxWidth = contentSize.getWidth();
				if ( resize ) {
					widgetsResize();
					return true;
				}
			}
		}
	}
	return false;
}

UIWidget* UIMenu::add( UIWidget* widget ) {
	if ( this != widget->getParent() )
		widget->setParent( this );
	widgetCheckSize( widget );
	setWidgetSize( widget );
	widget->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top + mNextPosY );
	mNextPosY += widget->getPixelsSize().getHeight();
	mItems.push_back( widget );
	auto cb = [this]( const Event* ) {
		if ( !mResizing ) {
			widgetsSetPos();
			widgetsResize();
		}
	};
	widget->addEventListener( Event::OnVisibleChange, cb );
	widget->addEventListener( Event::OnSizeChange, cb );
	resizeMe();
	return widget;
}

void UIMenu::setWidgetSize( UIWidget* widget ) {
	mResizing = true;
	widget->setPixelsSize( mSize.getWidth(), widget->getPixelsSize().getHeight() );
	mResizing = false;
}

UIMenuSeparator* UIMenu::addSeparator() {
	UIMenuSeparator* separator = UIMenuSeparator::New();
	separator->setParent( this );
	separator->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top + mNextPosY );
	separator->setPixelsSize( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
							  PixelDensity::dpToPxI( separator->getSkinSize().getHeight() ) );
	mNextPosY += separator->getPixelsSize().getHeight();
	mItems.push_back( separator );
	resizeMe();
	separator->addEventListener( Event::OnSizeChange, [this]( const Event* ) {
		if ( !mResizing ) {
			widgetsSetPos();
			widgetsResize();
		}
	} );
	return separator;
}

UIWidget* UIMenu::getItem( const Uint32& index ) {
	eeASSERT( index < mItems.size() );
	return mItems[index];
}

UIMenuItem* UIMenu::getItem( const String& text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem* tMenuItem = mItems[i]->asType<UIMenuItem>();
			if ( tMenuItem->getText() == text )
				return tMenuItem;
		}
	}
	return nullptr;
}

UIMenuItem* UIMenu::getItemId( const std::string& id ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem* tMenuItem = mItems[i]->asType<UIMenuItem>();
			if ( tMenuItem->getId() == id )
				return tMenuItem;
		}
	}
	return nullptr;
}

Uint32 UIMenu::getItemIndex( UIWidget* item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == item )
			return i;
	}
	return eeINDEX_NOT_FOUND;
}

Uint32 UIMenu::getCount() const {
	return mItems.size();
}

UIWidget* UIMenu::getItemSelected() const {
	return mItemSelected;
}

void UIMenu::remove( const Uint32& index ) {
	eeASSERT( index < mItems.size() );
	mItems[index]->close();
	mItems.erase( mItems.begin() + index );
	widgetsSetPos();
	widgetsResize();
}

void UIMenu::remove( UIWidget* widget ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == widget ) {
			remove( i );
			break;
		}
	}
}

void UIMenu::removeAll() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->close();
	}
	mItems.clear();
	mNextPosY = 0;
	mMaxWidth = 0;
	resizeMe();
}

void UIMenu::insert( const String& text, Drawable* icon, const Uint32& index ) {
	insert( createMenuItem( text, icon ), index );
}

void UIMenu::insert( UIWidget* widget, const Uint32& index ) {
	mItems.insert( mItems.begin() + index, widget );
	childAddAt( widget, index );
	widgetsSetPos();
	widgetsResize();
}

bool UIMenu::isSubMenu( Node* node ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( nullptr != mItems[i] ) {
			if ( mItems[i] == node || ( mItems[i]->isType( UI_TYPE_MENUSUBMENU ) &&
										mItems[i]->asType<UIMenuSubMenu>()->getSubMenu() == node ) )
				return true;
		}
	}
	if ( mOwnerNode && mOwnerNode->isType( UI_TYPE_MENUSUBMENU ) ) {
		if ( mOwnerNode->getParent() == node ||
			 mOwnerNode->getParent()->asType<UIMenu>()->isSubMenu( node ) )
			return true;
	}
	return false;
}

Uint32 UIMenu::onMessage( const NodeMessage* msg ) {
	switch ( msg->getMsg() ) {
		case NodeMessage::MouseOver: {
			if ( mOwnerNode && mOwnerNode->isType( UI_TYPE_MENUSUBMENU ) ) {
				mOwnerNode->getParent()->asType<UIMenu>()->setItemSelected( mOwnerNode );
			}
			break;
		}
		case NodeMessage::MouseUp: {
			if ( msg->getSender()->getParent() == this && ( msg->getFlags() & EE_BUTTONS_LRM ) ) {
				Event itemEvent( msg->getSender(), Event::OnItemClicked );
				sendEvent( &itemEvent );
				return 1;
			}
			break;
		}
		case NodeMessage::FocusLoss: {
			if ( nullptr != getEventDispatcher() ) {
				Node* focusNode = getEventDispatcher()->getFocusNode();
				if ( this != focusNode && !isParentOf( focusNode ) && !isSubMenu( focusNode ) &&
					 mOwnerNode != focusNode ) {
					backpropagateHide();
					return 1;
				}
			}
			break;
		}
	}
	return 0;
}

void UIMenu::onSizeChange() {
	Float minWidth = 0;
	if ( !mMinWidthEq.empty() ) {
		minWidth = lengthFromValueAsDp( mMinWidthEq, CSS::PropertyRelativeTarget::None );
	}

	if ( 0 != minWidth && getSize().getWidth() < (Int32)minWidth ) {
		setSize( minWidth, PixelDensity::pxToDpI( mNextPosY ) + mPadding.Top + mPadding.Bottom );
	}

	UIWidget::onSizeChange();
}

void UIMenu::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPadding( makePadding() );
	}
}

void UIMenu::widgetsResize() {
	resizeMe();

	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		setWidgetSize( mItems[i] );
	}
}

void UIMenu::widgetsSetPos() {
	Uint32 i;
	mNextPosY = 0;
	mBiggestIcon = mIconMinSize.getWidth();

	if ( mFlags & UI_AUTO_SIZE ) {
		mMaxWidth = 0;
		for ( i = 0; i < mItems.size(); i++ ) {
			widgetCheckSize( mItems[i], false );
		}
	}

	for ( i = 0; i < mItems.size(); i++ ) {
		UIWidget* widget = mItems[i];
		if ( widget->isVisible() ) {
			widget->setPixelsPosition( mPaddingPx.Left, mPaddingPx.Top + mNextPosY );
			mNextPosY += widget->getPixelsSize().getHeight();
		}
	}

	resizeMe();
	invalidateDraw();
}

void UIMenu::resizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setPixelsSize( mMaxWidth, mNextPosY + mPaddingPx.Top + mPaddingPx.Bottom );
	} else {
		setPixelsSize( mSize.getWidth(), mNextPosY + mPaddingPx.Top + mPaddingPx.Bottom );
	}
}

bool UIMenu::show() {
	setEnabled( true );
	setVisible( true );
	return true;
}

bool UIMenu::hide() {
	setEnabled( false );
	setVisible( false );
	safeHide();
	return true;
}

void UIMenu::safeHide() {
	if ( mOwnerNode && mOwnerNode->isType( UI_TYPE_MENUSUBMENU ) ) {
		UIMenu* menu = mOwnerNode->getParent()->asType<UIMenu>();
		if ( menu->mCurrentSubMenu == this )
			menu->mCurrentSubMenu = nullptr;
		if ( mOwnerNode == menu->getItemSelected() )
			menu->unselectSelected();
		if ( getEventDispatcher()->getFocusNode() == this ||
			 isParentOf( getEventDispatcher()->getFocusNode() ) )
			menu->setFocus();
	}
	unselectSelected();
	if ( mCurrentSubMenu ) {
		mCurrentSubMenu->hide();
		mCurrentSubMenu = nullptr;
	}
}

void UIMenu::unselectSelected() {
	if ( nullptr != mItemSelected )
		mItemSelected->popState( UIState::StateSelected );
	mItemSelected = nullptr;
	mItemSelectedIndex = eeINDEX_NOT_FOUND;
}

void UIMenu::setItemSelected( UIWidget* item ) {
	if ( nullptr != mItemSelected )
		mItemSelected->popState( UIState::StateSelected );
	if ( nullptr != item )
		item->pushState( UIState::StateSelected );
	if ( mItemSelected != item ) {
		mItemSelected = item;
		mItemSelectedIndex = getItemIndex( mItemSelected );
	}
}

void UIMenu::trySelect( UIWidget* node, bool up ) {
	if ( mItems.size() ) {
		if ( !node->isType( UI_TYPE_MENU_SEPARATOR ) ) {
			setItemSelected( node );
		} else {
			Uint32 index = getItemIndex( node );
			if ( index != eeINDEX_NOT_FOUND ) {
				if ( up ) {
					if ( index > 0 ) {
						for ( Int32 i = (Int32)index - 1; i >= 0; i-- ) {
							if ( !mItems[i]->isType( UI_TYPE_MENU_SEPARATOR ) &&
								 mItems[i]->isVisible() ) {
								setItemSelected( mItems[i] );
								return;
							}
						}
					}
					setItemSelected( mItems[mItems.size()] );
				} else {
					for ( Uint32 i = index + 1; i < mItems.size(); i++ ) {
						if ( !mItems[i]->isType( UI_TYPE_MENU_SEPARATOR ) &&
							 mItems[i]->isVisible() ) {
							setItemSelected( mItems[i] );
							return;
						}
					}
					setItemSelected( mItems[0] );
				}
			}
		}
	}
}

void UIMenu::backpropagateHide() {
	hide();
	if ( mOwnerNode && mOwnerNode->getParent()->isType( UI_TYPE_MENU ) ) {
		mOwnerNode->getParent()->asType<UIMenu>()->backpropagateHide();
	}
}

const Clock& UIMenu::getInactiveTime() const {
	return mInactiveTime;
}

void UIMenu::nextSel() {
	if ( mItems.size() ) {
		if ( mItemSelectedIndex != eeINDEX_NOT_FOUND ) {
			if ( mItemSelectedIndex + 1 < mItems.size() ) {
				trySelect( mItems[mItemSelectedIndex + 1], false );
			} else {
				trySelect( mItems[0], false );
			}
		} else {
			trySelect( mItems[0], false );
		}
	}
}

void UIMenu::prevSel() {
	if ( mItems.size() ) {
		if ( mItemSelectedIndex != eeINDEX_NOT_FOUND ) {
			if ( mItemSelectedIndex >= 1 ) {
				trySelect( mItems[mItemSelectedIndex - 1], true );
			} else {
				trySelect( mItems[mItems.size() - 1], true );
			}
		} else {
			trySelect( mItems[0], true );
		}
	}
}

Uint32 UIMenu::onKeyDown( const KeyEvent& event ) {
	switch ( event.getKeyCode() ) {
		case KEY_DOWN:
			nextSel();
			break;
		case KEY_UP:
			prevSel();
			break;
		case KEY_RIGHT:
			if ( nullptr != mItemSelected && mItemSelected->isType( UI_TYPE_MENUSUBMENU ) )
				mItemSelected->asType<UIMenuSubMenu>()->showSubMenu();
			else if ( getOwnerNode() && getOwnerNode()->getParent() &&
					  getOwnerNode()->getParent()->isType( UI_TYPE_MENUBAR ) )
				getOwnerNode()->getParent()->asType<UIMenuBar>()->showNextMenu();
			break;
		case KEY_LEFT:
			if ( getOwnerNode() && getOwnerNode()->getParent() &&
				 getOwnerNode()->getParent()->isType( UI_TYPE_MENUBAR ) )
				getOwnerNode()->getParent()->asType<UIMenuBar>()->showPrevMenu();
			else if ( getOwnerNode() && getOwnerNode()->getParent() &&
					  getOwnerNode()->getParent()->isType( UI_TYPE_MENU ) )
				hide();
			break;
		case KEY_ESCAPE:
			hide();
			break;
		case KEY_KP_ENTER:
		case KEY_RETURN:
			if ( nullptr != mItemSelected && nullptr != getEventDispatcher() ) {
				mItemSelected->sendMouseEvent(
					Event::MouseClick, getEventDispatcher()->getMousePos(), EE_BUTTON_LMASK );
				if ( mItemSelected ) {
					NodeMessage msg( mItemSelected, NodeMessage::MouseUp, EE_BUTTON_LMASK );
					mItemSelected->messagePost( &msg );
				}
				if ( mItemSelected ) {
					mItemSelected->forceKeyDown( event );
				}
			}
			break;
		default:
			break;
	}
	return UIWidget::onKeyDown( event );
}

static Drawable* getIconDrawable( const std::string& name, UIIconThemeManager* iconThemeManager ) {
	Drawable* iconDrawable = nullptr;
	if ( nullptr != iconThemeManager ) {
		UIIcon* icon = iconThemeManager->findIcon( name );
		if ( icon ) {
			// TODO: Fix size
			iconDrawable = icon->getSize( PixelDensity::dpToPx( 16 ) );
		}
	}
	if ( nullptr == iconDrawable )
		iconDrawable = DrawableSearcher::searchByName( name );
	return iconDrawable;
}

void UIMenu::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();
	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node item = node.first_child(); item; item = item.next_sibling() ) {
		std::string name( item.name() );
		String::toLowerInPlace( name );
		if ( name == "menuitem" || name == "item" ) {
			std::string text( item.attribute( "text" ).as_string() );
			std::string icon( item.attribute( "icon" ).as_string() );
			if ( nullptr != mSceneNode && mSceneNode->isUISceneNode() )
				add( getTranslatorString( text ),
					 getIconDrawable( icon, getUISceneNode()->getUIIconThemeManager() ) );
		} else if ( name == "menuseparator" || name == "separator" ) {
			addSeparator();
		} else if ( name == "menucheckbox" || name == "checkbox" ) {
			std::string text( item.attribute( "text" ).as_string() );
			bool active( item.attribute( "active" ).as_bool() );
			addCheckBox( getTranslatorString( text ), active );
		} else if ( name == "menuradiobutton" || name == "radiobutton" ) {
			std::string text( item.attribute( "text" ).as_string() );
			bool active( item.attribute( "active" ).as_bool() );
			addRadioButton( getTranslatorString( text ), active );
		} else if ( name == "menusubmenu" || name == "submenu" ) {
			std::string text( item.attribute( "text" ).as_string() );
			std::string icon( item.attribute( "icon" ).as_string() );
			UIPopUpMenu* subMenu = UIPopUpMenu::New();
			if ( nullptr != getDrawInvalidator() )
				subMenu->setParent( getDrawInvalidator() );
			subMenu->loadFromXmlNode( item );
			addSubMenu( getTranslatorString( text ),
						getIconDrawable( icon, getUISceneNode()->getUIIconThemeManager() ),
						subMenu );
		}
	}
	endAttributesTransaction();
}

std::string UIMenu::getPropertyString( const PropertyDefinition* propertyDef,
									   const Uint32& propertyIndex ) const {
	if ( nullptr == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MinIconSize:
			return String::format( "%ddp", mIconMinSize.getWidth() ) + " " +
				   String::format( "%ddp", mIconMinSize.getHeight() );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIMenu::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::MinIconSize };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIMenu::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::MinIconSize:
			setIconMinimumSize( attribute.asSizei() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

UINode* UIMenu::getOwnerNode() const {
	return mOwnerNode;
}

void UIMenu::setOwnerNode( UIWidget* ownerNode ) {
	mOwnerNode = ownerNode;
}

void UIMenu::setIconMinimumSize( const Sizei& minIconSize ) {
	mIconMinSize = minIconSize;
	mBiggestIcon = eemax( mBiggestIcon, mIconMinSize.getWidth() );
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) )
			mItems[i]->asType<UIMenuItem>()->setIconMinimumSize( mIconMinSize );
	}
	widgetsSetPos();
	widgetsResize();
}

const Sizei& UIMenu::getIconMinimumSize() const {
	return mIconMinSize;
}

void UIMenu::onVisibilityChange() {
	UIWidget::onVisibilityChange();
	if ( mOwnerNode && mOwnerNode->isType( UI_TYPE_MENUSUBMENU ) ) {
		if ( mVisible ) {
			mInactiveTime.restart();
			subscribeScheduledUpdate();
		} else {
			unsubscribeScheduledUpdate();
		}
	} else {
		mInactiveTime.restart();
	}
}

void UIMenu::scheduledUpdate( const Time& ) {
	if ( !mVisible )
		return;
	Node* node = getEventDispatcher()->getMouseOverNode();
	if ( node && ( isChildOrSubMenu( node ) || getItemSelected() ) )
		mInactiveTime.restart();
	if ( mInactiveTime.getElapsedTime() > Seconds( 1 ) )
		hide();
}

bool UIMenu::isChildOrSubMenu( Node* node ) {
	return isParentOf( node ) || mOwnerNode == node ||
		   ( mCurrentSubMenu && mCurrentSubMenu->isChildOrSubMenu( node ) );
}

void UIMenu::findBestMenuPos( Vector2f& pos, UIMenu* menu, UIMenu* parent,
							  UIMenuSubMenu* subMenu ) {
	SceneNode* sceneNode = menu->getSceneNode();

	if ( nullptr == sceneNode )
		return;

	Rectf qScreen( 0.f, 0.f, sceneNode->getPixelsSize().getWidth(),
				   sceneNode->getPixelsSize().getHeight() );
	Vector2f oriPos( pos );
	Rectf qPos( pos.x, pos.y, pos.x + menu->getPixelsSize().getWidth(),
				pos.y + menu->getPixelsSize().getHeight() );

	if ( nullptr != parent && nullptr != subMenu ) {
		Rectf qPrevMenu;
		bool clipMenu = parent->getOwnerNode() && parent->getOwnerNode()->getParent() &&
						parent->getOwnerNode()->getParent()->isType( UI_TYPE_MENU );

		Vector2f sPos = subMenu->getPixelsPosition();
		subMenu->nodeToWorldTranslation( sPos );

		Vector2f pPos = parent->getPixelsPosition();
		parent->nodeToWorldTranslation( pPos );

		if ( clipMenu ) {
			UIMenu* parentOwner = parent->getOwnerNode()->getParent()->asType<UIMenu>();
			Vector2f poPos = parentOwner->getPixelsPosition();
			parentOwner->nodeToWorldTranslation( poPos );
			qPrevMenu = Rectf( poPos.x, poPos.y, poPos.x + parentOwner->getPixelsSize().getWidth(),
							   poPos.y + parentOwner->getPixelsSize().getHeight() );
		}

		Rectf qParent( pPos.x, pPos.y, pPos.x + parent->getPixelsSize().getWidth(),
					   pPos.y + parent->getPixelsSize().getHeight() );

		pos.x = qParent.Right;
		pos.y = sPos.y;
		qPos.Left = pos.x;
		qPos.Right = qPos.Left + menu->getPixelsSize().getWidth();
		qPos.Top = pos.y;
		qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

		if ( !qScreen.contains( qPos ) || ( clipMenu && qPrevMenu.overlap( qPos ) ) ) {
			pos.y =
				sPos.y + subMenu->getPixelsSize().getHeight() - menu->getPixelsSize().getHeight();
			qPos.Top = pos.y;
			qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();
			if ( !qScreen.contains( qPos ) || ( clipMenu && qPrevMenu.overlap( qPos ) ) ) {
				pos.x = qParent.Left - menu->getPixelsSize().getWidth();
				pos.y = sPos.y;
				qPos.Left = pos.x;
				qPos.Right = qPos.Left + menu->getPixelsSize().getWidth();
				qPos.Top = pos.y;
				qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

				if ( !qScreen.contains( qPos ) || ( clipMenu && qPrevMenu.overlap( qPos ) ) ) {
					pos.y = sPos.y + subMenu->getPixelsSize().getHeight() -
							menu->getPixelsSize().getHeight();
					qPos.Top = pos.y;
					qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

					if ( !qScreen.contains( qPos ) ) {
						if ( menu->getPixelsSize().getHeight() <= qScreen.getHeight() ) {
							pos = { pos.x, eefloor( ( qScreen.getHeight() -
													  menu->getPixelsSize().getHeight() ) *
													0.5f ) };
						} else {
							pos = { pos.x, 0 };
						}
					}

					if ( pos.x < 0 )
						pos.x = 0;
					if ( pos.y < 0 )
						pos.y = 0;
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			pos.y -= menu->getPixelsSize().getHeight();
			qPos.Top -= menu->getPixelsSize().getHeight();
			qPos.Bottom -= menu->getPixelsSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				pos.x -= menu->getPixelsSize().getWidth();
				qPos.Left -= menu->getPixelsSize().getWidth();
				qPos.Right -= menu->getPixelsSize().getWidth();

				if ( !qScreen.contains( qPos ) ) {
					pos.y += menu->getPixelsSize().getHeight();
					qPos.Top += menu->getPixelsSize().getHeight();
					qPos.Bottom += menu->getPixelsSize().getHeight();

					if ( !qScreen.contains( qPos ) ) {
						pos = oriPos;
						pos.y -= menu->getPixelsSize().getHeight();
						qPos.Left = pos.x;
						qPos.Right = qPos.Left + menu->getPixelsSize().getWidth();
						qPos.Top = pos.y;
						qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

						if ( !qScreen.contains( qPos ) ) {
							pos.y = qScreen.Bottom - menu->getPixelsSize().getHeight();
							qPos.Left = pos.x;
							qPos.Right = qPos.Left + menu->getPixelsSize().getWidth();

							if ( qPos.Right > qScreen.Right ) {
								qPos.Right = qScreen.Right;
								qPos.Left = qPos.Right - menu->getPixelsSize().getWidth();
							}

							qPos.Top = pos.y;
							qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

							if ( qPos.Top < qScreen.Top ) {
								qPos.Top = qScreen.Top;
								qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();
							}

							pos = qPos.getPosition();
						}
					}
				}
			}
		}
	}
}

}} // namespace EE::UI
