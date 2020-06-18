#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
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
	mItemSelected( NULL ),
	mItemSelectedIndex( eeINDEX_NOT_FOUND ),
	mClickHide( false ),
	mResizing( false ),
	mLastTickMove( 0 ),
	mOwnerNode( NULL ) {
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

UIMenuCheckBox* UIMenu::createMenuCheckBox( const String& text, const bool& active ) {
	UIMenuCheckBox* widget = UIMenuCheckBox::New();
	widget->setHorizontalAlign( UI_HALIGN_LEFT );
	widget->setParent( this );
	widget->setIconMinimumSize( mIconMinSize );
	widget->setText( text );
	if ( active )
		widget->setActive( active );
	return widget;
}

UIMenuCheckBox* UIMenu::addCheckBox( const String& text, const bool& active ) {
	UIMenuCheckBox* chkBox = createMenuCheckBox( text, active );
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

		if ( NULL != tItem->getIcon() && tItem->getIcon()->getLayoutMargin().Left +
												 tItem->getIcon()->getLayoutMargin().Right +
												 tItem->getIcon()->getSize().getWidth() >
											 (Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->getIcon()->getLayoutMargin().Left +
						   tItem->getIcon()->getLayoutMargin().Right +
						   tItem->getIcon()->getSize().getWidth();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( widget->getPixelsSize().getWidth() > (Int32)mMaxWidth ) {
				mMaxWidth = widget->getPixelsSize().getWidth();

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

	widget->setPixelsPosition( mRealPadding.Left, mRealPadding.Top + mNextPosY );

	mNextPosY += widget->getPixelsSize().getHeight();

	mItems.push_back( widget );

	widget->addEventListener( Event::OnSizeChange, [&]( const Event* ) {
		if ( !mResizing ) {
			widgetsSetPos();
			widgetsResize();
		}
	} );

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
	separator->setPixelsPosition( mRealPadding.Left, mRealPadding.Top + mNextPosY );
	separator->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
							  PixelDensity::dpToPxI( separator->getSkinSize().getHeight() ) );

	mNextPosY += separator->getPixelsSize().getHeight();

	mItems.push_back( separator );

	resizeMe();

	separator->addEventListener( Event::OnSizeChange, [&]( const Event* ) {
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

UIWidget* UIMenu::getItem( const String& text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem* tMenuItem = mItems[i]->asType<UIMenuItem>();

			if ( tMenuItem->getText() == text )
				return tMenuItem;
		}
	}

	return NULL;
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
		if ( NULL != mItems[i] && mItems[i]->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu* tMenu = mItems[i]->asType<UIMenuSubMenu>();

			if ( tMenu->getSubMenu() == node )
				return true;
		}
	}

	return false;
}

Uint32 UIMenu::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			if ( Msg->getSender()->getParent() == this && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
				Event ItemEvent( Msg->getSender(), Event::OnItemClicked );
				sendEvent( &ItemEvent );
			}

			return 1;
		}
		case NodeMessage::FocusLoss: {
			if ( NULL != getEventDispatcher() ) {
				Node* focusNode = getEventDispatcher()->getFocusNode();

				if ( this != focusNode && !isParentOf( focusNode ) && !isSubMenu( focusNode ) &&
					 mOwnerNode != focusNode ) {
					mClickHide = true;

					onWidgetFocusLoss();
				}

				return 1;
			}
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

		widget->setPixelsPosition( mRealPadding.Left, mRealPadding.Top + mNextPosY );

		mNextPosY += widget->getPixelsSize().getHeight();
	}

	resizeMe();
	invalidateDraw();
}

void UIMenu::resizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setPixelsSize( mMaxWidth, mNextPosY + mRealPadding.Top + mRealPadding.Bottom );
	} else {
		setPixelsSize( mSize.getWidth(), mNextPosY + mRealPadding.Top + mRealPadding.Bottom );
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

	if ( NULL != mItemSelected )
		mItemSelected->popState( UIState::StateSelected );

	mItemSelected = NULL;
	mItemSelectedIndex = eeINDEX_NOT_FOUND;

	return true;
}

void UIMenu::setItemSelected( UIWidget* Item ) {
	if ( NULL != mItemSelected ) {
		if ( mItemSelected->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu* tMenu = mItemSelected->asType<UIMenuSubMenu>();

			if ( NULL != tMenu->getSubMenu() )
				tMenu->getSubMenu()->hide();
		}

		mItemSelected->popState( UIState::StateSelected );
	}

	if ( NULL != Item )
		Item->pushState( UIState::StateSelected );

	if ( mItemSelected != Item ) {
		mItemSelected = Item;
		mItemSelectedIndex = getItemIndex( mItemSelected );
	}
}

void UIMenu::trySelect( UIWidget* node, bool up ) {
	if ( mItems.size() ) {
		if ( !node->isType( UI_TYPE_MENU_SEPARATOR ) ) {
			setItemSelected( node );
		} else {
			Uint32 Index = getItemIndex( node );

			if ( Index != eeINDEX_NOT_FOUND ) {
				if ( up ) {
					if ( Index > 0 ) {
						for ( Int32 i = (Int32)Index - 1; i >= 0; i-- ) {
							if ( !mItems[i]->isType( UI_TYPE_MENU_SEPARATOR ) ) {
								setItemSelected( mItems[i] );
								return;
							}
						}
					}

					setItemSelected( mItems[mItems.size()] );
				} else {
					for ( Uint32 i = Index + 1; i < mItems.size(); i++ ) {
						if ( !mItems[i]->isType( UI_TYPE_MENU_SEPARATOR ) ) {
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

Uint32 UIMenu::onKeyDown( const KeyEvent& Event ) {
	if ( Sys::getTicks() - mLastTickMove > 50 ) {
		switch ( Event.getKeyCode() ) {
			case KEY_DOWN:
				mLastTickMove = Sys::getTicks();
				nextSel();

				break;
			case KEY_UP:
				mLastTickMove = Sys::getTicks();
				prevSel();

				break;
			case KEY_RIGHT:
				if ( NULL != mItemSelected && ( mItemSelected->isType( UI_TYPE_MENUSUBMENU ) ) ) {
					UIMenuSubMenu* tMenu = mItemSelected->asType<UIMenuSubMenu>();

					tMenu->showSubMenu();
				}

				break;
			case KEY_LEFT:
				hide();

				break;
			case KEY_ESCAPE:
				hide();

				break;
			case KEY_RETURN:
				if ( NULL != mItemSelected && NULL != getEventDispatcher() ) {
					mItemSelected->sendMouseEvent(
						Event::MouseClick, getEventDispatcher()->getMousePos(), EE_BUTTONS_ALL );

					NodeMessage Msg( mItemSelected, NodeMessage::MouseUp, EE_BUTTONS_ALL );
					mItemSelected->messagePost( &Msg );
				}

				break;
			default:
				break;
		}
	}

	return UIWidget::onKeyDown( Event );
}

static Drawable* getIconDrawable( const std::string& name, UITheme* theme ) {
	Drawable* iconDrawable = NULL;

	if ( NULL != theme )
		iconDrawable = theme->getIconByName( name );

	if ( NULL == iconDrawable )
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

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				add( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
					 getIconDrawable( icon,
									  getUISceneNode()->getUIThemeManager()->getDefaultTheme() ) );
		} else if ( name == "menuseparator" || name == "separator" ) {
			addSeparator();
		} else if ( name == "menucheckbox" || name == "checkbox" ) {
			std::string text( item.attribute( "text" ).as_string() );
			bool active( item.attribute( "active" ).as_bool() );

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				addCheckBox( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
							 active );
		} else if ( name == "menusubmenu" || name == "submenu" ) {
			std::string text( item.attribute( "text" ).as_string() );
			std::string icon( item.attribute( "icon" ).as_string() );

			UIPopUpMenu* subMenu = UIPopUpMenu::New();

			if ( NULL != getDrawInvalidator() )
				subMenu->setParent( getDrawInvalidator() );

			subMenu->loadFromXmlNode( item );

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				addSubMenu( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
							getIconDrawable(
								icon, getUISceneNode()->getUIThemeManager()->getDefaultTheme() ),
							subMenu );
		}
	}

	endAttributesTransaction();
}

std::string UIMenu::getPropertyString( const PropertyDefinition* propertyDef,
									   const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MinIconSize:
			return String::format( "%ddp", mIconMinSize.getWidth() ) + " " +
				   String::format( "%ddp", mIconMinSize.getHeight() );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
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

void UIMenu::setOwnerNode( UINode* ownerNode ) {
	mOwnerNode = ownerNode;
}

void UIMenu::setIconMinimumSize( const Sizei& minIconSize ) {
	mIconMinSize = minIconSize;
	mBiggestIcon = eemax( mBiggestIcon, mIconMinSize.getWidth() );

	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem* menuItem = static_cast<UIMenuItem*>( mItems[i] );

			menuItem->setIconMinimumSize( mIconMinSize );
		}
	}

	widgetsSetPos();
	widgetsResize();
}

const Sizei& UIMenu::getIconMinimumSize() const {
	return mIconMinSize;
}

void UIMenu::fixMenuPos( Vector2f& Pos, UIMenu* menu, UIMenu* Parent, UIMenuSubMenu* subMenu ) {
	SceneNode* sceneNode = menu->getSceneNode();

	if ( NULL == sceneNode )
		return;

	Rectf qScreen( 0.f, 0.f, sceneNode->getPixelsSize().getWidth(),
				   sceneNode->getPixelsSize().getHeight() );
	Rectf qPos( Pos.x, Pos.y, Pos.x + menu->getPixelsSize().getWidth(),
				Pos.y + menu->getPixelsSize().getHeight() );

	if ( NULL != Parent && NULL != subMenu ) {
		Vector2f sPos = subMenu->getPixelsPosition();
		subMenu->nodeToWorldTranslation( sPos );

		Vector2f pPos = Parent->getPixelsPosition();
		Parent->nodeToWorldTranslation( pPos );

		Rectf qParent( pPos.x, pPos.y, pPos.x + Parent->getPixelsSize().getWidth(),
					   pPos.y + Parent->getPixelsSize().getHeight() );

		Pos.x = qParent.Right;
		Pos.y = sPos.y;
		qPos.Left = Pos.x;
		qPos.Right = qPos.Left + menu->getPixelsSize().getWidth();
		qPos.Top = Pos.y;
		qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();
		Vector2f oriPos( Pos );

		if ( !qScreen.contains( qPos ) ) {
			Pos.y =
				sPos.y + subMenu->getPixelsSize().getHeight() - menu->getPixelsSize().getHeight();
			qPos.Top = Pos.y;
			qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x = qParent.Left - menu->getPixelsSize().getWidth();
				Pos.y = sPos.y;
				qPos.Left = Pos.x;
				qPos.Right = qPos.Left + menu->getPixelsSize().getWidth();
				qPos.Top = Pos.y;
				qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y = sPos.y + subMenu->getPixelsSize().getHeight() -
							menu->getPixelsSize().getHeight();
					qPos.Top = Pos.y;
					qPos.Bottom = qPos.Top + menu->getPixelsSize().getHeight();

					if ( !qScreen.contains( qPos ) ) {
						Pos = oriPos;
					}
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			Pos.y -= menu->getPixelsSize().getHeight();
			qPos.Top -= menu->getPixelsSize().getHeight();
			qPos.Bottom -= menu->getPixelsSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x -= menu->getPixelsSize().getWidth();
				qPos.Left -= menu->getPixelsSize().getWidth();
				qPos.Right -= menu->getPixelsSize().getWidth();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y += menu->getPixelsSize().getHeight();
					qPos.Top += menu->getPixelsSize().getHeight();
					qPos.Bottom += menu->getPixelsSize().getHeight();
				}
			}
		}
	}
}

}} // namespace EE::UI
