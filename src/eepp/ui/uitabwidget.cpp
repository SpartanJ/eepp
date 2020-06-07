#include <algorithm>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UITabWidget* UITabWidget::New() {
	return eeNew( UITabWidget, () );
}

UITabWidget::UITabWidget() :
	UIWidget( "tabwidget" ),
	mCtrlContainer( NULL ),
	mTabBar( NULL ),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND ),
	mHideTabBarOnSingleTab( false ),
	mAllowRearrangeTabs( false ) {
	setHorizontalAlign( UI_HALIGN_CENTER );

	mTabBar = UIWidget::NewWithTag( "tabwidget::tabbar" );
	mTabBar->setPixelsSize( mSize.getWidth(), mStyleConfig.TabHeight )
		->setParent( this )
		->setPosition( 0, 0 );
	mTabBar->clipEnable();

	mCtrlContainer = UIWidget::NewWithTag( "tabwidget::container" );
	mCtrlContainer
		->setPixelsSize( mSize.getWidth(),
						 mSize.getHeight() - PixelDensity::dpToPx( mStyleConfig.TabHeight ) )
		->setParent( this )
		->setPosition( 0, mStyleConfig.TabHeight );
	mCtrlContainer->clipEnable();

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

void UITabWidget::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	mTabBar->setThemeSkin( Theme, "tabwidget" );

	mCtrlContainer->setThemeSkin( Theme, "tabbar" );

	if ( 0 == mStyleConfig.TabHeight ) {
		UISkin* tSkin = Theme->getSkin( "tab" );

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
		mCtrlContainer->setPosition( mPadding.Left, mPadding.Top );
		Sizef s( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
				 mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
		if ( s != mCtrlContainer->getPixelsSize() ) {
			mCtrlContainer->setPixelsSize( s );

			for ( auto& tab : mTabs ) {
				refreshOwnedWidget( tab );
			}
		}
	} else {
		mTabBar->setVisible( true );
		mTabBar->setEnabled( true );
		mTabBar->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
								PixelDensity::dpToPx( mStyleConfig.TabHeight ) );
		mTabBar->setPosition( mPadding.Left, mPadding.Top );
		mCtrlContainer->setPosition( mPadding.Left, mPadding.Top + mStyleConfig.TabHeight );
		Sizef s( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
				 mSize.getHeight() - PixelDensity::dpToPx( mStyleConfig.TabHeight ) -
					 mRealPadding.Top - mRealPadding.Bottom );
		if ( s != mCtrlContainer->getPixelsSize() ) {
			mCtrlContainer->setPixelsSize( s );

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
											const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MaxTextLength:
			return String::toStr( getMaxTextLength() );
		case PropertyId::MinTabWidth:
			return String::fromFloat( getMinTabWidth(), "dp" );
		case PropertyId::MaxTabWidth:
			return String::fromFloat( getMaxTabWidth(), "dp" );
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
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UITabWidget::isDrawInvalidator() const {
	return true;
}

void UITabWidget::invalidate( Node* invalidator ) {
	// Only invalidate if the invalidator is actually visible in the current active tab.
	if ( NULL != invalidator ) {
		if ( invalidator == mCtrlContainer || invalidator == mTabBar ||
			 mTabBar->inParentTreeOf( invalidator ) ) {
			mNodeDrawInvalidator->invalidate( mCtrlContainer );
		} else if ( invalidator->getParent() == mCtrlContainer ) {
			if ( invalidator->isVisible() ) {
				mNodeDrawInvalidator->invalidate( mCtrlContainer );
			}
		} else {
			Node* container = invalidator->getParent();
			while ( container->getParent() != NULL && container->getParent() != mCtrlContainer ) {
				container = container->getParent();
			}
			if ( container->getParent() == mCtrlContainer && container->isVisible() ) {
				mNodeDrawInvalidator->invalidate( mCtrlContainer );
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
			setMinTabWidth( attribute.asDpDimension( this, "1" ) );
			break;
		case PropertyId::MaxTabWidth:
			setMaxTabWidth( attribute.asDpDimension( this ) );
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

Float UITabWidget::getMinTabWidth() const {
	return mStyleConfig.MinTabWidth;
}

void UITabWidget::setMinTabWidth( const Float& minTabWidth ) {
	if ( minTabWidth != mStyleConfig.MinTabWidth ) {
		mStyleConfig.MinTabWidth = minTabWidth;
		updateTabs();
		invalidateDraw();
	}
}

Float UITabWidget::getMaxTabWidth() const {
	return mStyleConfig.MaxTabWidth;
}

void UITabWidget::setMaxTabWidth( const Float& maxTabWidth ) {
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
	}
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
	nodeOwned->setParent( mCtrlContainer );
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

UITabWidget* UITabWidget::add( UITab* Tab ) {
	Tab->setParent( mTabBar );

	mTabs.push_back( Tab );

	if ( NULL == mTabSelected ) {
		setTabSelected( Tab );
	} else {
		orderTabs();
	}

	return this;
}

UITab* UITabWidget::getTab( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );
	return mTabs[Index];
}

UITab* UITabWidget::getTab( const String& Text ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i]->isType( UI_TYPE_TAB ) ) {
			UITab* tTab = mTabs[i]->asType<UITab>();

			if ( tTab->getText() == Text )
				return tTab;
		}
	}

	return NULL;
}

Uint32 UITabWidget::getTabIndex( UITab* Tab ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i] == Tab )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UITabWidget::getTabCount() const {
	return mTabs.size();
}

void UITabWidget::removeTab( const Uint32& index, bool destroyOwnedNode ) {
	eeASSERT( index < mTabs.size() );

	UITab* tab = mTabs[index];

	tab->close();
	tab->setVisible( false );
	tab->setEnabled( false );
	if ( destroyOwnedNode ) {
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

	TabEvent tabEvent( this, tab, Event::OnTabClosed );
	sendEvent( &tabEvent );
}

void UITabWidget::removeTab( UITab* Tab, bool destroyOwnedNode ) {
	removeTab( getTabIndex( Tab ), destroyOwnedNode );
}

void UITabWidget::removeAllTabs( bool destroyOwnedNode ) {
	std::deque<UITab*> tabs = mTabs;

	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( NULL != mTabs[i] ) {
			mTabs[i]->close();
			mTabs[i]->setVisible( false );
			mTabs[i]->setEnabled( false );
			if ( destroyOwnedNode ) {
				mTabs[i]->getOwnedWidget()->close();
				mTabs[i]->getOwnedWidget()->setVisible( false );
				mTabs[i]->getOwnedWidget()->setEnabled( false );
			}
		}
	}

	mTabs.clear();

	mTabSelected = NULL;
	mTabSelectedIndex = eeINDEX_NOT_FOUND;

	orderTabs();

	for ( Uint32 i = 0; i < tabs.size(); i++ ) {
		if ( NULL != tabs[i] ) {
			TabEvent tabEvent( this, tabs[i], Event::OnTabClosed );
			sendEvent( &tabEvent );
		}
	}
}

void UITabWidget::insertTab( const String& Text, UINode* CtrlOwned, Drawable* Icon,
							 const Uint32& Index ) {
	insertTab( createTab( Text, CtrlOwned, Icon ), Index );
}

void UITabWidget::insertTab( UITab* Tab, const Uint32& Index ) {
	mTabs.insert( mTabs.begin() + Index, Tab );

	childAddAt( Tab, Index );

	orderTabs();
}

UITab* UITabWidget::setTabSelected( UITab* Tab ) {
	if ( NULL == Tab )
		return NULL;

	if ( std::find( mTabs.begin(), mTabs.end(), Tab ) == mTabs.end() )
		return NULL;

	invalidateDraw();

	if ( Tab == mTabSelected ) {
		refreshOwnedWidget( Tab );
		if ( Tab->getOwnedWidget() )
			Tab->getOwnedWidget()->setFocus();
		return Tab;
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->unselect();

		if ( NULL != mTabSelected->getOwnedWidget() ) {
			mTabSelected->getOwnedWidget()->setVisible( false );
			mTabSelected->getOwnedWidget()->setEnabled( false );
		}
	}

	Tab->select();
	if ( Tab->getOwnedWidget() )
		Tab->getOwnedWidget()->setFocus();

	Uint32 TabIndex = getTabIndex( Tab );

	if ( eeINDEX_NOT_FOUND != TabIndex ) {
		mTabSelected = Tab;
		mTabSelectedIndex = TabIndex;

		refreshOwnedWidget( mTabSelected );

		orderTabs();

		sendCommonEvent( Event::OnTabSelected );
	}

	return Tab;
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

void UITabWidget::refreshOwnedWidget( UITab* tab ) {
	if ( NULL != tab && NULL != tab->getOwnedWidget() ) {
		tab->getOwnedWidget()->setParent( mCtrlContainer );
		tab->getOwnedWidget()->setVisible( tab == mTabSelected );
		tab->getOwnedWidget()->setEnabled( tab == mTabSelected );
		tab->getOwnedWidget()->setSize( mCtrlContainer->getSize() );

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
		mTabSelected->getOwnedWidget()->setSize( mCtrlContainer->getSize() );
	}

	UIWidget::onSizeChange();
}

void UITabWidget::onChildCountChange( Node* child, const bool& removed ) {
	if ( !removed && child != mTabBar && child != mCtrlContainer ) {
		if ( child->isType( UI_TYPE_TAB ) ) {
			UITab* Tab = static_cast<UITab*>( child );

			Tab->setParent( mTabBar );

			mTabs.push_back( Tab );

			if ( NULL == mTabSelected ) {
				setTabSelected( Tab );
			} else {
				orderTabs();
			}
		} else {
			child->setParent( mCtrlContainer );
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

UIWidget* UITabWidget::getControlContainer() const {
	return mCtrlContainer;
}

}} // namespace EE::UI
