#include <algorithm>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UITabWidget* UITabWidget::New() {
	return eeNew( UITabWidget, () );
}

UITabWidget::UITabWidget() :
	UIWidget( "tabwidget" ),
	mNodeContainer( NULL ),
	mTabBar( NULL ),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND ),
	mHideTabBarOnSingleTab( false ),
	mAllowRearrangeTabs( false ),
	mAllowDragAndDropTabs( false ),
	mAllowSwitchTabsInEmptySpaces( false ),
	mDroppableHoveringColorWasSet( false ),
	mTabVerticalDragResistance( PixelDensity::dpToPx( 64 ) ) {
	mFlags |=  UI_SCROLLABLE;
	setHorizontalAlign( UI_HALIGN_CENTER );
	setClipType( ClipType::ContentBox );

	mTabBar = UIWidget::NewWithTag( "tabwidget::tabbar" );
	mTabBar->setPixelsSize( mSize.getWidth(), mStyleConfig.TabHeight )
		->setParent( this )
		->setPosition( 0, 0 );

	mNodeContainer = UIWidget::NewWithTag( "tabwidget::container" );
	mNodeContainer
		->setPixelsSize( mSize.getWidth(),
						 mSize.getHeight() - PixelDensity::dpToPx( mStyleConfig.TabHeight ) )
		->setParent( this )
		->setPosition( 0, mStyleConfig.TabHeight );

	mTabScroll = UIScrollBar::NewHorizontalWithTag( "scrollbarmini" );
	mTabScroll->setParent( this );
	mTabScroll->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
	mTabScroll->addEventListener( Event::OnSizeChange, [&]( const Event* ) { updateScrollBar(); } );
	mTabScroll->addEventListener( Event::OnValueChange, [&]( const Event* ) { updateScroll(); } );

	onSizeChange();

	applyDefaultTheme();
}

UITabWidget::~UITabWidget() {}

Uint32 UITabWidget::getType() const {
	return UI_TYPE_TABWIDGET;
}

bool UITabWidget::isType( const Uint32& type ) const {
	return UITabWidget::getType() == type ? true : UIWidget::isType( type );
}

void UITabWidget::setTheme( UITheme* theme ) {
	UIWidget::setTheme( theme );

	mTabBar->setThemeSkin( theme, "tabwidget" );

	mNodeContainer->setThemeSkin( theme, "tabbar" );

	if ( 0 == mStyleConfig.TabHeight ) {
		UISkin* tSkin = theme->getSkin( "tab" );

		if ( NULL != tSkin ) {
			Sizef tSize1 = getSkinSize( tSkin );
			Sizef tSize2 = getSkinSize( tSkin, UIState::StateFlagSelected );

			mStyleConfig.TabHeight = eemax( tSize1.getHeight(), tSize2.getHeight() );

			setContainerSize();
			orderTabs();
		}
	}

	onThemeLoaded();
}

void UITabWidget::onThemeLoaded() {
	onSizeChange();

	UIWidget::onThemeLoaded();
}

void UITabWidget::setContainerSize() {
	if ( getTabCount() < 2 && mHideTabBarOnSingleTab ) {
		mTabBar->setVisible( false );
		mTabBar->setEnabled( false );
		mNodeContainer->setPosition( mPadding.Left, mPadding.Top );
		Sizef s( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
				 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		if ( s != mNodeContainer->getPixelsSize() ) {
			mNodeContainer->setPixelsSize( s );

			for ( auto& tab : mTabs ) {
				refreshOwnedWidget( tab );
			}
		}
	} else {
		mTabBar->setVisible( true );
		mTabBar->setEnabled( true );
		Float totalTabsWidth = mTabs.empty() ? 0
											 : mTabs.back()->getPixelsPosition().x +
												   mTabs.back()->getPixelsSize().getWidth() +
												   mPaddingPx.Left + mPaddingPx.Right;
		Float totalSize = totalTabsWidth > mSize.getWidth() ? totalTabsWidth : mSize.getWidth();
		mTabBar->setPixelsSize( totalSize - mPaddingPx.Left - mPaddingPx.Right,
								PixelDensity::dpToPx( mStyleConfig.TabHeight ) );
		mTabBar->setPosition( mPadding.Left, mPadding.Top );
		mNodeContainer->setPosition( mPadding.Left, mPadding.Top + mStyleConfig.TabHeight );
		Sizef s( mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
				 mSize.getHeight() - PixelDensity::dpToPx( mStyleConfig.TabHeight ) -
					 mPaddingPx.Top - mPaddingPx.Bottom );
		if ( s != mNodeContainer->getPixelsSize() ) {
			mNodeContainer->setPixelsSize( s );

			for ( auto& tab : mTabs ) {
				refreshOwnedWidget( tab );
			}
		}
	}
}

const UITabWidget::StyleConfig& UITabWidget::getStyleConfig() const {
	return mStyleConfig;
}

void UITabWidget::setStyleConfig( const StyleConfig& styleConfig ) {
	Uint32 tabWidgetHeight = mStyleConfig.TabHeight;
	mStyleConfig = styleConfig;
	mStyleConfig.TabHeight = tabWidgetHeight;
	orderTabs();
}

std::string UITabWidget::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MaxTextLength:
			return String::toString( getMaxTextLength() );
		case PropertyId::MinTabWidth:
			return getMinTabWidth();
		case PropertyId::MaxTabWidth:
			return getMaxTabWidth();
		case PropertyId::TabClosable:
			return getTabsClosable() ? "true" : "false";
		case PropertyId::TabsEdgesDiffSkin:
			return getSpecialBorderTabs() ? "true" : "false";
		case PropertyId::TabSeparation:
			return String::fromFloat( getTabSeparation(), "dp" );
		case PropertyId::TabHeight:
			return String::fromFloat( getTabsHeight(), "dp" );
		case PropertyId::TabBarHideOnSingleTab:
			return getHideTabBarOnSingleTab() ? "true" : "false";
		case PropertyId::TabBarAllowRearrange:
			return getAllowRearrangeTabs() ? "true" : "false";
		case PropertyId::TabBarAllowDragAndDrop:
			return getAllowDragAndDropTabs() ? "true" : "false";
		case PropertyId::TabAllowSwitchTabsInEmptySpaces:
			return getAllowSwitchTabsInEmptySpaces() ? "true" : "false";
		case PropertyId::DroppableHoveringColor:
			return !mDroppableHoveringColorWasSet
					   ? UIWidget::getPropertyString( propertyDef, propertyIndex )
					   : getDroppableHoveringColor().toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITabWidget::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::MaxTextLength,
				   PropertyId::MinTabWidth,
				   PropertyId::MaxTabWidth,
				   PropertyId::TabClosable,
				   PropertyId::TabsEdgesDiffSkin,
				   PropertyId::TabSeparation,
				   PropertyId::TabHeight,
				   PropertyId::TabBarHideOnSingleTab,
				   PropertyId::TabBarAllowRearrange,
				   PropertyId::TabBarAllowDragAndDrop,
				   PropertyId::TabAllowSwitchTabsInEmptySpaces,
				   PropertyId::DroppableHoveringColor };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UITabWidget::isDrawInvalidator() const {
	return true;
}

void UITabWidget::invalidate( Node* invalidator ) {
	// Only invalidate if the invalidator is actually visible in the current active tab.
	if ( NULL != invalidator ) {
		if ( invalidator == mTabScroll || mTabScroll->inParentTreeOf( invalidator ) ) {
			if ( NULL != mNodeDrawInvalidator ) {
				mNodeDrawInvalidator->invalidate( this );
			} else if ( NULL != mSceneNode ) {
				mSceneNode->invalidate( this );
			}
		} else if ( invalidator == mNodeContainer || invalidator == mTabBar ||
					mTabBar->inParentTreeOf( invalidator ) ) {
			mNodeDrawInvalidator->invalidate( mNodeContainer );
		} else if ( invalidator->getParent() == mNodeContainer ) {
			if ( invalidator->isVisible() )
				mNodeDrawInvalidator->invalidate( mNodeContainer );
		} else {
			Node* container = invalidator->getParent();
			while ( container->getParent() != NULL && container->getParent() != mNodeContainer ) {
				container = container->getParent();
			}
			if ( container->getParent() == mNodeContainer && container->isVisible() ) {
				mNodeDrawInvalidator->invalidate( mNodeContainer );
			}
		}
	} else if ( NULL != mNodeDrawInvalidator ) {
		mNodeDrawInvalidator->invalidate( this );
	} else if ( NULL != mSceneNode ) {
		mSceneNode->invalidate( this );
	}
}

bool UITabWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::MaxTextLength:
			setMaxTextLength( attribute.asUint( 1 ) );
			break;
		case PropertyId::MinTabWidth:
			setMinTabWidth( attribute.asString() );
			break;
		case PropertyId::MaxTabWidth:
			setMaxTabWidth( attribute.asString() );
			break;
		case PropertyId::TabClosable:
			setTabsClosable( attribute.asBool() );
			break;
		case PropertyId::TabsEdgesDiffSkin:
			setTabsEdgesDiffSkins( attribute.asBool() );
			break;
		case PropertyId::TabSeparation:
			setTabSeparation( attribute.asDpDimension( this ) );
			break;
		case PropertyId::TabHeight:
			setTabsHeight( attribute.asDpDimension( this ) );
			break;
		case PropertyId::TabBarHideOnSingleTab:
			setHideTabBarOnSingleTab( attribute.asBool() );
			break;
		case PropertyId::TabBarAllowRearrange:
			setAllowRearrangeTabs( attribute.asBool() );
			break;
		case PropertyId::TabBarAllowDragAndDrop:
			setAllowDragAndDropTabs( attribute.asBool() );
			break;
		case PropertyId::TabAllowSwitchTabsInEmptySpaces:
			setAllowSwitchTabsInEmptySpaces( attribute.asBool() );
			break;
		case PropertyId::DroppableHoveringColor:
			setDroppableHoveringColor( attribute.asColor() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

Float UITabWidget::getTabSeparation() const {
	return mStyleConfig.TabSeparation;
}

void UITabWidget::setTabSeparation( const Float& tabSeparation ) {
	if ( tabSeparation != mStyleConfig.TabSeparation ) {
		mStyleConfig.TabSeparation = tabSeparation;
		posTabs();
	}
}

Uint32 UITabWidget::getMaxTextLength() const {
	return mStyleConfig.MaxTextLength;
}

void UITabWidget::setMaxTextLength( const Uint32& maxTextLength ) {
	if ( maxTextLength != mStyleConfig.MaxTextLength ) {
		mStyleConfig.MaxTextLength = maxTextLength;
		updateTabs();
		invalidateDraw();
	}
}

std::string UITabWidget::getMinTabWidth() const {
	return mStyleConfig.MinTabWidth;
}

void UITabWidget::setMinTabWidth( const std::string& minTabWidth ) {
	if ( minTabWidth != mStyleConfig.MinTabWidth ) {
		mStyleConfig.MinTabWidth = minTabWidth;
		updateTabs();
		invalidateDraw();
	}
}

std::string UITabWidget::getMaxTabWidth() const {
	return mStyleConfig.MaxTabWidth;
}

void UITabWidget::setMaxTabWidth( const std::string& maxTabWidth ) {
	if ( maxTabWidth != mStyleConfig.MaxTabWidth ) {
		mStyleConfig.MaxTabWidth = maxTabWidth;
		updateTabs();
		invalidateDraw();
	}
}

bool UITabWidget::getTabsClosable() const {
	return mStyleConfig.TabsClosable;
}

void UITabWidget::setTabsClosable( bool tabsClosable ) {
	if ( mStyleConfig.TabsClosable != tabsClosable ) {
		mStyleConfig.TabsClosable = tabsClosable;
		updateTabs();
		invalidateDraw();
	}
}

bool UITabWidget::getSpecialBorderTabs() const {
	return mStyleConfig.TabsEdgesDiffSkins;
}

void UITabWidget::setTabsEdgesDiffSkins( bool diffSkins ) {
	if ( mStyleConfig.TabsEdgesDiffSkins != diffSkins ) {
		mStyleConfig.TabsEdgesDiffSkins = diffSkins;
		applyThemeToTabs();
	}
}

void UITabWidget::setTabsHeight( const Float& height ) {
	if ( height != mStyleConfig.TabHeight ) {
		mStyleConfig.TabHeight = height;
		setContainerSize();
		orderTabs();
		updateScrollBar();
	}
}

Float UITabWidget::getTabsHeight() const {
	return mStyleConfig.TabHeight;
}

void UITabWidget::posTabs() {
	Int32 x = 0;
	Int32 y = 0;
	Int32 alignOffset = 0;
	Uint32 tabsWidth = 0;

	if ( !mTabs.empty() ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			tabsWidth += mTabs[i]->getPixelsSize().getWidth() + mStyleConfig.TabSeparation;
		}

		tabsWidth -= mStyleConfig.TabSeparation;
	}

	switch ( Font::getHorizontalAlign( mFlags ) ) {
		case UI_HALIGN_LEFT:
			alignOffset = 0;
			break;
		case UI_HALIGN_CENTER:
			alignOffset = ( getPixelsSize().getWidth() - tabsWidth ) / 2;
			break;
		case UI_HALIGN_RIGHT:
			alignOffset = getSize().getWidth() - tabsWidth;
			break;
	}

	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		switch ( Font::getVerticalAlign( mFlags ) ) {
			case UI_VALIGN_BOTTOM:
				y = PixelDensity::dpToPx( mStyleConfig.TabHeight ) -
					mTabs[i]->getPixelsSize().getHeight();
				break;
			case UI_VALIGN_TOP:
				y = 0;
				break;
			case UI_VALIGN_CENTER:
				y = ( PixelDensity::dpToPx( mStyleConfig.TabHeight ) -
					  mTabs[i]->getPixelsSize().getHeight() ) /
					2;
				break;
		}

		if ( !mTabs[i]->isDragging() ) {
			mTabs[i]->setPixelsPosition( alignOffset + x, y );
		}

		x += mTabs[i]->getPixelsSize().getWidth() + mStyleConfig.TabSeparation;

		if ( mStyleConfig.TabHeight != 0 && mTabs[i]->getPixelsSize().getHeight() == 0 ) {
			mTabs[i]->setPixelsSize( mTabs[i]->getPixelsSize().getWidth(),
									 PixelDensity::dpToPx( mStyleConfig.TabHeight ) );
		}
	}

	updateScrollBar();
}

void UITabWidget::zorderTabs() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		mTabs[i]->toBack();
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->toFront();
	}
}

void UITabWidget::orderTabs() {
	applyThemeToTabs();

	setContainerSize();

	zorderTabs();

	posTabs();

	invalidateDraw();
}

void UITabWidget::updateTabs() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		mTabs[i]->updateTab();
	}
}

UITab* UITabWidget::createTab( const String& text, UINode* nodeOwned, Drawable* icon ) {
	UITab* tab = UITab::New();
	tab->setParent( mTabBar );
	tab->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE );
	tab->setIcon( icon );
	tab->setText( text );
	tab->setVisible( true );
	tab->setEnabled( true );
	tab->setOwnedWidget( nodeOwned );
	nodeOwned->setParent( mNodeContainer );
	nodeOwned->setVisible( false );
	nodeOwned->setEnabled( true );

	if ( nodeOwned->isWidget() ) {
		UIWidget* widgetOwned = static_cast<UIWidget*>( nodeOwned );

		widgetOwned->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	return tab;
}

UITab* UITabWidget::add( const String& text, UINode* nodeOwned, Drawable* icon ) {
	UITab* tab = createTab( text, nodeOwned, icon );
	add( tab );
	return tab;
}

UITabWidget* UITabWidget::add( UITab* tab ) {
	tab->setParent( mTabBar );

	mTabs.push_back( tab );

	if ( NULL == mTabSelected ) {
		setTabSelected( tab );
	} else {
		orderTabs();
	}

	TabEvent tabEvent( this, tab, getTabIndex( tab ), Event::OnTabAdded );
	sendEvent( &tabEvent );

	return this;
}

UITab* UITabWidget::getTabFromOwnedWidget( const UIWidget* widget ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i]->isType( UI_TYPE_TAB ) ) {
			UITab* tTab = mTabs[i]->asType<UITab>();

			if ( tTab->getOwnedWidget() == widget )
				return tTab;
		}
	}
	return nullptr;
}

UITab* UITabWidget::getTab( const Uint32& index ) {
	eeASSERT( index < mTabs.size() );
	return mTabs[index];
}

UITab* UITabWidget::getTab( const String& text ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i]->isType( UI_TYPE_TAB ) ) {
			UITab* tTab = mTabs[i]->asType<UITab>();

			if ( tTab->getText() == text )
				return tTab;
		}
	}

	return NULL;
}

Uint32 UITabWidget::getTabIndex( UITab* tab ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i] == tab )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UITabWidget::getTabCount() const {
	return mTabs.size();
}

void UITabWidget::removeTab( const Uint32& index, bool destroyOwnedNode, bool immediateClose ) {
	removeTab( index, destroyOwnedNode, true, immediateClose );
}

void UITabWidget::removeTab( const Uint32& index, bool destroyOwnedNode, bool destroyTab,
							 bool immediateClose ) {
	eeASSERT( index < mTabs.size() );

	UITab* tab = mTabs[index];

	if ( destroyTab && !immediateClose ) {
		tab->close();
		tab->setVisible( false );
		tab->setEnabled( false );
	}

	if ( destroyOwnedNode && !immediateClose ) {
		tab->getOwnedWidget()->close();
		tab->getOwnedWidget()->setVisible( false );
		tab->getOwnedWidget()->setEnabled( false );
	}

	mTabs.erase( mTabs.begin() + index );

	if ( index == mTabSelectedIndex ) {
		if ( !mTabs.empty() ) {
			if ( mTabSelectedIndex < mTabs.size() ) {
				setTabSelected( mTabs[mTabSelectedIndex] );
			} else {
				if ( mTabSelectedIndex > 0 && mTabSelectedIndex - 1 < mTabs.size() ) {
					setTabSelected( mTabs[mTabSelectedIndex - 1] );
				} else {
					setTabSelected( mTabs[0] );
				}
			}
		} else {
			mTabSelected = NULL;
			mTabSelectedIndex = eeINDEX_NOT_FOUND;
		}
	} else {
		mTabSelectedIndex = getTabIndex( mTabSelected );
	}

	orderTabs();

	updateScrollBar();
	updateScroll();

	TabEvent tabEvent( this, tab, index, Event::OnTabClosed );
	sendEvent( &tabEvent );

	if ( destroyOwnedNode && immediateClose ) {
		eeDelete( tab->getOwnedWidget() );
	}

	if ( destroyTab && immediateClose ) {
		eeSAFE_DELETE( tab );
	}
}

void UITabWidget::removeTab( UITab* tab, bool destroyOwnedNode, bool immediateClose ) {
	removeTab( getTabIndex( tab ), destroyOwnedNode, true, immediateClose );
}

void UITabWidget::removeTab( UITab* tab, bool destroyOwnedNode, bool destroyTab,
							 bool immediateClose ) {
	removeTab( getTabIndex( tab ), destroyOwnedNode, destroyTab, immediateClose );
}

void UITabWidget::removeAllTabs( bool destroyOwnedNode, bool immediateClose ) {
	std::deque<UITab*> tabs = mTabs;

	if ( immediateClose ) {
		for ( Uint32 i = 0; i < tabs.size(); i++ ) {
			if ( NULL != tabs[i] ) {
				TabEvent tabEvent( this, tabs[i], i, Event::OnTabClosed );
				sendEvent( &tabEvent );
			}
		}
	}

	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( NULL != mTabs[i] ) {
			if ( !immediateClose ) {
				mTabs[i]->close();
				mTabs[i]->setVisible( false );
				mTabs[i]->setEnabled( false );
				if ( destroyOwnedNode ) {
					mTabs[i]->getOwnedWidget()->close();
					mTabs[i]->getOwnedWidget()->setVisible( false );
					mTabs[i]->getOwnedWidget()->setEnabled( false );
				}
			} else {
				if ( destroyOwnedNode )
					eeDelete( mTabs[i]->getOwnedWidget() );
				eeDelete( mTabs[i] );
			}
		}
	}

	mTabs.clear();

	mTabSelected = NULL;
	mTabSelectedIndex = eeINDEX_NOT_FOUND;

	orderTabs();

	if ( !immediateClose ) {
		for ( Uint32 i = 0; i < tabs.size(); i++ ) {
			if ( NULL != tabs[i] ) {
				TabEvent tabEvent( this, tabs[i], i, Event::OnTabClosed );
				sendEvent( &tabEvent );
			}
		}
	}
}

void UITabWidget::insertTab( const String& text, UINode* nodeOwned, Drawable* icon,
							 const Uint32& index ) {
	insertTab( createTab( text, nodeOwned, icon ), index );
}

void UITabWidget::insertTab( UITab* Tab, const Uint32& index ) {
	mTabs.insert( mTabs.begin() + index, Tab );

	childAddAt( Tab, index );

	orderTabs();
}

UITab* UITabWidget::setTabSelected( UITab* tab ) {
	if ( NULL == tab )
		return NULL;

	if ( std::find( mTabs.begin(), mTabs.end(), tab ) == mTabs.end() )
		return NULL;

	invalidateDraw();

	if ( tab == mTabSelected ) {
		refreshOwnedWidget( tab );
		if ( tab->getOwnedWidget() )
			tab->getOwnedWidget()->setFocus();
		return tab;
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->unselect();

		if ( NULL != mTabSelected->getOwnedWidget() ) {
			mTabSelected->getOwnedWidget()->setVisible( false );
			mTabSelected->getOwnedWidget()->setEnabled( false );
		}
	}

	tab->select();
	if ( tab->getOwnedWidget() )
		tab->getOwnedWidget()->setFocus();

	Uint32 tabIndex = getTabIndex( tab );

	if ( eeINDEX_NOT_FOUND != tabIndex ) {
		mTabSelected = tab;
		mTabSelectedIndex = tabIndex;

		refreshOwnedWidget( mTabSelected );

		orderTabs();

		sendCommonEvent( Event::OnTabSelected );
	}

	updateScroll();

	return tab;
}

UITab* UITabWidget::setTabSelected( const Uint32& tabIndex ) {
	if ( tabIndex < mTabs.size() ) {
		return setTabSelected( mTabs[tabIndex] );
	}
	return NULL;
}

const bool& UITabWidget::getHideTabBarOnSingleTab() const {
	return mHideTabBarOnSingleTab;
}

void UITabWidget::setHideTabBarOnSingleTab( const bool& hideTabBarOnSingleTab ) {
	if ( mHideTabBarOnSingleTab != hideTabBarOnSingleTab ) {
		mHideTabBarOnSingleTab = hideTabBarOnSingleTab;
		setContainerSize();
	}
}

const UITabWidget::TabTryCloseCallback& UITabWidget::getTabTryCloseCallback() const {
	return mTabTryCloseCallback;
}

void UITabWidget::setTabTryCloseCallback( const TabTryCloseCallback& tabTryCloseCallback ) {
	mTabTryCloseCallback = tabTryCloseCallback;
}

const bool& UITabWidget::getAllowRearrangeTabs() const {
	return mAllowRearrangeTabs;
}

void UITabWidget::setAllowRearrangeTabs( bool allowRearrangeTabs ) {
	if ( allowRearrangeTabs != mAllowRearrangeTabs ) {
		mAllowRearrangeTabs = allowRearrangeTabs;
		updateTabs();
	}
}

bool UITabWidget::getAllowDragAndDropTabs() const {
	return mAllowDragAndDropTabs;
}

void UITabWidget::setAllowDragAndDropTabs( bool allowDragAndDropTabs ) {
	mAllowDragAndDropTabs = allowDragAndDropTabs;
}

const Float& UITabWidget::getTabVerticalDragResistance() const {
	return mTabVerticalDragResistance;
}

void UITabWidget::setTabVerticalDragResistance( const Float& tabVerticalDragResistance ) {
	mTabVerticalDragResistance = tabVerticalDragResistance;
}

bool UITabWidget::getAllowSwitchTabsInEmptySpaces() const {
	return mAllowSwitchTabsInEmptySpaces;
}

void UITabWidget::setAllowSwitchTabsInEmptySpaces( bool allowSwitchTabsInEmptySpaces ) {
	mAllowSwitchTabsInEmptySpaces = allowSwitchTabsInEmptySpaces;
}

bool UITabWidget::acceptsDropOfWidget( const UIWidget* widget ) {
	return mAllowDragAndDropTabs && widget && UI_TYPE_TAB == widget->getType() &&
		   !isParentOf( widget ) && widget->asConstType<UITab>()->getTabWidget() != this;
}

const Color& UITabWidget::getDroppableHoveringColor() const {
	return mDroppableHoveringColor;
}

void UITabWidget::setDroppableHoveringColor( const Color& droppableHoveringColor ) {
	mDroppableHoveringColor = droppableHoveringColor;
	mDroppableHoveringColorWasSet = true;
}

void UITabWidget::refreshOwnedWidget( UITab* tab ) {
	if ( NULL != tab && NULL != tab->getOwnedWidget() ) {
		tab->getOwnedWidget()->setParent( mNodeContainer );
		tab->getOwnedWidget()->setVisible( tab == mTabSelected );
		tab->getOwnedWidget()->setEnabled( tab == mTabSelected );
		tab->getOwnedWidget()->setSize( mNodeContainer->getSize() );

		if ( tab->getOwnedWidget()->isWidget() ) {
			UIWidget* widget = static_cast<UIWidget*>( tab->getOwnedWidget() );

			widget->setPosition( widget->getLayoutMargin().Left, widget->getLayoutMargin().Top );
		} else {
			tab->getOwnedWidget()->setPosition( 0, 0 );
		}
	}
}

void UITabWidget::tryCloseTab( UITab* tab ) {
	if ( mTabTryCloseCallback && !mTabTryCloseCallback( tab ) )
		return;
	removeTab( tab );
}

void UITabWidget::swapTabs( UITab* left, UITab* right ) {
	Uint32 leftIndex = getTabIndex( left );
	Uint32 rightIndex = getTabIndex( right );
	if ( leftIndex != eeINDEX_NOT_FOUND && rightIndex != eeINDEX_NOT_FOUND ) {
		if ( leftIndex == mTabSelectedIndex )
			mTabSelectedIndex = rightIndex;
		else if ( rightIndex == mTabSelectedIndex )
			mTabSelectedIndex = leftIndex;
		mTabs[leftIndex] = right;
		mTabs[rightIndex] = left;
		posTabs();
	}
}

void UITabWidget::selectPreviousTab() {
	if ( eeINDEX_NOT_FOUND != mTabSelectedIndex && mTabSelectedIndex > 0 ) {
		setTabSelected( getTab( mTabSelectedIndex - 1 ) );
	}
}

void UITabWidget::selectNextTab() {
	if ( mTabSelectedIndex + 1 < mTabs.size() ) {
		setTabSelected( getTab( mTabSelectedIndex + 1 ) );
	}
}

UITab* UITabWidget::getTabSelected() const {
	return mTabSelected;
}

Uint32 UITabWidget::getTabSelectedIndex() const {
	return mTabSelectedIndex;
}

void UITabWidget::onSizeChange() {
	setContainerSize();
	posTabs();

	if ( NULL != mTabSelected && NULL != mTabSelected->getOwnedWidget() ) {
		mTabSelected->getOwnedWidget()->setSize( mNodeContainer->getSize() );
	}

	updateScrollBar();
	updateScroll();

	UIWidget::onSizeChange();
}

void UITabWidget::onChildCountChange( Node* child, const bool& removed ) {
	if ( !removed && child != mTabBar && child != mNodeContainer && child != mTabScroll ) {
		if ( child->isType( UI_TYPE_TAB ) ) {
			// This must be a tab that was dragging.
			if ( std::find( mTabs.begin(), mTabs.end(), child->asType<UITab>() ) != mTabs.end() )
				return;

			UITab* tab = static_cast<UITab*>( child );

			tab->setParent( mTabBar );

			mTabs.push_back( tab );

			if ( NULL == mTabSelected ) {
				setTabSelected( tab );
			} else {
				orderTabs();
			}

			TabEvent tabEvent( this, tab, getTabIndex( tab ), Event::OnTabAdded );
			sendEvent( &tabEvent );
		} else {
			child->setParent( mNodeContainer );
			child->setVisible( false );
			child->setEnabled( true );
		}
	}

	UIWidget::onChildCountChange( child, removed );
}

void UITabWidget::onPaddingChange() {
	onSizeChange();

	UIWidget::onPaddingChange();
}

Uint32 UITabWidget::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::Drop && mAllowDragAndDropTabs ) {
		const NodeDropMessage* dropMsg = static_cast<const NodeDropMessage*>( msg );
		if ( dropMsg->getDroppedNode()->isType( UI_TYPE_TAB ) ) {
			UITab* tab = dropMsg->getDroppedNode()->asType<UITab>();
			if ( tab->getTabWidget() != this && tab->getTabWidget()->getAllowDragAndDropTabs() ) {
				tab->getTabWidget()->removeTab( tab, false, false, false );
				add( tab );
				setTabSelected( tab );
				return 1;
			}
		}
	} else if ( msg->getMsg() == NodeMessage::MouseUp && mAllowSwitchTabsInEmptySpaces &&
				msg->getSender() == mTabBar ) {
		Uint32 flags = msg->getFlags();
		if ( flags & EE_BUTTONS_WUWD ) {
			if ( flags & EE_BUTTON_WUMASK ) {
				selectPreviousTab();
			} else if ( flags & EE_BUTTON_WDMASK ) {
				selectNextTab();
			}
		}
	}
	return 0;
}

void UITabWidget::applyThemeToTabs() {
	if ( mStyleConfig.TabsEdgesDiffSkins ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			mTabs[i]->applyDefaultTheme();
		}
	}
}

UIWidget* UITabWidget::getTabBar() const {
	return mTabBar;
}

UIWidget* UITabWidget::getContainerNode() const {
	return mNodeContainer;
}

void UITabWidget::updateScrollBar() {
	mTabScroll->setPixelsSize(
		{ getPixelsSize().getWidth(), mTabScroll->getPixelsSize().getHeight() } );
	mTabScroll->setPixelsPosition(
		{ 0, mTabBar->getPixelsSize().getHeight() - mTabScroll->getPixelsSize().getHeight() } );

	Float totalSize = mTabs.empty() ? 0
									: mTabs.back()->getPixelsPosition().x +
										  mTabs.back()->getPixelsSize().getWidth();
	mTabScroll->setVisible( totalSize > getPixelsSize().getWidth() );

	if ( mTabScroll->isVisible() ) {
		mTabScroll->setMaxValue( totalSize - mSize.getWidth() );
		mTabScroll->setClickStep( eefloor( mTabScroll->getMaxValue() * 0.2f ) );
		mTabScroll->setPageStep( ( totalSize - mSize.getWidth() ) *
								 ( mSize.getWidth() / totalSize ) );
	}
}

void UITabWidget::updateScroll() {
	if ( mTabScroll->isVisible() )
		mTabBar->setPixelsPosition(
			{ mPaddingPx.Left + -mTabScroll->getValue(), mTabBar->getPixelsPosition().y } );
}

}} // namespace EE::UI
