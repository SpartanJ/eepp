#include <algorithm>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/uinodelink.hpp>

#include <nlohmann/json.hpp>

namespace EE { namespace UI { namespace Tools {

UITabWidgetSplitter* UITabWidgetSplitter::New( UITabWidgetSplitter::Client* client,
											   UISceneNode* sceneNode ) {
	return eeNew( UITabWidgetSplitter, ( client, sceneNode ) );
}

UITabWidgetSplitter::UITabWidgetSplitter( UITabWidgetSplitter::Client* client,
										  UISceneNode* sceneNode ) :
	mUISceneNode( sceneNode ), mClient( client ) {}

UITabWidgetSplitter::~UITabWidgetSplitter() {}

UITabWidget* UITabWidgetSplitter::tabWidgetFromWidget( UIWidget* widget ) const {
	if ( widget )
		return ( (UITab*)widget->getData() )->getTabWidget();
	return nullptr;
}

UISplitter* UITabWidgetSplitter::splitterFromWidget( UIWidget* widget ) const {
	if ( widget && widget->getParent() && widget->getParent()->getParent() &&
		 widget->getParent()->getParent()->getParent() &&
		 widget->getParent()->getParent()->getParent()->isType( UI_TYPE_SPLITTER ) )
		return widget->getParent()->getParent()->getParent()->asType<UISplitter>();
	return nullptr;
}

void UITabWidgetSplitter::setCurrentWidget( UIWidget* curWidget ) {
	if ( curWidget == nullptr ) {
		auto widget = getSomeWidget();
		if ( widget )
			setCurrentWidget( widget );
		return;
	}
	bool isNewW = mCurWidget != curWidget;
	mCurWidget = curWidget;
	if ( isNewW )
		mClient->onWidgetFocusChange( curWidget );
}

std::pair<UITab*, UIWidget*>
UITabWidgetSplitter::createWidgetInTabWidget( UITabWidget* tabWidget, UIWidget* widget,
											  const std::string& tabName, bool focus ) {
	eeASSERT( curWidgetExists() );
	if ( nullptr == tabWidget )
		return std::make_pair( (UITab*)nullptr, (UIWidget*)nullptr );
	UITab* tab = tabWidget->add( tabName, widget );
	widget->setData( (UintPtr)tab );
	widget->on( Event::OnFocusWithin, [this]( const Event* event ) {
		setCurrentWidget( event->getNode()->asType<UIWidget>() );
	} );
	widget->on( Event::OnTitleChange, [this]( const Event* event ) {
		const TextEvent* tevent = static_cast<const TextEvent*>( event );
		UIWidget* widget = event->getNode()->asType<UIWidget>();
		UITabWidget* tabWidget = tabWidgetFromWidget( widget );
		UITab* tab = tabWidget->getTabFromOwnedWidget( widget );
		if ( !tab )
			return;
		tab->setText( tevent->getText() );
	} );
	if ( focus )
		tabWidget->setTabSelected( tab );
	mClient->onTabCreated( tab, widget );
	return std::make_pair( tab, widget );
}

std::pair<UITab*, UIWidget*>
UITabWidgetSplitter::createWidget( UIWidget* widget, const std::string& tabName, bool focus ) {
	UITabWidget* tabWidget = nullptr;

	UIWidget* curWidget = getCurWidget();
	if ( !curWidget )
		return std::make_pair( (UITab*)nullptr, (UIWidget*)nullptr );
	tabWidget = tabWidgetFromWidget( curWidget );

	if ( !tabWidget ) {
		if ( !getTabWidgets().empty() ) {
			tabWidget = getTabWidgets()[0];
		} else {
			return std::make_pair( (UITab*)nullptr, (UIWidget*)nullptr );
		}
	}

	return createWidgetInTabWidget( tabWidget, widget, tabName, focus );
}

std::vector<std::pair<UITab*, UITabWidget*>>
UITabWidgetSplitter::getTabFromOwnedWidgetId( const std::string& id ) {
	std::vector<std::pair<UITab*, UITabWidget*>> ret;
	forEachTabWidget( [&ret, &id]( UITabWidget* tabWidget ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); ++i ) {
			UITab* tab = tabWidget->getTab( i );
			Node* ownedNode = tab->getOwnedWidget();
			if ( ownedNode->isWidget() && ownedNode->asType<UIWidget>()->getId() == id ) {
				ret.push_back( { tab, tabWidget } );
			}
		}
	} );
	return ret;
}

bool UITabWidgetSplitter::ownedWidgetExists( UIWidget* widget ) {
	if ( !widget )
		return false;
	bool found = false;
	forEachTabWidgetStoppable( [&found, widget]( UITabWidget* tabWidget ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); ++i ) {
			UITab* tab = tabWidget->getTab( i );
			if ( tab->getOwnedWidget() == widget ) {
				found = true;
				return true;
			}
		}
		return false;
	} );
	return found;
}

bool UITabWidgetSplitter::removeTabWithOwnedWidgetId( const std::string& id, bool destroyOwnedNode,
													  bool immediateClose ) {
	auto ret = getTabFromOwnedWidgetId( id );
	if ( ret.empty() )
		return false;

	for ( const auto& r : ret )
		r.second->removeTab( r.first, destroyOwnedNode, immediateClose );

	return true;
}

UITabWidget* UITabWidgetSplitter::createTabWidget( Node* parent ) {
	if ( nullptr == mBaseLayout )
		mBaseLayout = parent;
	UITabWidget* tabWidget = UITabWidget::New();
	tabWidget->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	tabWidget->setParent( parent );
	tabWidget->setTabsClosable( true );
	tabWidget->setHideTabBarOnSingleTab( mHideTabBarOnSingleTab );
	tabWidget->setHideTabBar( mHideTabBar );
	tabWidget->setAllowRearrangeTabs( true );
	tabWidget->setAllowDragAndDropTabs( true );
	tabWidget->setAllowSwitchTabsInEmptySpaces( true );
	tabWidget->setEnabledCreateContextMenu( true );
	tabWidget->setFocusTabBehavior( UITabWidget::FocusTabBehavior::FocusOrder );
	if ( mVisualSplitting ) {
		tabWidget->setSplitFunction(
			[this]( SplitDirection dir, UITabWidget* widget ) -> UITabWidget* {
				return splitTabWidget( dir, widget );
			},
			mVisualSplitEdgePercent );
	}
	tabWidget->on( Event::OnTabSelected, [this]( const Event* event ) {
		UITabWidget* tabWidget = event->getNode()->asType<UITabWidget>();
		eeASSERT( nullptr != tabWidget && nullptr != tabWidget->getTabSelected() &&
				  nullptr != tabWidget->getTabSelected()->getOwnedWidget() );
		if ( !isWidgetInAnyWidget(
				 tabWidget->getTabSelected()->getOwnedWidget()->asType<UIWidget>() ) )
			return;
		setCurrentWidget( tabWidget->getTabSelected()->getOwnedWidget()->asType<UIWidget>() );
	} );
	tabWidget->setTabTryCloseCallback(
		[this]( UITab* tab, UITabWidget::FocusTabBehavior focusTabBehavior ) -> bool {
			if ( tab->getOwnedWidget() &&
				 tryTabClose( tab->getOwnedWidget()->asType<UIWidget>(), focusTabBehavior ) ) {
				closeTab( tab->getOwnedWidget()->asType<UIWidget>(),
						  UITabWidget::FocusTabBehavior::Default );
			}
			return false;
		} );
	tabWidget->on( Event::OnTabClosed, [this]( const Event* event ) {
		onTabClosed( static_cast<const TabEvent*>( event ) );
	} );
	if ( mOnTabWidgetCreateCb )
		mOnTabWidgetCreateCb( tabWidget );
	Lock l( mTabWidgetMutex );
	mTabWidgets.push_back( tabWidget );
	return tabWidget;
}

bool UITabWidgetSplitter::tryTabClose( UIWidget* widget,
									   UITabWidget::FocusTabBehavior focusTabBehavior,
									   std::function<void()> onMsgBoxCloseCb ) {
	if ( !widget )
		return true;

	if ( mTabTryCloseCb )
		return mTabTryCloseCb( widget, focusTabBehavior, onMsgBoxCloseCb );

	return true;
}

void UITabWidgetSplitter::closeAllTabs( std::vector<UITab*> tabs,
										UITabWidget::FocusTabBehavior focusTabBehavior ) {
	while ( !tabs.empty() ) {
		UITab* tab = tabs.back();
		if ( tab && !tryTabClose( tab->getOwnedWidget()->asType<UIWidget>(), focusTabBehavior,
								  [this, tabs, focusTabBehavior]() mutable {
									  tabs.pop_back();
									  closeAllTabs( tabs, focusTabBehavior );
								  } ) ) {
			return;
		} else {
			closeTab( tab->getOwnedWidget()->asType<UIWidget>(),
					  UITabWidget::FocusTabBehavior::Default );
			tabs.pop_back();
		}
	}
}

bool UITabWidgetSplitter::tryCloseAllTabs( UIWidget* widget,
										   UITabWidget::FocusTabBehavior focusTabBehavior ) {
	UITabWidget* tabW = tabWidgetFromWidget( widget );
	if ( !tabW )
		return false;

	size_t tabCount = tabW->getTabCount();
	std::vector<UITab*> tabs;
	for ( size_t i = 0; i < tabCount; i++ ) {
		if ( tabW->getTab( i )->getOwnedWidget() )
			tabs.push_back( tabW->getTab( i ) );
	}

	closeAllTabs( tabs, focusTabBehavior );

	return true;
}

bool UITabWidgetSplitter::tryCloseOtherTabs( UIWidget* widget,
											 UITabWidget::FocusTabBehavior focusTabBehavior ) {
	UITabWidget* tabW = tabWidgetFromWidget( widget );
	if ( !tabW )
		return false;

	size_t tabCount = tabW->getTabCount();
	std::vector<UITab*> tabs;
	for ( size_t i = 0; i < tabCount; i++ ) {
		if ( tabW->getTab( i )->getOwnedWidget() != widget )
			tabs.push_back( tabW->getTab( i ) );
	}

	closeAllTabs( tabs, focusTabBehavior );

	return true;
}

bool UITabWidgetSplitter::tryCloseTabsToDirection( UIWidget* widget,
												   UITabWidget::FocusTabBehavior focusTabBehavior,
												   bool toTheRight ) {
	UITabWidget* tabW = tabWidgetFromWidget( widget );
	if ( !tabW )
		return false;

	UITab* tab = tabW->getTabFromOwnedWidget( widget );
	if ( !tab )
		return false;
	size_t tabIndex = tabW->getTabIndex( tab );
	if ( tabIndex == eeINDEX_NOT_FOUND )
		return false;

	size_t tabCount = tabW->getTabCount();
	std::vector<UITab*> tabs;

	if ( toTheRight ) {
		for ( size_t i = tabIndex + 1; i < tabCount; i++ )
			tabs.push_back( tabW->getTab( i ) );
	} else {
		for ( size_t i = 0; i < tabIndex; i++ )
			tabs.push_back( tabW->getTab( i ) );
	}

	closeAllTabs( tabs, focusTabBehavior );

	return true;
}

void UITabWidgetSplitter::closeTab( UIWidget* widget,
									UITabWidget::FocusTabBehavior focusTabBehavior ) {
	if ( widget ) {
		UITabWidget* tabWidget = tabWidgetFromWidget( widget );
		if ( tabWidget )
			tabWidget->removeTab( (UITab*)widget->getData(), true, false, focusTabBehavior );
		if ( mCurWidget == widget )
			mCurWidget = nullptr;
	}
}

UISplitter* UITabWidgetSplitter::split( const SplitDirection& direction, UIWidget* widget ) {
	if ( !widget || ( mCanCreateSplitFn && !mCanCreateSplitFn( direction, widget ) ) )
		return nullptr;
	UIOrientation orientation =
		direction == SplitDirection::Left || direction == SplitDirection::Right
			? UIOrientation::Horizontal
			: UIOrientation::Vertical;
	UITabWidget* tabWidget = tabWidgetFromWidget( widget );
	if ( !tabWidget )
		return nullptr;
	Node* parent = tabWidget->getParent();
	UISplitter* parentSplitter = nullptr;
	bool wasFirst = true;

	if ( parent->isType( UI_TYPE_SPLITTER ) ) {
		parentSplitter = parent->asType<UISplitter>();
		wasFirst = parentSplitter->getFirstWidget() == tabWidget;
		if ( !parentSplitter->isFull() ) {
			parentSplitter->setOrientation( orientation );
			createTabWidget( parentSplitter );
			if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
				parentSplitter->swap();
			return nullptr;
		}
	}

	UISplitter* splitter = UISplitter::New();
	splitter->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	splitter->setOrientation( orientation );
	tabWidget->detach();
	splitter->setParent( parent );
	tabWidget->setParent( splitter );
	createTabWidget( splitter );
	if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
		splitter->swap();

	if ( parentSplitter ) {
		if ( wasFirst && parentSplitter->getFirstWidget() != splitter ) {
			parentSplitter->swap();
		} else if ( !wasFirst && parentSplitter->getLastWidget() != splitter ) {
			parentSplitter->swap();
		}
	}

	return splitter;
}

void UITabWidgetSplitter::setCanCreateSplitFn(
	std::function<bool( SplitDirection direction, UIWidget* widget )> fn ) {
	mCanCreateSplitFn = std::move( fn );
}

UITabWidget* UITabWidgetSplitter::splitTabWidget( SplitDirection direction,
												  UITabWidget* tabWidget ) {
	if ( !tabWidget || ( mCanCreateSplitFn && !mCanCreateSplitFn( direction, tabWidget ) ) )
		return nullptr;
	UIOrientation orientation =
		direction == SplitDirection::Left || direction == SplitDirection::Right
			? UIOrientation::Horizontal
			: UIOrientation::Vertical;
	Node* parent = tabWidget->getParent();
	UISplitter* parentSplitter = nullptr;
	bool wasFirst = true;

	if ( parent->isType( UI_TYPE_SPLITTER ) ) {
		parentSplitter = parent->asType<UISplitter>();
		wasFirst = parentSplitter->getFirstWidget() == tabWidget;
		if ( !parentSplitter->isFull() ) {
			parentSplitter->setOrientation( orientation );
			UITabWidget* newTabWidget = createTabWidget( parentSplitter );
			if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
				parentSplitter->swap();
			return newTabWidget;
		}
	}

	UISplitter* splitter = UISplitter::New();
	splitter->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	splitter->setOrientation( orientation );
	tabWidget->detach();
	splitter->setParent( parent );
	tabWidget->setParent( splitter );
	UITabWidget* newTabWidget = createTabWidget( splitter );
	if ( direction == SplitDirection::Left || direction == SplitDirection::Top )
		splitter->swap();

	if ( parentSplitter ) {
		if ( wasFirst && parentSplitter->getFirstWidget() != splitter ) {
			parentSplitter->swap();
		} else if ( !wasFirst && parentSplitter->getLastWidget() != splitter ) {
			parentSplitter->swap();
		}
	}

	return newTabWidget;
}

void UITabWidgetSplitter::switchToTab( Int32 index ) {
	UITabWidget* tabWidget = tabWidgetFromWidget( mCurWidget );
	if ( tabWidget ) {
		tabWidget->setTabSelected( eeclamp<Int32>( index, 0, tabWidget->getTabCount() - 1 ) );
	}
}

UITabWidget* UITabWidgetSplitter::findPreviousSplit( UIWidget* widget ) {
	if ( !widget )
		return nullptr;
	UISplitter* splitter = splitterFromWidget( widget );
	if ( !splitter )
		return nullptr;
	UITabWidget* tabWidget = tabWidgetFromWidget( widget );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.rbegin(), mTabWidgets.rend(), tabWidget );
		if ( it != mTabWidgets.rend() && ++it != mTabWidgets.rend() ) {
			return *it;
		}
	}
	return nullptr;
}

void UITabWidgetSplitter::switchPreviousSplit( UIWidget* widget ) {
	UITabWidget* tabWidget = findPreviousSplit( widget );
	if ( tabWidget && tabWidget->getTabSelected() &&
		 tabWidget->getTabSelected()->getOwnedWidget() ) {
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} else {
		tabWidget = findNextSplit( widget );
		if ( tabWidget && tabWidget->getTabSelected() &&
			 tabWidget->getTabSelected()->getOwnedWidget() ) {
			tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
		}
	}
}

UITabWidget* UITabWidgetSplitter::findNextSplit( UIWidget* widget ) {
	if ( !widget )
		return nullptr;
	UISplitter* splitter = splitterFromWidget( widget );
	if ( !splitter )
		return nullptr;
	UITabWidget* tabWidget = tabWidgetFromWidget( widget );
	if ( tabWidget ) {
		auto it = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
		if ( it != mTabWidgets.end() && ++it != mTabWidgets.end() ) {
			return *it;
		}
	}
	return nullptr;
}

void UITabWidgetSplitter::switchNextSplit( UIWidget* widget ) {
	UITabWidget* tabWidget = findNextSplit( widget );
	if ( tabWidget && tabWidget->getTabSelected() &&
		 tabWidget->getTabSelected()->getOwnedWidget() ) {
		tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} else {
		tabWidget = findPreviousSplit( widget );
		if ( tabWidget && tabWidget->getTabSelected() &&
			 tabWidget->getTabSelected()->getOwnedWidget() ) {
			tabWidget->getTabSelected()->getOwnedWidget()->setFocus();
		}
	}
}

void UITabWidgetSplitter::focusSomeWidget( Node* searchFrom ) {
	UIWidget* widget =
		searchFrom && searchFrom->isWidget()
			? searchFrom->findByType<UIWidget>( UI_TYPE_WIDGET )
			: ( mBaseLayout ? mBaseLayout->findByType<UIWidget>( UI_TYPE_WIDGET ) : nullptr );

	UITabWidget* tabW = nullptr;
	if ( widget && widget->getParent() && widget->getParent()->getParent() &&
		 widget->getParent()->getParent()->isType( UI_TYPE_TABWIDGET ) &&
		 ( tabW = tabWidgetFromWidget( widget ) ) && !tabW->isClosing() &&
		 tabW->getTabCount() > 1 ) {
		if ( tabW && ( tabW->getTabSelected()->getOwnedWidget() != widget ||
					   ( searchFrom == nullptr && widget != nullptr ) ) ) {
			tabW->setTabSelected( tabW->getTabSelected() );
			return;
		} else if ( tabW ) {
			for ( size_t i = 0; i < tabW->getTabCount(); ++i ) {
				if ( tabW->getTab( i )->getOwnedWidget() != widget ) {
					tabW->setTabSelected( i );
					return;
				}
			}
		}
	}

	for ( auto widget : mTabWidgets ) {
		if ( !widget->isClosing() && widget->getTabCount() > 0 ) {
			if ( widget->getTabSelected() != nullptr ) {
				widget->setTabSelected( widget->getTabSelected() );
			} else {
				widget->setTabSelected( (Uint32)0 );
			}
			return;
		}
	}
}

void UITabWidgetSplitter::closeTabWidgets( UISplitter* splitter ) {
	Node* node = splitter->getFirstChild();
	while ( node ) {
		if ( node->isType( UI_TYPE_TABWIDGET ) ) {
			auto it =
				std::find( mTabWidgets.begin(), mTabWidgets.end(), node->asType<UITabWidget>() );
			if ( it != mTabWidgets.end() ) {
				Lock l( mTabWidgetMutex );
				mTabWidgets.erase( it );
			}
		} else if ( node->isType( UI_TYPE_SPLITTER ) ) {
			closeTabWidgets( node->asType<UISplitter>() );
		}
		node = node->getNextNode();
	}
}

bool UITabWidgetSplitter::checkWidgetExists( UIWidget* checkWidget ) const {
	bool found = false;
	forEachWidgetStoppable( [&]( UIWidget* widget ) {
		if ( widget == checkWidget ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found || checkWidget == nullptr;
}

bool UITabWidgetSplitter::isWidgetInAnyWidget( UIWidget* checkWidget ) const {
	bool found = false;
	forEachWidgetStoppable( [&found, checkWidget]( UIWidget* widget ) {
		if ( widget == checkWidget ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found;
}

UITab* UITabWidgetSplitter::getTabFromWidget( UIWidget* checkWidget ) const {
	for ( auto tabWidget : mTabWidgets ) {
		size_t tabCount = tabWidget->getTabCount();
		for ( size_t i = 0; i < tabCount; i++ ) {
			UITab* tab = tabWidget->getTab( i );
			if ( tab->getOwnedWidget() == checkWidget )
				return tab;
		}
	}
	return nullptr;
}

bool UITabWidgetSplitter::hasSplit() const {
	return mTabWidgets.size() > 1;
}

UIOrientation UITabWidgetSplitter::getMainSplitOrientation() const {
	if ( !hasSplit() )
		return UIOrientation::Vertical;

	UITab* tab = nullptr;
	if ( mTabWidgets[0]->getTabCount() > 0 && ( tab = mTabWidgets[0]->getTab( 0 ) ) &&
		 tab->getOwnedWidget() && tab->getOwnedWidget()->isWidget() ) {
		UISplitter* splitter = splitterFromWidget( tab->getOwnedWidget()->asType<UIWidget>() );
		if ( splitter )
			return splitter->getOrientation();
	}

	return UIOrientation::Vertical;
}

bool UITabWidgetSplitter::curWidgetExists() const {
	bool found = false;
	forEachWidgetStoppable( [&]( UIWidget* widget ) {
		if ( widget == mCurWidget ) {
			found = true;
			return true;
		}
		return false;
	} );
	return found || mCurWidget == nullptr;
}

UISceneNode* UITabWidgetSplitter::getUISceneNode() const {
	return mUISceneNode;
}

UIWidget* UITabWidgetSplitter::getCurWidget() const {
	return mCurWidget;
}

UIWidget* UITabWidgetSplitter::getSomeWidget() {
	UIWidget* w = nullptr;
	forEachWidgetStoppable( [&]( UIWidget* widget ) {
		w = widget;
		return true;
	} );
	return w;
}

void UITabWidgetSplitter::addRemainingTabWidgets( Node* widget ) {
	if ( widget->isType( UI_TYPE_TABWIDGET ) ) {
		if ( std::find( mTabWidgets.begin(), mTabWidgets.end(), widget->asType<UITabWidget>() ) ==
			 mTabWidgets.end() ) {
			Lock l( mTabWidgetMutex );
			mTabWidgets.push_back( widget->asType<UITabWidget>() );
		}
	} else if ( widget->isType( UI_TYPE_SPLITTER ) ) {
		UISplitter* splitter = widget->asType<UISplitter>();
		addRemainingTabWidgets( splitter->getFirstWidget() );
		addRemainingTabWidgets( splitter->getLastWidget() );
	}
}

void UITabWidgetSplitter::closeSplitter( UISplitter* splitter ) {
	splitter->setParent( mUISceneNode->getRoot() );
	splitter->setVisible( false );
	splitter->setEnabled( false );
	splitter->close();
	closeTabWidgets( splitter );
}

void UITabWidgetSplitter::onTabClosed( const TabEvent* tabEvent ) {
	UIWidget* widget = tabEvent->getTab()->getOwnedWidget()->asType<UIWidget>();
	UITabWidget* tabWidget = tabEvent->getTab()->getTabWidget();
	if ( tabWidget->getTabCount() == 0 ) {
		UISplitter* splitter = splitterFromWidget( widget );
		if ( splitter && splitter->isFull() ) {
			tabWidget->close();
			auto itWidget = std::find( mTabWidgets.begin(), mTabWidgets.end(), tabWidget );
			if ( itWidget != mTabWidgets.end() ) {
				Lock l( mTabWidgetMutex );
				mTabWidgets.erase( itWidget );
			}

			Node* parent = splitter->getParent();
			if ( parent->isType( UI_TYPE_SPLITTER ) ) {
				UISplitter* parentSplitter = parent->asType<UISplitter>();
				Node* remainingNode = tabWidget == splitter->getFirstWidget()
										  ? splitter->getLastWidget()
										  : splitter->getFirstWidget();
				bool wasFirst = parentSplitter->getFirstWidget() == splitter;
				remainingNode->detach();
				closeSplitter( splitter );
				remainingNode->setParent( parentSplitter );
				addRemainingTabWidgets( remainingNode );
				if ( wasFirst )
					parentSplitter->swap();
				focusSomeWidget( parentSplitter );
			} else {
				Node* remainingNode = tabWidget == splitter->getFirstWidget()
										  ? splitter->getLastWidget()
										  : splitter->getFirstWidget();
				closeSplitter( splitter );
				eeASSERT( parent->getChildCount() == 0 );
				remainingNode->setParent( parent );
				if ( remainingNode->isWidget() )
					remainingNode->asType<UIWidget>()->setLayoutSizePolicy(
						SizePolicy::MatchParent, SizePolicy::MatchParent );
				addRemainingTabWidgets( remainingNode );
				focusSomeWidget( nullptr );
			}

			eeASSERT( !mTabWidgets.empty() );
			eeASSERT( !( mTabWidgets.size() == 1 && mTabWidgets[0]->getTabCount() == 0 ) );
			return;
		}
		mCurWidget = nullptr;
	} else {
		if ( tabWidget->getTabSelectedIndex() >= tabWidget->getTabCount() )
			tabWidget->setTabSelected(
				eemin( tabWidget->getTabCount() - 1, tabEvent->getTabIndex() ) );

		if ( mCurWidget == widget )
			mCurWidget = nullptr;
	}
}

void UITabWidgetSplitter::setOnTabWidgetCreateCb( std::function<void( UITabWidget* )> cb ) {
	mOnTabWidgetCreateCb = std::move( cb );
}

bool UITabWidgetSplitter::getVisualSplitting() const {
	return mVisualSplitting;
}

void UITabWidgetSplitter::setVisualSplitting( bool visualSplitting ) {
	if ( mVisualSplitting != visualSplitting ) {
		mVisualSplitting = visualSplitting;
		updateTabWidgetVisualSplitting();
	}
}

Float UITabWidgetSplitter::getVisualSplitEdgePercent() const {
	return mVisualSplitEdgePercent;
}

void UITabWidgetSplitter::setVisualSplitEdgePercent( Float visualSplitEdgePercent ) {
	if ( mVisualSplitEdgePercent != visualSplitEdgePercent ) {
		mVisualSplitEdgePercent = visualSplitEdgePercent;
		updateTabWidgetVisualSplitting();
	}
}

void UITabWidgetSplitter::updateTabWidgetVisualSplitting() {
	for ( UITabWidget* tabWidget : mTabWidgets ) {
		if ( mVisualSplitting ) {
			tabWidget->setSplitFunction(
				[this]( SplitDirection dir, UITabWidget* widget ) -> UITabWidget* {
					return splitTabWidget( dir, widget );
				},
				mVisualSplitEdgePercent );
		} else {
			tabWidget->setSplitFunction( nullptr, mVisualSplitEdgePercent );
		}
	}
}

void UITabWidgetSplitter::setTabTryCloseCallback( TabTryCloseCallback cb ) {
	mTabTryCloseCb = std::move( cb );
}

const std::vector<UITabWidget*>& UITabWidgetSplitter::getTabWidgets() const {
	return mTabWidgets;
}

UITabWidget* UITabWidgetSplitter::getFirstTabWidget() const {
	eeASSERT( !mTabWidgets.empty() );
	return mTabWidgets[0];
}

Node* UITabWidgetSplitter::getBaseLayout() const {
	return mBaseLayout;
}

bool UITabWidgetSplitter::getHideTabBarOnSingleTab() const {
	return mHideTabBarOnSingleTab;
}

void UITabWidgetSplitter::setHideTabBarOnSingleTab( bool hideTabBarOnSingleTab ) {
	if ( hideTabBarOnSingleTab != mHideTabBarOnSingleTab ) {
		mHideTabBarOnSingleTab = hideTabBarOnSingleTab;

		for ( auto widget : mTabWidgets )
			widget->setHideTabBarOnSingleTab( hideTabBarOnSingleTab );
	}
}

void UITabWidgetSplitter::setHideTabBar( bool hideTabBar ) {
	if ( hideTabBar != mHideTabBar ) {
		mHideTabBar = hideTabBar;

		for ( auto widget : mTabWidgets )
			widget->setHideTabBar( hideTabBar );
	}
}

void UITabWidgetSplitter::forEachWidgetClass( const std::string& className,
											  std::function<void( UIWidget* )> run ) const {
	Node* node;
	UIWidget* widget;
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			node = tabWidget->getTab( i )->getOwnedWidget();
			if ( node && node->isWidget() ) {
				widget = node->asType<UIWidget>();
				if ( widget->hasClass( className ) )
					run( widget );
			}
		}
	}
}

void UITabWidgetSplitter::forEachWidgetClassStoppable(
	const std::string& className, std::function<bool( UIWidget* )> run ) const {
	Node* node;
	UIWidget* widget;
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			node = tabWidget->getTab( i )->getOwnedWidget();
			if ( node && node->isWidget() ) {
				widget = node->asType<UIWidget>();
				if ( widget->hasClass( className ) && run( widget ) )
					return;
			}
		}
	}
}

void UITabWidgetSplitter::forEachWidgetType( const UINodeType& nodeType,
											 std::function<void( UIWidget* )> run ) const {
	Node* node;
	UIWidget* widget;
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			node = tabWidget->getTab( i )->getOwnedWidget();
			if ( node && node->isWidget() ) {
				widget = node->asType<UIWidget>();
				if ( widget->isType( nodeType ) )
					run( widget );
			}
		}
	}
}

void UITabWidgetSplitter::forEachWidgetTypeStoppable( const UINodeType& nodeType,
													  std::function<bool( UIWidget* )> run ) const {
	Node* node;
	UIWidget* widget;
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			node = tabWidget->getTab( i )->getOwnedWidget();
			if ( node && node->isWidget() ) {
				widget = node->asType<UIWidget>();
				if ( widget->isType( nodeType ) && run( widget ) )
					return;
			}
		}
	}
}

void UITabWidgetSplitter::forEachWidgetStoppable( std::function<bool( UIWidget* )> run ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isWidget() &&
				 run( tabWidget->getTab( i )->getOwnedWidget()->asType<UIWidget>() ) ) {
				return;
			}
		}
	}
}

void UITabWidgetSplitter::forEachWidget( std::function<void( UIWidget* )> run ) const {
	for ( auto tabWidget : mTabWidgets ) {
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ ) {
			if ( tabWidget->getTab( i )->getOwnedWidget()->isWidget() )
				run( tabWidget->getTab( i )->getOwnedWidget()->asType<UIWidget>() );
		}
	}
}

void UITabWidgetSplitter::forEachTabWidget( std::function<void( UITabWidget* )> run ) const {
	for ( auto widget : mTabWidgets )
		run( widget );
}

void UITabWidgetSplitter::forEachTabWidgetStoppable(
	std::function<bool( UITabWidget* )> run ) const {
	for ( auto widget : mTabWidgets )
		if ( run( widget ) )
			return;
}

void UITabWidgetSplitter::forEachTab( std::function<void( UITab* )> run ) const {
	for ( auto tabWidget : mTabWidgets )
		for ( size_t i = 0; i < tabWidget->getTabCount(); i++ )
			run( tabWidget->getTab( i ) );
}

nlohmann::json UITabWidgetSplitter::serializeNode( Node* node ) const {
	using json = nlohmann::json;
	json res;

	if ( node->isType( UI_TYPE_SPLITTER ) ) {
		UISplitter* splitter = node->asType<UISplitter>();
		res["type"] = "splitter";
		res["split"] = splitter->getSplitPartition().toString();
		res["orientation"] =
			splitter->getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		res["first"] = serializeNode( splitter->getFirstWidget() );
		res["last"] = serializeNode( splitter->getLastWidget() );
	} else if ( node->isType( UI_TYPE_NODELINK ) && node->asType<UINodeLink>()->getNodeLink() ) {
		return serializeNode( node->asType<UINodeLink>()->getNodeLink() );
	} else if ( node->isType( UI_TYPE_TABWIDGET ) ) {
		UITabWidget* tabWidget = node->asType<UITabWidget>();
		json files = json::array();
		for ( size_t i = 0; i < tabWidget->getTabCount(); ++i ) {
			Node* ownedWidget = tabWidget->getTab( i )->getOwnedWidget();
			if ( !ownedWidget || !ownedWidget->isWidget() )
				continue;
			UIWidget* widget = ownedWidget->asType<UIWidget>();
			Lock l( mWidgetTypesMutex );
			for ( const auto& cls : widget->getClasses() ) {
				auto it = mWidgetTypes.find( cls );
				if ( it != mWidgetTypes.end() ) {
					json f = it->second.onSave( widget );
					f["type"] = it->first;
					if ( !f.contains( "title" ) || !f["title"].is_string() )
						f["title"] = tabWidget->getTab( i )->getText();
					files.emplace_back( std::move( f ) );
					break;
				}
			}
		}
		res["type"] = "tabwidget";
		res["files"] = files;
		res["current_page"] = tabWidget->getTabSelectedIndex();
	}

	return res;
}

void UITabWidgetSplitter::unserializeNode( const nlohmann::json& j, UITabWidget* curTabWidget ) {
	if ( j["type"] == "tabwidget" ) {
		Int64 currentPage = j.value( "current_page", (Int64)0 );
		const auto& filesArr = j["files"];
		for ( const auto& file : filesArr ) {
			if ( !file.contains( "type" ) )
				continue;
			std::string type = file["type"];
			Lock l( mWidgetTypesMutex );
			auto it = mWidgetTypes.find( type );
			if ( it != mWidgetTypes.end() ) {
				WidgetLoadResult result = it->second.onLoad( file );
				std::string title =
					!result.title.empty() ? result.title : file.value( "title", "" );
				auto [tab, _] = createWidgetInTabWidget( curTabWidget, result.widget, title );
				if ( result.icon )
					tab->setIcon( result.icon );
			}
		}
		if ( curTabWidget->getTabCount() > 0 ) {
			curTabWidget->setTabSelected(
				eeclamp<Int32>( currentPage, 0, curTabWidget->getTabCount() - 1 ) );
		}
	} else if ( j["type"] == "splitter" ) {
		SplitDirection dir = j.value( "orientation", "vertical" ) == "horizontal"
								 ? SplitDirection::Right
								 : SplitDirection::Bottom;
		UITabWidget* newTabWidget = splitTabWidget( dir, curTabWidget );
		if ( !newTabWidget )
			return;

		unserializeNode( j["first"], curTabWidget );
		unserializeNode( j["last"], newTabWidget );

		if ( j.contains( "split" ) && j["split"].is_string() ) {
			Node* parent = curTabWidget->getParent();
			if ( parent && parent->isType( UI_TYPE_SPLITTER ) ) {
				UISplitter* splitter = parent->asType<UISplitter>();
				splitter->setSplitPartition( StyleSheetLength( j["split"].get<std::string>() ) );
			}
		}
	}
}

nlohmann::json UITabWidgetSplitter::toJSON() const {
	if ( mTabWidgets.empty() )
		return nlohmann::json::object();

	// Walk up from the first tab widget to the root splitter
	Node* root = mTabWidgets[0];
	while ( root->getParent() && root->getParent()->isType( UI_TYPE_SPLITTER ) )
		root = root->getParent();

	return serializeNode( root );
}

void UITabWidgetSplitter::fromJSON( const nlohmann::json& j ) {
	if ( mTabWidgets.empty() )
		return;
	if ( !j.contains( "type" ) )
		return;
	UITabWidget* initialTabWidget = getFirstTabWidget();
	unserializeNode( j, initialTabWidget );
}

}}} // namespace EE::UI::Tools
