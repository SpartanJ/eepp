#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI {

UITab* UITab::New() {
	return eeNew( UITab, () );
}

UITab::UITab() :
	UISelectButton( "tab" ), mOwnedWidget( NULL ), mDragTotalDiff( 0.f ), mTabWidget( NULL ) {
	mTextBox->setElementTag( mTag + "::text" );
	auto cb = [this]( const Event* ) { onSizeChange(); };
	mTextBox->addEventListener( Event::OnSizeChange, cb );
	mCloseButton = UIWidget::NewWithTag( mTag + "::close" );
	mCloseButton->setParent( const_cast<UITab*>( this ) );
	mCloseButton->setEnabled( false );
	mCloseButton->setVisible( false );
	mCloseButton->addEventListener( Event::OnPaddingChange, cb );
	mCloseButton->addEventListener( Event::OnMarginChange, cb );
	mCloseButton->addEventListener( Event::OnSizeChange, cb );
	mCloseButton->addEventListener( Event::OnVisibleChange, cb );
	applyDefaultTheme();
	unsetFlags( UI_DRAG_VERTICAL );
}

UITab::~UITab() {}

Uint32 UITab::getType() const {
	return UI_TYPE_TAB;
}

bool UITab::isType( const Uint32& type ) const {
	return UITab::getType() == type ? true : UISelectButton::isType( type );
}

UITabWidget* UITab::getTabWidget() const {
	if ( NULL != getParent() && NULL != getParent()->getParent() &&
		 getParent()->getParent()->isType( UI_TYPE_TABWIDGET ) ) {
		return getParent()->getParent()->asType<UITabWidget>();
	} else if ( mTabWidget ) {
		return mTabWidget;
	}
	return NULL;
}

Uint32 UITab::onDrag( const Vector2f& pos, const Uint32&, const Sizef& dragDiff ) {
	UITabWidget* tabW = getTabWidget();
	if ( !tabW )
		return 0;
	Vector2f newPos( mPosition - dragDiff );
	if ( mFlags & UI_DRAG_VERTICAL )
		newPos = tabW->getTabBar()->convertToNodeSpace( newPos );

	Uint32 index = tabW->getTabIndex( this );
	bool swapped = false;
	if ( index > 0 ) {
		UITab* tab = tabW->getTab( index - 1 );
		Float tabMiddle = tab->getPixelsPosition().x + tab->getPixelsSize().getWidth() * 0.5f;
		if ( tab && newPos.x < tabMiddle ) {
			tabW->swapTabs( this, tab );
			swapped = true;
		}
	}
	if ( !swapped && index + 1 < tabW->getTabCount() ) {
		UITab* tab = tabW->getTab( index + 1 );
		Float tabMiddle = tab->getPixelsPosition().x + tab->getPixelsSize().getWidth() * 0.5f;
		if ( tab && newPos.x + mSize.getWidth() > tabMiddle )
			tabW->swapTabs( tab, this );
	}

	if ( tabW->getAllowDragAndDropTabs() && !( mFlags & UI_DRAG_VERTICAL ) ) {
		mDragTotalDiff += mDragPoint.y - pos.y;
		if ( eeabs( mDragTotalDiff ) >= tabW->getTabVerticalDragResistance() ) {
			setFlags( UI_DRAG_VERTICAL );
			setPixelsPosition( mPosition.x, mPosition.y - mDragTotalDiff );
			mDragPoint = pos;
			mTabWidget = getTabWidget();
			Vector2f posDiff( pos );
			worldToNode( posDiff );
			setParent( getUISceneNode()->getRoot() );
			setPixelsPosition( pos - posDiff );
		}
	}

	if ( tabW->getAllowDragAndDropTabs() ) {
		setEnabled( false );
		Node* overFind = getUISceneNode()->overFind( pos );
		setEnabled( true );

		if ( overFind && overFind->isType( UI_TYPE_WIDGET ) ) {
			UIWidget* widget = overFind->asType<UIWidget>()->acceptsDropOfWidgetInTree( this );

			if ( mCurDropWidget ) {
				mCurDropWidget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 0 );
				mCurDropWidget = nullptr;
			}

			if ( widget ) {
				mCurDropWidget = widget;
				mCurDropWidget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 1 );
			}
		}
	}

	return 1;
}

Uint32 UITab::onDragStart( const Vector2i& position, const Uint32& flags ) {
	UITabWidget* tabW = getTabWidget();
	mTabWidget = NULL;
	mDragTotalDiff = 0;
	mWasToolipEnabled = isTooltipEnabled();
	if ( mWasToolipEnabled )
		setTooltipEnabled( false );
	unsetFlags( UI_DRAG_VERTICAL );
	if ( tabW ) {
		for ( size_t i = 0; i < tabW->getTabCount(); i++ ) {
			UITab* tab = tabW->getTab( i );
			if ( tab != this ) {
				tab->setEnabled( false );
			}
		}
	}
	return UISelectButton::onDragStart( position, flags );
}

Uint32 UITab::onDragStop( const Vector2i& position, const Uint32& flags ) {
	if ( mWasToolipEnabled )
		setTooltipEnabled( true );
	UITabWidget* tabW = getTabWidget();
	if ( mTabWidget ) {
		setParent( tabW->getTabBar() );
		mTabWidget = NULL;
	}
	if ( mCurDropWidget ) {
		mCurDropWidget->writeNodeFlag( NODE_FLAG_DROPPABLE_HOVERING, 0 );
		mCurDropWidget = nullptr;
	}
	if ( tabW ) {
		for ( size_t i = 0; i < tabW->getTabCount(); i++ ) {
			UITab* tab = tabW->getTab( i );
			if ( tab != this ) {
				tab->setEnabled( true );
			}
		}
		tabW->posTabs();
		tabW->zorderTabs();
	}
	return UISelectButton::onDragStop( position, flags );
}

void UITab::onParentChange() {
	applyDefaultTheme();
	UISelectButton::onParentChange();
}

void UITab::onSizeChange() {
	onAutoSize();
	if ( NULL != getTabWidget() )
		getTabWidget()->orderTabs();
	UISelectButton::onSizeChange();
}

UIWidget* UITab::getExtraInnerWidget() const {
	return mCloseButton;
}

void UITab::removeTab( bool destroyOwnedNode, bool immediateClose ) {
	getTabWidget()->removeTab( this, destroyOwnedNode, immediateClose );
}

UIWidget* UITab::getCloseButton() const {
	return mCloseButton;
}

void UITab::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	std::string tabPos = "tab";

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( tTabW->getSpecialBorderTabs() ) {
			if ( 0 == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_left";
			} else if ( tTabW->getTabCount() > 0 &&
						( tTabW->getTabCount() - 1 ) == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_right";
			}
		}
	}

	UINode::setThemeSkin( Theme, tabPos );

	onThemeLoaded();
	onStateChange();
}

void UITab::onStateChange() {
	UISelectButton::onStateChange();

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW && NULL != mSkinState ) {
		Int32 skinSize = getSkinSize( getSkin(), mSkinState->getCurrentState() ).getHeight();

		if ( 0 == skinSize ) {
			skinSize = getSkinSize().getHeight();
		}

		setSize( getSize().getWidth(), skinSize );
	}
}

const String& UITab::getText() const {
	return UIPushButton::getText();
}

UIPushButton* UITab::setText( const String& text ) {
	if ( text != mText ) {
		mText = text;

		UITabWidget* tTabW = getTabWidget();

		if ( NULL != tTabW ) {
			updateTab();

			tTabW->orderTabs();
		}

		sendTextEvent( Event::OnTitleChange, mText.toUtf8() );
	}
	return this;
}

void UITab::onAutoSize() {
	UITabWidget* tTabW = getTabWidget();

	if ( ( mFlags & UI_AUTO_SIZE ) == 0 )
		return;

	Float nonTextW =
		( NULL != mIcon ? mIcon->getSize().getWidth() + mIcon->getLayoutMargin().Left +
							  mIcon->getLayoutMargin().Right
						: 0 ) +
		( NULL != mCloseButton && mCloseButton->isVisible()
			  ? mCloseButton->getSize().getWidth() + mCloseButton->getLayoutMargin().Left +
					mCloseButton->getLayoutMargin().Right
			  : 0 ) +
		getSkinSize().getWidth();

	Float textW = mTextBox->getSize().getWidth();
	Float w = textW + nonTextW;

	if ( NULL != tTabW ) {
		if ( !mMinWidthEq.empty() )
			w = eemax( w, getMinSize().getWidth() );
		if ( !mMaxWidthEq.empty() )
			w = eemin( w, getMaxSize().getWidth() );

		if ( textW > w - nonTextW )
			getTextBox()->setMaxWidthEq( String::format( "%.0fdp", w - nonTextW ) );
	}

	setInternalWidth( w );

	if ( getSize().getWidth() != w ) {
		if ( NULL != getTabWidget() )
			getTabWidget()->orderTabs();
	}

	if ( getTextBox()->getTextWidth() > getTextBox()->getSize().getWidth() )
		getTextBox()->setHorizontalAlign( UI_HALIGN_LEFT );
}

std::string UITab::getPropertyString( const PropertyDefinition* propertyDef,
									  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::Owns:
			return mOwnedName;
		default:
			return UISelectButton::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITab::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Text, PropertyId::Owns };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UITab::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				setText( static_cast<UISceneNode*>( mSceneNode )
							 ->getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::Owns:
			mOwnedName = attribute.value();
			setOwnedNode();
			break;
		default:
			return UISelectButton::applyProperty( attribute );
	}

	return true;
}

Uint32 UITab::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_RMASK ) && getTabWidget() &&
		 getTabWidget()->getEnabledCreateContextMenu() ) {
		onCreateContextMenu( position, flags );
	}
	return UISelectButton::onMouseUp( position, flags );
}

bool UITab::onCreateContextMenu( const Vector2i& position, const Uint32& flags ) {
	UITabWidget* tTabW = getTabWidget();
	if ( !tTabW )
		return false;

	if ( tTabW->mCurrentMenu )
		return false;

	UIPopUpMenu* menu = UIPopUpMenu::New();
	ContextMenuEvent event( this, menu, Event::OnCreateContextMenu, position, flags );
	sendEvent( &event );

	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}

	menu->setCloseOnHide( true );
	Vector2f pos( position.asFloat() );
	menu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
	menu->addEventListener( Event::OnClose,
							[tTabW]( const Event* ) { tTabW->mCurrentMenu = nullptr; } );
	tTabW->mCurrentMenu = menu;
	return true;
}

Uint32 UITab::onMessage( const NodeMessage* message ) {
	UITabWidget* tTabW = getTabWidget();
	if ( !tTabW || !mEnabled || !mVisible )
		return 1;

	if ( NULL == mOwnedWidget && !mOwnedName.empty() ) {
		setOwnedNode();
	}

	Uint32 flags = message->getFlags();

	switch ( message->getMsg() ) {
		case NodeMessage::MouseDown: {
			if ( flags & EE_BUTTON_LMASK && message->getSender() != mCloseButton &&
				 getEventDispatcher()->getMouseDownNode() == this ) {
				tTabW->setTabSelected( this );
			}
			break;
		}
		case NodeMessage::MouseUp: {
			if ( flags & EE_BUTTON_LMASK && message->getSender() != mCloseButton ) {
				tTabW->setTabSelected( this );
			} else if ( tTabW->getTabsClosable() && ( flags & EE_BUTTON_MMASK ) ) {
				tTabW->tryCloseTab( this, UITabWidget::FocusTabBehavior::Closest );
			} else if ( flags & EE_BUTTONS_WUWD ) {
				Input* input = getUISceneNode()->getWindow()->getInput();
				if ( input->isModState( KeyMod::getDefaultModifier() ) ) {
					UISlider* slider = tTabW->getTabScroll()->getSlider();
					if ( flags & EE_BUTTON_WUMASK ) {
						slider->setValue( slider->getValue() - slider->getClickStep() );
					} else if ( flags & EE_BUTTON_WDMASK ) {
						slider->setValue( slider->getValue() + slider->getClickStep() );
					}
				} else {
					if ( flags & EE_BUTTON_WUMASK ) {
						tTabW->selectPreviousTab();
					} else if ( flags & EE_BUTTON_WDMASK ) {
						tTabW->selectNextTab();
					}
				}
			}
			break;
		}
		case NodeMessage::MouseClick: {
			if ( flags & EE_BUTTON_LMASK && message->getSender() == mCloseButton ) {
				tTabW->tryCloseTab( this, UITabWidget::FocusTabBehavior::Closest );
			}
			break;
		}
	}

	return 0;
}

void UITab::setOwnedNode() {
	Node* node = getParent()->getParent()->find( mOwnedName );

	if ( NULL != node ) {
		setOwnedWidget( node );
	}
}

void UITab::updateTab() {
	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		setMinWidthEq( tTabW->getMinTabWidth() );
		setMaxWidthEq( tTabW->getMaxTabWidth() );
		if ( mText.size() > tTabW->getMaxTextLength() ) {
			UIPushButton::setText( mText.substr( 0, tTabW->getMaxTextLength() ) );
		} else {
			UIPushButton::setText( mText );
		}
		setDragEnabled( tTabW->getAllowRearrangeTabs() || tTabW->getAllowDragAndDropTabs() );
		onAutoSize();
	}
}

Node* UITab::getOwnedWidget() const {
	return mOwnedWidget;
}

void UITab::setOwnedWidget( Node* ownedWidget ) {
	mOwnedWidget = ownedWidget;

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		tTabW->refreshOwnedWidget( this );

		if ( NULL == tTabW->mTabSelected )
			tTabW->setTabSelected( this );
	}
}

void UITab::setTabSelected() {
	if ( getTabWidget() )
		getTabWidget()->setTabSelected( this );
}

}} // namespace EE::UI
