#include "statusdebuggercontroller.hpp"
#include "../../appconfig.hpp"
#include "../../uirightpanel.hpp"
#include "../../widgetcommandexecuter.hpp"
#include "../plugincontextprovider.hpp"
#include "eepp/ui/uiwidgetcreator.hpp"
#include <eepp/ui/uicheckbox.hpp>
#include <nlohmann/json.hpp>

namespace ecode {

static constexpr size_t DEBUGGER_LAYOUT_MAX_DEPTH = 16;
static constexpr size_t DEBUGGER_LAYOUT_MAX_NODES = 64;
static constexpr size_t DEBUGGER_WIDGET_ID_PREFIX_LENGTH = 9;

static bool isValidDebuggerLayoutNode( const nlohmann::json& node,
									   UnorderedSet<std::string>& widgetTypes, size_t depth,
									   size_t& nodeCount ) {
	if ( depth > DEBUGGER_LAYOUT_MAX_DEPTH || ++nodeCount > DEBUGGER_LAYOUT_MAX_NODES )
		return false;
	if ( !node.is_object() || !node.contains( "type" ) || !node["type"].is_string() )
		return false;
	if ( node["type"] == "splitter" )
		return node.contains( "first" ) && node.contains( "last" ) &&
			   isValidDebuggerLayoutNode( node["first"], widgetTypes, depth + 1, nodeCount ) &&
			   isValidDebuggerLayoutNode( node["last"], widgetTypes, depth + 1, nodeCount );
	if ( node["type"] != "tabwidget" || !node.contains( "files" ) || !node["files"].is_array() )
		return false;
	for ( const auto& file : node["files"] ) {
		if ( !file.is_object() || !file.contains( "type" ) || !file["type"].is_string() ||
			 !widgetTypes.emplace( file["type"].get<std::string>() ).second )
			return false;
	}
	return true;
}

static void restoreColumnWidths( UIAbstractTableView* view, const nlohmann::json& columns,
								 const char* key ) {
	if ( view && columns.contains( key ) )
		view->unserializeColumnWidths( columns[key] );
}

class UIBreakpointsTableCell : public UITableCell {
  public:
	static UIBreakpointsTableCell* New( const std::string& tag, const BreakpointsModel* model,
										ModelIndex curIndex ) {
		return eeNew( UIBreakpointsTableCell, ( tag, model, curIndex ) );
	}

	UIBreakpointsTableCell( const std::string& tag, const BreakpointsModel* model,
							ModelIndex curIndex ) :
		UITableCell( tag, getCheckBoxFn( model, curIndex ) ) {}

	std::function<UITextView*( UIPushButton* )> getCheckBoxFn( const BreakpointsModel* model,
															   ModelIndex index ) {
		return [index, model, this]( UIPushButton* ) -> UITextView* {
			UICheckBox* chk = UICheckBox::New();
			bool enabled = model
							   ->data( model->index( index.row(), BreakpointsModel::Enabled ),
									   ModelRole::Data )
							   .asBool();
			chk->setChecked( enabled );
			chk->setCheckMode( UICheckBox::Button );
			chk->on( Event::OnValueChange, [this, chk]( const Event* ) {
				auto parent = static_cast<UIBreakpointsTableView*>( getParent()->getParent() );
				auto model = parent->getModel();
				auto index = getCurIndex();
				bool checked = chk->isChecked();
				if ( !parent->onBreakpointEnabledChange )
					return;

				std::string filePath(
					model
						->data( model->index( index.row(), BreakpointsModel::SourcePath ),
								ModelRole::Data )
						.asCStr() );
				int line( model
							  ->data( model->index( index.row(), BreakpointsModel::Line ),
									  ModelRole::Data )
							  .asInt() );
				parent->onBreakpointEnabledChange( filePath, line, checked );
			} );
			return chk;
		};
	}

	virtual void updateCell( Model* model ) {
		if ( !mTextBox->isType( UI_TYPE_CHECKBOX ) )
			return;
		auto bpModel = static_cast<BreakpointsModel*>( model );
		auto cur = bpModel->get( getCurIndex() );
		if ( cur.first.empty() )
			return;
		mTextBox->asType<UICheckBox>()->setChecked( cur.second.enabled );
	}
};

UIWidget* UIBreakpointsTableView::createCell( UIWidget* rowWidget, const ModelIndex& index ) {
	if ( index.column() == BreakpointsModel::Enabled ) {
		UIBreakpointsTableCell* widget = UIBreakpointsTableCell::New(
			mTag + "::cell", (const BreakpointsModel*)getModel(), index );
		widget->getTextView()->setEnabled( true );
		widget->setDontAutoHideEmptyTextBox( true );
		return setupCell( widget, rowWidget, index );
	} else if ( index.column() == BreakpointsModel::Remove ) {
		auto cell = UITableView::createCell( rowWidget, index );
		cell->onClick( [index, this]( auto ) {
			auto model = getModel();
			if ( onBreakpointRemove ) {
				std::string filePath(
					model
						->data( model->index( index.row(), BreakpointsModel::SourcePath ),
								ModelRole::Data )
						.asCStr() );
				int line( model
							  ->data( model->index( index.row(), BreakpointsModel::Line ),
									  ModelRole::Data )
							  .asInt() );
				onBreakpointRemove( filePath, line );
			}
		} );
		return cell;
	}
	return UITableView::createCell( rowWidget, index );
}

const KeyBindings::ShortcutMap
StatusDebuggerController::getLocalDefaultKeybindings() {
	return {
		{ { KEY_TAB, UICodeEditorSplitter::getDefaultSwitchToTabModifier() }, "next-tab" },
		{ { KEY_TAB, UICodeEditorSplitter::getDefaultSwitchToTabModifier() | KEYMOD_SHIFT },
		  "previous-tab" },
		{ { KEY_1, UICodeEditorSplitter::getDefaultSwitchToTabModifier() }, "switch-to-tab-1" },
		{ { KEY_2, UICodeEditorSplitter::getDefaultSwitchToTabModifier() }, "switch-to-tab-2" },
		{ { KEY_3, UICodeEditorSplitter::getDefaultSwitchToTabModifier() }, "switch-to-tab-3" },
		{ { KEY_4, UICodeEditorSplitter::getDefaultSwitchToTabModifier() }, "switch-to-tab-4" },
		{ { KEY_5, UICodeEditorSplitter::getDefaultSwitchToTabModifier() }, "switch-to-tab-5" },
	};
}

StatusDebuggerController::StatusDebuggerController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode,
													PluginContextProvider* pluginContext ) :
	StatusBarElement( mainSplitter, uiSceneNode, pluginContext ), mLifetime( this, uiSceneNode ) {
	mSerializedLayout = mContext->getConfig().iniState.getValue( "debugger", "panel_layout" );
}

StatusDebuggerController::~StatusDebuggerController() {
	mLifetime.invalidate();
	mEventConnections.clear();
	mTabWidgetEventConnections.clear();
	if ( mContainer )
		mContainer->removeActionsByTag( reinterpret_cast<UintPtr>( this ) );
	if ( mTabWidgetSplitter ) {
		mTabWidgetSplitter->setOnTabWidgetCreateCb( nullptr );
		mTabWidgetSplitter->setOnTabWidgetCloseCb( nullptr );
		mTabWidgetSplitter->forEachTabWidget( []( UITabWidget* tabWidget ) {
			tabWidget->setSplitFunction( nullptr );
			tabWidget->setTabTryCloseCallback( nullptr );
			tabWidget->setAcceptsDropOfWidgetFn( nullptr );
		} );
		eeDelete( mTabWidgetSplitter );
	}
	if ( mContext && mContext->getRightPanel() )
		mContext->getRightPanel()->unregisterPanel( "debugger" );
	if ( mContainer )
		mContainer->close();
}

void StatusDebuggerController::show() {
	StatusBarElement::show();
	updateRightPanel();
}

void StatusDebuggerController::hide() {
	StatusBarElement::hide();
	updateRightPanel();
}

UIWidget* StatusDebuggerController::getWidget() {
	return mContainer;
}

UIWidget* StatusDebuggerController::createWidget() {
	if ( nullptr == mContainer ) {
		createContainer();
		if ( onWidgetCreated )
			onWidgetCreated( this, mContainer );
	}
	return mContainer;
}

UITableView* StatusDebuggerController::getUIThreads() {
	return mUIThreads;
}

UITableView* StatusDebuggerController::getUIStack() {
	return mUIStack;
}

UIBreakpointsTableView* StatusDebuggerController::getUIBreakpoints() {
	return mUIBreakpoints;
}

UITreeView* StatusDebuggerController::getUIVariables() const {
	return mUIVariables;
}

UITreeView* StatusDebuggerController::getUIExpressions() const {
	return mUIExpressions;
}

UITabWidget* StatusDebuggerController::getUITabWidget() const {
	return mUITabWidget;
}

UICodeEditor* StatusDebuggerController::getUIConsole() const {
	return mUIConsole;
}

void StatusDebuggerController::insertConsoleBuffer( std::string&& buffer ) {
	auto console = getUIConsole();
	if ( console == nullptr )
		return;
	console->runOnMainThread( [console, this, buffer = std::move( buffer )]() {
		console->getDocument().insert( 0, console->getDocument().endOfDoc(), buffer );
		if ( getConsoleScrollLocked() )
			console->setScrollY( console->getMaxScroll().y );
	} );
}

void StatusDebuggerController::setDebuggingState( State state ) {
	if ( !mContainer )
		return;

	mLifetime.weakHandle().run( [state]( StatusDebuggerController* controller ) {
		controller->mUIButStart->setVisible( state == State::NotStarted )
			->setEnabled( state == State::NotStarted );
		controller->mUIButStop->setVisible( state != State::NotStarted )
			->setEnabled( state != State::NotStarted );
		controller->mUIButContinue->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		controller->mUIButPause->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Running );
		controller->mUIButStepOver->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		controller->mUIButStepInto->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		controller->mUIButStepOut->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
	} );
}

void StatusDebuggerController::clearConsoleBuffer() {
	if ( mUIConsole == nullptr )
		return;
	mUIConsole->runOnMainThread( [this] {
		mUIConsole->getDocument().reset();
		mUIConsole->invalidateLongestLineWidth();
		mUIConsole->setScrollY( mUIConsole->getMaxScroll().y );
	} );
}

const std::string& StatusDebuggerController::saveLayout() {
	saveTabLayout();
	return mSerializedLayout;
}

void StatusDebuggerController::onTabCreated( UITab* tab, UIWidget* widget ) {
	tab->setId( "debugger_tab_" + widget->getId().substr( DEBUGGER_WIDGET_ID_PREFIX_LENGTH ) );
	tab->addClass( "debugger-tab" );
	tab->setTextAsFallback( true );
	mEventConnections += tab->connect( Event::OnDragStart,
									   [this]( const Event* ) { beginRightPanelDropPreview(); } );
	mEventConnections +=
		tab->connect( Event::OnDragStop, [this]( const Event* ) { endRightPanelDropPreview(); } );
}

void StatusDebuggerController::onWidgetFocusChange( UIWidget* ) {}

void StatusDebuggerController::createTabWidgets() {
	mRightPanelContainer = mContext->getRightPanel()
							   ? mContext->getRightPanel()->registerPanel( "debugger" )
							   : nullptr;
	if ( !mDebuggerTabsContainer || !mRightPanelContainer )
		return;

	mTabWidgetSplitter = UITabWidgetSplitter::New( this, mUISceneNode );
	mTabWidgetSplitter->setHideTabBarOnSingleTab( false );
	mTabWidgetSplitter->setOnTabWidgetCreateCb( [this]( UITabWidget* tabWidget ) {
		tabWidget->setTabsClosable( false );
		tabWidget->setAcceptsDropOfWidgetFn(
			[]( const UIWidget* widget ) { return widget->hasClass( "debugger-tab" ); } );
		tabWidget->setSplitFunction(
			[this]( SplitDirection direction, UITabWidget* target ) -> UITabWidget* {
				Node* dragging = mUISceneNode->getEventDispatcher()->getNodeDragging();
				if ( dragging && dragging->isType( UI_TYPE_TAB ) ) {
					auto source = dragging->asType<UITab>()->getTabWidget();
					bool sourceInRight = source && mRightPanelContainer->isParentOf( source );
					bool targetInRight = mRightPanelContainer->isParentOf( target );
					if ( source && sourceInRight != targetInRight )
						return target;
				}
				return mTabWidgetSplitter->splitTabWidget( direction, target );
			},
			mTabWidgetSplitter->getVisualSplitEdgePercent() );
		auto& connections = mTabWidgetEventConnections[tabWidget];
		connections += tabWidget->connect( Event::OnTabAdded, [this]( const Event* ) {
			if ( !mRestoringLayout ) {
				saveTabLayout();
				updateRightPanel();
			}
		} );
	} );
	mTabWidgetSplitter->setOnTabWidgetCloseCb(
		[this]( UITabWidget* tabWidget ) { mTabWidgetEventConnections.erase( tabWidget ); } );

	mUITabWidget = mTabWidgetSplitter->createTabWidget( mDebuggerTabsContainer );
	mUITabWidget->setId( "app_debugger_tab_widget" );
	mUIRightTabWidget = mTabWidgetSplitter->createTabWidget( mRightPanelContainer );
	mUIRightTabWidget->setId( "app_debugger_right_tab_widget" );

	const auto registerWidget = [this]( const std::string& type, UIWidget* widget,
										const std::string& title ) {
		widget->addClass( type );
		mTabWidgetSplitter->registerWidgetType(
			type, { []( UIWidget* ) { return nlohmann::json::object(); },
					[widget, title]( const nlohmann::json& ) -> WidgetLoadResult {
						return { widget, nullptr, title };
					} } );
	};
	registerWidget( "debugger-threads-and-stack", mUIThreadsSplitter,
					mContext->i18n( "threads_and_stack", "Threads & Stack" ).toUtf8() );
	registerWidget( "debugger-variables", mUIVariables,
					mContext->i18n( "variables", "Variables" ).toUtf8() );
	registerWidget( "debugger-expressions", mUIExpressions,
					mContext->i18n( "expressions", "Expressions" ).toUtf8() );
	registerWidget( "debugger-breakpoints", mUIBreakpoints,
					mContext->i18n( "breakpoints", "Breakpoints" ).toUtf8() );
	registerWidget( "debugger-console", mUIConsole,
					mContext->i18n( "console_output", "Console Output" ).toUtf8() );

	restoreTabLayout();
}

void StatusDebuggerController::restoreTabLayout() {
	static const UnorderedSet<std::string> expectedWidgetTypes{
		"debugger-threads-and-stack", "debugger-variables", "debugger-expressions",
		"debugger-breakpoints", "debugger-console" };
	mRestoringLayout = true;
	bool restored = false;
	mSerializedLayout = mContext->getConfig().iniState.getValue( "debugger", "panel_layout" );
	const std::string& saved = mSerializedLayout;
	if ( !saved.empty() ) {
		auto layout = nlohmann::json::parse( saved, nullptr, false, true );
		UnorderedSet<std::string> widgetTypes;
		size_t nodeCount = 0;
		const int version = layout.is_discarded() ? 0 : layout.value( "version", 0 );
		if ( !layout.is_discarded() && ( version == 2 || version == 3 ) &&
			 layout.contains( "bottom" ) && layout.contains( "right" ) &&
			 isValidDebuggerLayoutNode( layout["bottom"], widgetTypes, 0, nodeCount ) &&
			 isValidDebuggerLayoutNode( layout["right"], widgetTypes, 0, nodeCount ) &&
			 widgetTypes == expectedWidgetTypes ) {
			mTabWidgetSplitter->unserializeNode( layout["bottom"], mUITabWidget );
			mTabWidgetSplitter->unserializeNode( layout["right"], mUIRightTabWidget );
			if ( layout.contains( "columns" ) && layout["columns"].is_object() ) {
				restoreColumnWidths( mUIThreads, layout["columns"], "threads" );
				restoreColumnWidths( mUIStack, layout["columns"], "stack" );
				restoreColumnWidths( mUIVariables, layout["columns"], "variables" );
				restoreColumnWidths( mUIExpressions, layout["columns"], "expressions" );
				restoreColumnWidths( mUIBreakpoints, layout["columns"], "breakpoints" );
			}
			restored = true;
		}
	}

	if ( !restored ) {
		mTabWidgetSplitter->createWidgetInTabWidget(
			mUITabWidget, mUIThreadsSplitter,
			mContext->i18n( "threads_and_stack", "Threads & Stack" ).toUtf8(), false );
		mTabWidgetSplitter->createWidgetInTabWidget(
			mUITabWidget, mUIBreakpoints, mContext->i18n( "breakpoints", "Breakpoints" ).toUtf8(),
			false );
		mTabWidgetSplitter->createWidgetInTabWidget(
			mUITabWidget, mUIConsole, mContext->i18n( "console_output", "Console Output" ).toUtf8(),
			false );
		mTabWidgetSplitter->createWidgetInTabWidget(
			mUIRightTabWidget, mUIVariables, mContext->i18n( "variables", "Variables" ).toUtf8(),
			false );
		mTabWidgetSplitter->createWidgetInTabWidget(
			mUIRightTabWidget, mUIExpressions,
			mContext->i18n( "expressions", "Expressions" ).toUtf8(), false );
		mUITabWidget->setTabSelected( Uint32{ 0 } );
		mUIRightTabWidget->setTabSelected( Uint32{ 0 } );
	}

	if ( mUITabWidget->getTabCount() ) {
		mTabWidgetSplitter->setCurrentWidget(
			mUITabWidget->getTabSelected()->getOwnedWidget()->asType<UIWidget>() );
	}

	mRestoringLayout = false;
	updateRightPanel();
}

void StatusDebuggerController::saveTabLayout() {
	if ( mRestoringLayout || !mTabWidgetSplitter || !mDebuggerTabsContainer ||
		 !mRightPanelContainer || !mDebuggerTabsContainer->getFirstChild() ||
		 !mRightPanelContainer->getFirstChild() )
		return;
	nlohmann::json layout;
	layout["version"] = 3;
	layout["bottom"] = mTabWidgetSplitter->serializeNode( mDebuggerTabsContainer->getFirstChild() );
	layout["right"] = mTabWidgetSplitter->serializeNode( mRightPanelContainer->getFirstChild() );
	layout["columns"]["threads"] = mUIThreads->serializeColumnWidths();
	layout["columns"]["stack"] = mUIStack->serializeColumnWidths();
	layout["columns"]["variables"] = mUIVariables->serializeColumnWidths();
	layout["columns"]["expressions"] = mUIExpressions->serializeColumnWidths();
	layout["columns"]["breakpoints"] = mUIBreakpoints->serializeColumnWidths();
	mSerializedLayout = layout.dump();
}

bool StatusDebuggerController::rightPanelHasTabs() const {
	bool hasTabs = false;
	if ( !mTabWidgetSplitter || !mRightPanelContainer )
		return false;
	mTabWidgetSplitter->forEachTabWidgetStoppable( [this, &hasTabs]( UITabWidget* tabWidget ) {
		hasTabs = mRightPanelContainer->isParentOf( tabWidget ) && tabWidget->getTabCount() > 0;
		return hasTabs;
	} );
	return hasTabs;
}

void StatusDebuggerController::beginRightPanelDropPreview() {
	if ( !mRightPanelDropPreview && mContainer && mContainer->isVisible() &&
		 !rightPanelHasTabs() ) {
		mRightPanelDropPreview = true;
		updateRightPanel();
	}
}

void StatusDebuggerController::endRightPanelDropPreview() {
	if ( !mRightPanelDropPreview || !mContainer )
		return;
	// The drop target is resolved after OnDragStop. Keep the empty panel alive until the next
	// update so it can receive the drop, then collapse it if the drag was canceled or rejected.
	mContainer->removeActionsByTag( reinterpret_cast<UintPtr>( this ) );
	mContainer->runOnMainThread(
		[this] {
			mRightPanelDropPreview = false;
			updateRightPanel();
		},
		Time::Zero, reinterpret_cast<UintPtr>( this ) );
}

void StatusDebuggerController::updateRightPanel() {
	auto rightPanel = mContext->getRightPanel();
	if ( !rightPanel || !mRightPanelContainer || !mTabWidgetSplitter )
		return;
	rightPanel->setPanelVisible( "debugger",
								 mContainer && mContainer->isVisible() &&
									 ( mRightPanelDropPreview || rightPanelHasTabs() ) );
}

void StatusDebuggerController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
	<style>
	#app_debugger_buttons > PushButton:disabled {
		tint: var(--disabled-color);
	}
	#app_debugger.vertical_bar {
		background-color: none;
	}
	</style>
	<hboxce id="app_debugger" lw="mp" lh="mp" visible="false">
		<vbox id="app_debugger_tabs" lw="0" lw8="1" lh="mp">
			<Splitter id="debugger_threads_and_stack" layout_width="mp" lh="mp" splitter-partition="15%">
			<TableView id="debugger_threads" layout_width="mp" layout_height="mp" column-width-mode="percentage" column-width-mode-menu="true" />
			<TableView id="debugger_stack" layout_width="mp" layout_height="mp" column-width-mode="percentage" column-width-mode-menu="true" />
		</Splitter>
		<TreeView id="debugger_variables" layout_width="mp" layout_height="mp" column-width-mode="percentage" column-width-mode-menu="true" />
		<TreeView id="debugger_expressions" layout_width="mp" layout_height="mp" column-width-mode="percentage" column-width-mode-menu="true" />
		<BreakpointsTableView id="debugger_breakpoints" layout_width="mp" layout_height="mp" column-width-mode="percentage" column-width-mode-menu="true" />
			<CodeEditor id="debugger_console" layout_width="mp" layout_height="mp" />
		</vbox>
		<vbox id="app_debugger_buttons" class="vertical_bar" lw="16dp" lh="mp">
			<PushButton class="expand_status_bar_panel" lw="mp" tooltip="@string(expand_panel, Expand Panel)" />
			<PushButton id="app_debugger_start" class="debugger_start" lw="mp" icon="icon(debug-start, 12dp)" tooltip="@string(start, Start)" />
			<PushButton id="app_debugger_stop" class="debugger_stop" lw="mp" icon="icon(debug-stop, 12dp)" tooltip="@string(stop, Stop)" />
			<PushButton id="app_debugger_continue" class="debugger_continue" lw="mp" icon="icon(debug-continue, 12dp)" tooltip="@string(continue, Continue)" />
			<PushButton id="app_debugger_pause" class="debugger_pause" lw="mp" icon="icon(debug-pause, 12dp)" tooltip="@string(pause, Pause)" />
			<PushButton id="app_debugger_step_over" class="debugger_step_over" lw="mp" icon="icon(debug-step-over, 12dp)" tooltip="@string(step_over, Step Over)" />
			<PushButton id="app_debugger_step_into" class="debugger_step_into" lw="mp" icon="icon(debug-step-into, 12dp)" tooltip="@string(step_into, Step Into)" />
			<PushButton id="app_debugger_step_out" class="debugger_step_out" lw="mp" icon="icon(debug-step-out, 12dp)" tooltip="@string(step_out, Step Out)" />
		</vbox>
	</hboxce>
	)xml";

	UIWidgetCreator::registerWidget( "BreakpointsTableView", UIBreakpointsTableView::New );

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mContext->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter,
											 String::hash( "status_debugger_controller" ) )
					 ->asType<UIHLinearLayoutCommandExecuter>();

	mContext->getStatusBar()->registerStatusBarPanel( mContainer, mContainer );

	mContainer->bind( "app_debugger_tabs", mDebuggerTabsContainer );
	mContainer->bind( "debugger_threads_and_stack", mUIThreadsSplitter );
	mContainer->bind( "debugger_threads", mUIThreads );
	mContainer->bind( "debugger_stack", mUIStack );
	mContainer->bind( "debugger_breakpoints", mUIBreakpoints );
	mContainer->bind( "debugger_variables", mUIVariables );
	mContainer->bind( "debugger_expressions", mUIExpressions );
	mContainer->bind( "debugger_console", mUIConsole );
	mEventConnections += mContainer->connect( Event::OnVisibleChange,
											  [this]( const Event* ) { updateRightPanel(); } );
	mContainer->bind( "app_debugger_start", mUIButStart );
	mContainer->bind( "app_debugger_stop", mUIButStop );
	mContainer->bind( "app_debugger_pause", mUIButPause );
	mContainer->bind( "app_debugger_continue", mUIButContinue );
	mContainer->bind( "app_debugger_step_over", mUIButStepOver );
	mContainer->bind( "app_debugger_step_into", mUIButStepInto );
	mContainer->bind( "app_debugger_step_out", mUIButStepOut );

	createTabWidgets();

	mContainer->setCommand( "next-tab", [this] {
		if ( mTabWidgetSplitter && mTabWidgetSplitter->getCurWidget() ) {
			auto tabWidget =
				mTabWidgetSplitter->tabWidgetFromWidget( mTabWidgetSplitter->getCurWidget() );
			if ( tabWidget )
				tabWidget->focusNextTab();
		}
	} );

	mContainer->setCommand( "previous-tab", [this] {
		if ( mTabWidgetSplitter && mTabWidgetSplitter->getCurWidget() ) {
			auto tabWidget =
				mTabWidgetSplitter->tabWidgetFromWidget( mTabWidgetSplitter->getCurWidget() );
			if ( tabWidget )
				tabWidget->focusPreviousTab();
		}
	} );

	for ( int i = 1; i <= 5; i++ ) {
		mContainer->setCommand( String::format( "switch-to-tab-%d", i ), [this, i] {
			if ( mTabWidgetSplitter )
				mTabWidgetSplitter->switchToTab( i - 1 );
		} );
	}

	mEventConnections += mContainer->connect( Event::KeyDown, [this]( const Event* event ) {
		auto ke = event->asKeyEvent();
		if ( ke->getSanitizedMod() == 0 && ke->getKeyCode() == EE::Window::KEY_ESCAPE &&
			 mSplitter->getCurEditor() ) {
			mSplitter->getCurEditor()->setFocus();
		}
	} );

	mContainer->getKeyBindings().addKeybinds( getLocalDefaultKeybindings() );

	setDebuggingState( State::NotStarted );

	mUIButStart->onClick(
		[this]( auto ) { mContext->runCommand( "debugger-continue-interrupt" ); } );

	mUIButContinue->onClick(
		[this]( auto ) { mContext->runCommand( "debugger-continue-interrupt" ); } );

	mUIButPause->onClick(
		[this]( auto ) { mContext->runCommand( "debugger-continue-interrupt" ); } );

	mUIButStop->onClick( [this]( auto ) { mContext->runCommand( "debugger-stop" ); } );

	mUIButStepOver->onClick( [this]( auto ) { mContext->runCommand( "debugger-step-over" ); } );

	mUIButStepInto->onClick( [this]( auto ) { mContext->runCommand( "debugger-step-into" ); } );

	mUIButStepOut->onClick( [this]( auto ) { mContext->runCommand( "debugger-step-out" ); } );

	mUIThreads->setAutoExpandOnSingleColumn( true );

	mUIStack->setMainColumn( 1 );

	mUIVariables->setMainColumn( 1 );

	mUIBreakpoints->setMainColumn( 1 );

	mUIExpressions->setMainColumn( 1 );

	mUIConsole->setLocked( true );
	mUIConsole->setLineBreakingColumn( 0 );
	mUIConsole->setShowLineNumber( false );
	mUIConsole->getDocument().reset();
	mUIConsole->setScrollY( mUIConsole->getMaxScroll().y );
	mEventConnections += mUIConsole->connect( Event::OnScrollChange, [this]( const Event* ) {
		mScrollLocked = mUIConsole->getMaxScroll().y == mUIConsole->getScroll().y;
	} );
}

} // namespace ecode
