#include "statusdebuggercontroller.hpp"
#include "../../widgetcommandexecuter.hpp"
#include "../plugincontextprovider.hpp"
#include "eepp/ui/uiwidgetcreator.hpp"
#include <eepp/ui/uicheckbox.hpp>

namespace ecode {

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

const std::map<KeyBindings::Shortcut, std::string>
StatusDebuggerController::getLocalDefaultKeybindings() {
	return {
		{ { KEY_TAB, KeyMod::getDefaultModifier() }, "next-tab" },
		{ { KEY_TAB, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "previous-tab" },
		{ { KEY_1, KeyMod::getDefaultModifier() }, "switch-to-tab-1" },
		{ { KEY_2, KeyMod::getDefaultModifier() }, "switch-to-tab-2" },
		{ { KEY_3, KeyMod::getDefaultModifier() }, "switch-to-tab-3" },
		{ { KEY_4, KeyMod::getDefaultModifier() }, "switch-to-tab-4" },
		{ { KEY_5, KeyMod::getDefaultModifier() }, "switch-to-tab-5" },
	};
}

StatusDebuggerController::StatusDebuggerController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode,
													PluginContextProvider* pluginContext ) :
	StatusBarElement( mainSplitter, uiSceneNode, pluginContext ) {}

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

	mUISceneNode->runOnMainThread( [this, state] {
		mUIButStart->setVisible( state == State::NotStarted )
			->setEnabled( state == State::NotStarted );
		mUIButStop->setVisible( state != State::NotStarted )
			->setEnabled( state != State::NotStarted );
		mUIButContinue->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		mUIButPause->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Running );
		mUIButStepOver->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		mUIButStepInto->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		mUIButStepOut->setVisible( state != State::NotStarted )
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
		<TabWidget id="app_debugger_tab_widget" lw="0" lw8="1" lh="mp">
			<Splitter id="debugger_threads_and_stack" layout_width="mp" lh="mp" splitter-partition="15%">
				<TableView id="debugger_threads" layout_width="mp" layout_height="mp" />
				<TableView id="debugger_stack" layout_width="mp" layout_height="mp" />
			</Splitter>
			<TreeView id="debugger_variables" layout_width="mp" layout_height="mp" />
			<TreeView id="debugger_expressions" layout_width="mp" layout_height="mp" />
			<BreakpointsTableView id="debugger_breakpoints" layout_width="mp" layout_height="mp" />
			<CodeEditor id="debugger_console" layout_width="mp" layout_height="mp" />
			<Tab id="debugger_tab_threads_and_stack" text="@string(threads_and_stack, Threads & Stack)" owns="debugger_threads_and_stack" />
			<Tab id="debugger_tab_variables" text="@string(variables, Variables)" owns="debugger_variables" />
			<Tab id="debugger_tab_expressions" text="@string(expressions, Expressions)" owns="debugger_expressions" />
			<Tab id="debugger_tab_breakpoints" text="@string(breakpoints, Breakpoints)" owns="debugger_breakpoints" />
			<Tab id="debugger_tab_console" text="@string(console_output, Console Output)" owns="debugger_console" />
		</TabWidget>
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

	mContainer->bind( "app_debugger_tab_widget", mUITabWidget );
	mContainer->bind( "debugger_threads_and_stack", mUIThreadsSplitter );
	mContainer->bind( "debugger_threads", mUIThreads );
	mContainer->bind( "debugger_stack", mUIStack );
	mContainer->bind( "debugger_breakpoints", mUIBreakpoints );
	mContainer->bind( "debugger_variables", mUIVariables );
	mContainer->bind( "debugger_expressions", mUIExpressions );
	mContainer->bind( "debugger_console", mUIConsole );
	mContainer->bind( "app_debugger_start", mUIButStart );
	mContainer->bind( "app_debugger_stop", mUIButStop );
	mContainer->bind( "app_debugger_pause", mUIButPause );
	mContainer->bind( "app_debugger_continue", mUIButContinue );
	mContainer->bind( "app_debugger_step_over", mUIButStepOver );
	mContainer->bind( "app_debugger_step_into", mUIButStepInto );
	mContainer->bind( "app_debugger_step_out", mUIButStepOut );

	mContainer->setCommand( "next-tab", [this] {
		if ( mUITabWidget )
			mUITabWidget->focusNextTab();
	} );

	mContainer->setCommand( "previous-tab", [this] {
		if ( mUITabWidget )
			mUITabWidget->focusPreviousTab();
	} );

	for ( int i = 1; i <= 5; i++ ) {
		mContainer->setCommand( String::format( "switch-to-tab-%d", i ), [this, i] {
			if ( mUITabWidget )
				mUITabWidget->setTabSelected(
					eeclamp<Int32>( i - 1, 0, mUITabWidget->getTabCount() - 1 ) );
		} );
	}

	mContainer->on( Event::KeyDown, [this]( const Event* event ) {
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

	mUIStack->setAutoColumnsWidth( true );
	mUIStack->setFitAllColumnsToWidget( true );
	mUIStack->setMainColumn( 1 );

	mUIVariables->setAutoColumnsWidth( true );
	mUIVariables->setFitAllColumnsToWidget( true );

	mUIBreakpoints->setAutoColumnsWidth( true );
	mUIBreakpoints->setFitAllColumnsToWidget( true );
	mUIBreakpoints->setMainColumn( 1 );

	mUIExpressions->setAutoColumnsWidth( true );
	mUIExpressions->setFitAllColumnsToWidget( true );
	mUIExpressions->setMainColumn( 1 );

	mUIConsole->setLocked( true );
	mUIConsole->setLineBreakingColumn( 0 );
	mUIConsole->setShowLineNumber( false );
	mUIConsole->getDocument().reset();
	mUIConsole->setScrollY( mUIConsole->getMaxScroll().y );
	mUIConsole->on( Event::OnScrollChange, [this]( auto ) {
		mScrollLocked = mUIConsole->getMaxScroll().y == mUIConsole->getScroll().y;
	} );
}

} // namespace ecode
