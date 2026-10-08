#include <eepp/scene/scenemanager.hpp>
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
				<Tab id="widget_inspector_tab_computed" text="@string(computed, Computed)" owns="widget_inspector_computed" />
				<Tab id="widget_inspector_tab_style" text="@string(style, Style)" owns="widget_inspector_style" />
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

	UITableView* computedView = cont->find<UITableView>( "widget_inspector_computed" );
	computedView->setAutoColumnsWidth( true );
	computedView->setHeadersVisible( true );
	nodeTree->setOnSelection( [computedView, stylesEditor]( const ModelIndex& index ) {
		Node* node = static_cast<Node*>( index.internalData() );
		computedView->setModel( node->isWidget()
									? CSSPropertiesModel::create( node->asType<UIWidget>() )
									: CSSPropertiesModel::create() );

		stylesEditor->getDocument().reset();
		stylesEditor->getDocument().setSyntaxDefinition(
			SyntaxDefinitionManager::instance()->getByLSPName( "css" ) );

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
