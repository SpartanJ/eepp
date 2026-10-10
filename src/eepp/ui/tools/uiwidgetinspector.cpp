#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/accessibility/accessibilitywidgetresolver.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/models/csspropertiesmodel.hpp>
#include <eepp/ui/models/widgettreemodel.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

#include <vector>

using namespace EE::Window;
using namespace EE::UI::Models;
using namespace EE::Scene;

namespace EE { namespace UI { namespace Tools {

namespace {

void appendAccessibilityFlag( std::string& output, const char* label, Uint64 value, Uint64 flags ) {
	if ( flags & value ) {
		if ( !output.empty() )
			output += ", ";
		output += label;
	}
}

class AccessibilityPropertiesModel final : public Model {
  public:
	static std::shared_ptr<AccessibilityPropertiesModel> create() {
		return std::make_shared<AccessibilityPropertiesModel>();
	}

	size_t rowCount( const ModelIndex& ) const override { return mData.size(); }

	size_t columnCount( const ModelIndex& ) const override { return 2; }

	std::string columnName( const size_t& column ) const override {
		return column == 0 ? "Property" : "Value";
	}

	Variant data( const ModelIndex& index, ModelRole role ) const override {
		if ( role != ModelRole::Display || index.row() < 0 ||
			 index.row() >= static_cast<Int64>( mData.size() ) )
			return {};
		return index.column() == 0 ? Variant( mData[index.row()].first )
								   : Variant( mData[index.row()].second );
	}

	void setWidget( UIWidget* widget ) {
		mConnections.clear();
		mWidget = widget;
		refresh();
		if ( !mWidget )
			return;
		static constexpr Uint32 Events[] = {
			Event::OnTextChanged,		Event::OnValueChange,	Event::OnSelectionChanged,
			Event::OnEnabledChange,		Event::OnVisibleChange, Event::OnPositionChange,
			Event::OnSizeChange,		Event::OnFocus,			Event::OnFocusLoss,
			Event::OnChildCountChanged,
		};
		for ( auto event : Events )
			mConnections += mWidget->connect( event, [this]( const Event* ) { refresh(); } );
		mConnections +=
			mWidget->connect( Event::OnClose, [this]( const Event* ) { setWidget( nullptr ); } );
	}

  private:
	void refresh() {
		mData.clear();
		if ( mWidget ) {
			auto manager = mWidget->getUISceneNode()->getAccessibilityManager();
			auto ref = manager->getNodeRef( mWidget );
			auto info = manager->getNodeInfo( ref );
			auto parent = manager->getParent( ref );
			std::string states;
			static constexpr std::pair<AccessibilityState, const char*> StateNames[] = {
				{ AccessibilityState::Active, "Active" },
				{ AccessibilityState::Enabled, "Enabled" },
				{ AccessibilityState::Focusable, "Focusable" },
				{ AccessibilityState::Focused, "Focused" },
				{ AccessibilityState::Checked, "Checked" },
				{ AccessibilityState::Selected, "Selected" },
				{ AccessibilityState::Editable, "Editable" },
				{ AccessibilityState::ReadOnly, "ReadOnly" },
				{ AccessibilityState::Visible, "Visible" },
				{ AccessibilityState::Showing, "Showing" },
				{ AccessibilityState::Expanded, "Expanded" },
			};
			for ( const auto& state : StateNames )
				appendAccessibilityFlag( states, state.second, static_cast<Uint64>( state.first ),
										 static_cast<Uint64>( info.states ) );
			std::string actions;
			static constexpr std::pair<AccessibilityAction, const char*> ActionNames[] = {
				{ AccessibilityAction::Focus, "Focus" },
				{ AccessibilityAction::Press, "Press" },
				{ AccessibilityAction::Toggle, "Toggle" },
				{ AccessibilityAction::Select, "Select" },
				{ AccessibilityAction::Increment, "Increment" },
				{ AccessibilityAction::Decrement, "Decrement" },
				{ AccessibilityAction::SetValue, "SetValue" },
				{ AccessibilityAction::SetText, "SetText" },
				{ AccessibilityAction::Expand, "Expand" },
				{ AccessibilityAction::Collapse, "Collapse" },
				{ AccessibilityAction::ScrollTo, "ScrollTo" },
				{ AccessibilityAction::SetTextSelection, "SetTextSelection" },
			};
			for ( const auto& action : ActionNames )
				appendAccessibilityFlag( actions, action.second,
										 accessibilityActionMask( action.first ), info.actions );
			static constexpr const char* LiveNames[] = { "Off", "Polite", "Assertive" };
			mData = {
				{ "Role", std::string( AccessibilityWidgetResolver::getRoleName( info.role ) ) +
							  " (" + mWidget->getElementTag() + ")" },
				{ "Name", info.name.toUtf8() },
				{ "Description", info.description.toUtf8() },
				{ "Value", info.value.toUtf8() },
				{ "Shortcut", info.shortcut.text },
				{ "Labelled By", mWidget->getAccessibilityLabelledBy() },
				{ "Described By", mWidget->getAccessibilityDescribedBy() },
				{ "Live", LiveNames[static_cast<int>( mWidget->getAccessibilityLive() )] },
				{ "States", states },
				{ "Actions", actions },
				{ "Source", String::toString( ref.source ) },
				{ "Stable ID", String::toString( ref.id ) },
				{ "Parent ID", String::toString( parent.id ) },
				{ "Children",
				  String::toString( static_cast<Uint64>( manager->getChildCount( ref ) ) ) },
				{ "Bounds", String::toString( info.bounds.Left ) + ", " +
								String::toString( info.bounds.Top ) + ", " +
								String::toString( info.bounds.Right ) + ", " +
								String::toString( info.bounds.Bottom ) },
			};
			if ( info.role != AccessibilityRole::None && info.name.empty() && info.value.empty() )
				mData.emplace_back( "Warning", "Exposed element has no accessible name or value." );
		}
		invalidate();
	}

	UIWidget* mWidget{ nullptr };
	Scene::EventConnectionList mConnections;
	std::vector<std::pair<std::string, std::string>> mData;
};

/** Lists the problems found by AccessibilityWidgetResolver::audit(). */
class AccessibilityAuditModel final : public Model {
  public:
	static std::shared_ptr<AccessibilityAuditModel> create() {
		return std::make_shared<AccessibilityAuditModel>();
	}

	size_t rowCount( const ModelIndex& ) const override { return mIssues.size(); }

	size_t columnCount( const ModelIndex& ) const override { return 2; }

	std::string columnName( const size_t& column ) const override {
		return column == 0 ? "Widget" : "Issue";
	}

	Variant data( const ModelIndex& index, ModelRole role ) const override {
		if ( role != ModelRole::Display || index.row() < 0 ||
			 index.row() >= static_cast<Int64>( mIssues.size() ) )
			return {};
		const auto& issue = mIssues[index.row()];
		return index.column() == 0 ? Variant( issue.label ) : Variant( issue.message );
	}

	void run( const UIWidget* root ) {
		mIssues.clear();
		for ( auto& issue : AccessibilityWidgetResolver::audit( root ) ) {
			std::string label( issue.widget->getElementTag() );
			if ( !issue.widget->getId().empty() )
				label += "#" + issue.widget->getId();
			mIssues.push_back( { issue.widget, std::move( label ), issue.message.toUtf8() } );
		}
		if ( mIssues.empty() )
			mIssues.push_back( { nullptr, "", "No issues found." } );
		invalidate();
	}

	/** The audited widget of a row. The pointer is only compared, never dereferenced, until it is
	 * found again in the live tree. */
	const UIWidget* widgetAt( Int64 row ) const {
		return row >= 0 && row < static_cast<Int64>( mIssues.size() ) ? mIssues[row].widget
																	  : nullptr;
	}

  private:
	struct Row {
		const UIWidget* widget;
		std::string label;
		std::string message;
	};
	std::vector<Row> mIssues;
};

bool containsNode( const Node* root, const Node* target ) {
	if ( root == target )
		return true;
	for ( const Node* child = root->getFirstChild(); child; child = child->getNextNode() ) {
		if ( containsNode( child, target ) )
			return true;
	}
	return false;
}

} // namespace

struct UIWidgetInspector::PickHighlightOverState {
	std::vector<std::pair<UISceneNode*, bool>> sceneStates;

	void capture( UISceneNode* sceneNode ) {
		if ( !sceneNode )
			return;

		sceneStates.emplace_back( sceneNode, sceneNode->getHighlightOver() );

		for ( auto* childSceneNode : sceneNode->getChildUISceneNodes() )
			capture( childSceneNode );
	}

	void restore() {
		for ( auto& sceneState : sceneStates )
			sceneState.first->setHighlightOver( sceneState.second );
	}
};

UIWindow* UIWidgetInspector::create( UISceneNode* sceneNode, const Float& menuIconSize,
									 std::function<void()> highlightToggle,
									 std::function<void()> drawBoxesToggle,
									 std::function<void()> drawDebugDataToggle ) {
	static ModelIndex lastModelIndex = {};
	auto wtv = sceneNode->getRoot()->hasChild( "widget-tree-view" );
	if ( wtv ) {
		wtv->toFront();
		return nullptr;
	}
	UIWindow* uiWin = UIWindow::New();
	uiWin->setId( "widget-tree-view" );
	uiWin->setMinWindowSize( 600, 400 );
	uiWin->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );
	static const auto WIDGET_LAYOUT = R"xml(
	<vbox lw="mp" lh="mp">
		<hbox lw="wc" lh="wc">
			<PushButton id="pick_widget" lh="18dp" icon="icon(inspect, 12dp)" text='@string(pick_widget, "Pick Widget")' text-as-fallback="true" />
			<CheckBox id="debug-draw-highlight" text='@string(debug_draw_highlight, "Highlight Focus & Hover")' margin-left="4dp" lg="center" />
			<CheckBox id="debug-draw-boxes" text='@string(debug_draw_boxes, "Draw Boxes")' margin-left="4dp" lg="center" />
			<CheckBox id="debug-draw-debug-data" text='@string(debug_draw_debug_data, "Draw Debug Data")' margin-left="4dp" lg="center" />"
			<PushButton id="widget-tree-search-collapse" layout_width="wrap_content" layout_height="18dp" tooltip='@string(collapse_all, "Collapse All")' margin-left="8dp" icon="menu-fold" text-as-fallback="true" />
			<PushButton id="widget-tree-search-expand" layout_width="wrap_content" layout_height="18dp" tooltip='@string(expand_all, "Expand All")' margin-left="8dp" icon="menu-unfold" text-as-fallback="true" />
			<PushButton id="open-texture-viewer" lh="18dp" text="@string(texture_viewer, Texture Viewer)" margin-left="8dp" />
		</hbox>
		<Splitter layout_width="match_parent" lh="fixed" lw8="1" splitter-partition="50%">
			<TreeView id="widget_inspector_nodetree" lw="fixed" lh="mp" />
			<TabWidget lw="fixed" lh="mp">
				<TableView id="widget_inspector_computed" class="computed" lw="mp" lh="mp" />
				<CodeEditor id="widget_inspector_style" lw="mp" lh="mp" />
				<TableView id="widget_inspector_accessibility" class="computed" lw="mp" lh="mp" />
				<vbox id="widget_inspector_audit" lw="mp" lh="mp">
					<PushButton id="widget_inspector_audit_run" lh="18dp" text="@string(run_accessibility_audit, Run Audit)" />
					<TableView id="widget_inspector_audit_issues" class="computed" lw="mp" lh="fixed" lw8="1" />
				</vbox>
				<Tab id="widget_inspector_tab_computed" text="@string(computed, Computed)" owns="widget_inspector_computed" />
				<Tab id="widget_inspector_tab_style" text="@string(style, Style)" owns="widget_inspector_style" />
				<Tab text="Accessibility" owns="widget_inspector_accessibility" />
				<Tab text="@string(accessibility_audit, Audit)" owns="widget_inspector_audit" />
			</TabWidget>
		</Splitter>
	</vbox>
	)xml";
	UIWidget* cont = sceneNode->loadLayoutFromString( WIDGET_LAYOUT, uiWin->getContainer() );
	UITreeView* nodeTree = cont->find<UITreeView>( "widget_inspector_nodetree" );
	nodeTree->on( Event::OnRowCreated, [sceneNode]( const Event* event ) {
		UITableRow* row = event->asRowCreatedEvent()->getRow();
		row->on( Event::MouseOver, [row, sceneNode]( const Event* ) {
			if ( lastModelIndex.isValid() && lastModelIndex != row->getCurIndex() &&
				 sceneNode->getRoot()->inNodeTree( lastModelIndex.ref<UINode>() ) ) {
				lastModelIndex.ref<UINode>()->unsetFlags( UI_HIGHLIGHT );
			}
			if ( row->getCurIndex().internalData() && row->getCurIndex().ref<Node>()->isUINode() ) {
				row->getCurIndex().ref<UINode>()->setFlags( UI_HIGHLIGHT );
				lastModelIndex = row->getCurIndex();
			}
		} );
		row->on( Event::MouseLeave, [row, sceneNode]( const Event* ) {
			if ( row->getCurIndex().internalData() &&
				 sceneNode->getRoot()->inNodeTree( row->getCurIndex().ref<UINode>() ) &&
				 row->getCurIndex().ref<Node>()->isUINode() )
				row->getCurIndex().ref<UINode>()->unsetFlags( UI_HIGHLIGHT );
		} );
	} );
	nodeTree->setHeadersVisible( true );
	nodeTree->setExpanderIconSize( menuIconSize );
	nodeTree->setAutoColumnsWidth( true );
	auto model = WidgetTreeModel::New( sceneNode );
	nodeTree->setModel( model );
	nodeTree->tryOpenModelIndex( model->getRoot() );

	UICodeEditor* stylesEditor = cont->find<UICodeEditor>( "widget_inspector_style" );
	stylesEditor->setLocked( true );
	stylesEditor->setShowLineNumber( false );
	stylesEditor->setShowFoldingRegion( false );
	stylesEditor->setLineWrapType( LineWrapType::Viewport );
	stylesEditor->setLineWrapMode( LineWrapMode::Word );
	stylesEditor->setLineWrapKeepIndentation( true );
	stylesEditor->setColorPreview( true );
	stylesEditor->setColorScheme( sceneNode->getColorSchemePreference() ==
										  ColorSchemePreference::Dark
									  ? SyntaxColorScheme::getDefaultDark()
									  : SyntaxColorScheme::getDefaultLight() );
	UITableView* accessibilityView = cont->find<UITableView>( "widget_inspector_accessibility" );
	accessibilityView->setAutoColumnsWidth( true );
	accessibilityView->setHeadersVisible( true );
	auto accessibilityModel = AccessibilityPropertiesModel::create();
	accessibilityView->setModel( accessibilityModel );

	UITableView* auditView = cont->find<UITableView>( "widget_inspector_audit_issues" );
	auditView->setAutoColumnsWidth( true );
	auditView->setHeadersVisible( true );
	auto auditModel = AccessibilityAuditModel::create();
	auditView->setModel( auditModel );
	cont->find( "widget_inspector_audit_run" )
		->on( Event::MouseClick, [sceneNode, auditModel]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK )
				auditModel->run( sceneNode->getRoot() );
		} );
	auditView->setOnSelection( [sceneNode, nodeTree, auditModel]( const ModelIndex& index ) {
		const UIWidget* widget = auditModel->widgetAt( index.row() );
		// The tree may have changed since the audit ran: select only widgets still in it.
		if ( !widget || !containsNode( sceneNode->getRoot(), widget ) )
			return;
		auto* treeModel = static_cast<WidgetTreeModel*>( nodeTree->getModel() );
		nodeTree->setSelection( treeModel->getModelIndex( widget ) );
	} );

	UITableView* computedView = cont->find<UITableView>( "widget_inspector_computed" );
	computedView->setAutoColumnsWidth( true );
	computedView->setHeadersVisible( true );
	nodeTree->setOnSelection(
		[computedView, stylesEditor, accessibilityModel]( const ModelIndex& index ) {
			Node* node = static_cast<Node*>( index.internalData() );
			computedView->setModel( node->isWidget()
										? CSSPropertiesModel::create( node->asType<UIWidget>() )
										: CSSPropertiesModel::create() );

			stylesEditor->getDocument().reset();
			stylesEditor->getDocument().setSyntaxDefinition(
				SyntaxDefinitionManager::instance()->getByLSPName( "css" ) );
			accessibilityModel->setWidget( node->isWidget() ? node->asType<UIWidget>() : nullptr );

			if ( node->isWidget() ) {
				UIWidget* widget = node->asType<UIWidget>();
				if ( widget->getUIStyle() && widget->getUIStyle()->getDefinition() ) {
					const auto& styles = widget->getUIStyle()->getDefinition()->getStyles();
					String elemStyle;
					for ( const auto& style : styles ) {
						if ( style->getSelector().getName() != ":root" ||
							 widget->getElementTag() == ":root" )
							elemStyle += style->build( false, false );
					}
					stylesEditor->getDocument().textInput( elemStyle );
				}
			}
		} );

	UIPushButton* button = cont->find<UIPushButton>( "pick_widget" );

	if ( button->getIcon() == nullptr ) {
		DrawablePtr cursorPointer = button->getUISceneNode()->findIconDrawable(
			"cursor-pointer", PixelDensity::dpToPx( 16 ) );

		if ( cursorPointer )
			button->setIcon( std::move( cursorPointer ) );
	}

	button->on( Event::MouseClick, [sceneNode, nodeTree, computedView]( const Event* event ) {
		if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
			auto highlightOverState = std::make_shared<PickHighlightOverState>();
			highlightOverState->capture( sceneNode );
			sceneNode->setHighlightOverRecursive( true );
			sceneNode->getEventDispatcher()->setDisableMousePress( true );
			sceneNode->runOnMainThread( [sceneNode, nodeTree, computedView, highlightOverState]() {
				checkWidgetPick( sceneNode, nodeTree, highlightOverState, computedView );
			} );
		}
	} );

	cont->find<UICheckBox>( "debug-draw-highlight" )
		->setChecked( sceneNode->getHighlightOver() )
		->on( Event::OnValueChange, [sceneNode, highlightToggle]( const auto* ) {
			if ( highlightToggle ) {
				highlightToggle();
			} else {
				bool highlight = !sceneNode->getHighlightOver();
				sceneNode->setHighlightFocusRecursive( highlight );
				sceneNode->setHighlightOverRecursive( highlight );
			}
		} );

	cont->find<UICheckBox>( "debug-draw-boxes" )
		->setChecked( sceneNode->getDrawBoxes() )
		->on( Event::OnValueChange, [sceneNode, drawBoxesToggle]( const auto* ) {
			if ( drawBoxesToggle ) {
				drawBoxesToggle();
			} else {
				sceneNode->setDrawBoxesRecursive( !sceneNode->getDrawBoxes() );
			}
		} );

	cont->find<UICheckBox>( "debug-draw-debug-data" )
		->setChecked( sceneNode->getDrawDebugData() )
		->on( Event::OnValueChange, [sceneNode, drawDebugDataToggle]( const auto* ) {
			if ( drawDebugDataToggle ) {
				drawDebugDataToggle();
			} else {
				sceneNode->setDrawDebugDataRecursive( !sceneNode->getDrawDebugData() );
			}
		} );

	cont->find<UIPushButton>( "widget-tree-search-collapse" )
		->on( Event::MouseClick, [nodeTree]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
				nodeTree->collapseAll();
			}
		} );

	cont->find<UIPushButton>( "widget-tree-search-expand" )
		->on( Event::MouseClick, [nodeTree]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
				nodeTree->expandAll();
			}
		} );

	cont->find<UIPushButton>( "open-texture-viewer" )->onClick( []( auto ) {
		auto win = SceneManager::instance()->getUISceneNode()->loadLayoutFromString( R"xml(
		<window layout_width="830dp" layout_height="600dp" winflags="default|maximize|shadow" window-title="@string(texture_viewer, Texture Viewer)">
			<TextureViewer layout_width="match_parent" layout_height="match_parent" />
		</window>
	)xml" );
		win->center()->runOnMainThread( [win] { win->toFront(); }, Milliseconds( 1 ) );
	} );

	uiWin->center();

	Uint32 winCb = sceneNode->on( Event::OnWindowAdded, [sceneNode, uiWin]( const Event* event ) {
		UIWindow* eWin = event->asWindowEvent()->getWindow()->asType<UIWindow>();
		if ( eWin != uiWin ) {
			Uint32 winRdCb = eWin->on( Event::OnWindowReady, [uiWin]( const Event* eWinEvent ) {
				uiWin->toFront();
				eWinEvent->getNode()->removeEventListener( eWinEvent->getCallbackId() );
			} );
			uiWin->on( Event::OnWindowClose, [sceneNode, winRdCb]( const Event* ) {
				if ( !SceneManager::instance()->isShuttingDown() )
					sceneNode->removeEventListener( winRdCb );
			} );
		}
	} );
	uiWin->on( Event::OnWindowClose, [sceneNode, winCb]( const Event* ) {
		if ( !SceneManager::instance()->isShuttingDown() )
			sceneNode->removeEventListener( winCb );
	} );
	return uiWin;
}

void UIWidgetInspector::checkWidgetPick( UISceneNode* sceneNode, UITreeView* widgetTree,
										 std::shared_ptr<PickHighlightOverState> highlightOverState,
										 UITableView* tableView ) {
	Input* input = sceneNode->getWindow()->getInput();
	if ( input->getClickTrigger() & EE_BUTTON_LMASK ) {
		Node* node = sceneNode->getEventDispatcher()->getMouseOverNode();
		WidgetTreeModel* model = static_cast<WidgetTreeModel*>( widgetTree->getModel() );
		ModelIndex index( model->getModelIndex( node ) );
		widgetTree->setSelection( index );
		highlightOverState->restore();
		sceneNode->getEventDispatcher()->setDisableMousePress( false );
	} else {
		sceneNode->runOnMainThread( [sceneNode, widgetTree, highlightOverState, tableView]() {
			checkWidgetPick( sceneNode, widgetTree, highlightOverState, tableView );
		} );
	}
}

}}} // namespace EE::UI::Tools
