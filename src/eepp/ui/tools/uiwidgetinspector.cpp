#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/models/csspropertiesmodel.hpp>
#include <eepp/ui/models/widgettreemodel.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

using namespace EE::Window;
using namespace EE::UI::Models;
using namespace EE::Scene;

namespace EE { namespace UI { namespace Tools {

UIWindow* UIWidgetInspector::create( UISceneNode* sceneNode, const Float& menuIconSize,
									 std::function<void()> highlightToggle,
									 std::function<void()> drawBoxesToggle,
									 std::function<void()> drawDebugDataToggle ) {
	static ModelIndex lastModelIndex = {};
	if ( sceneNode->getRoot()->hasChild( "widget-tree-view" ) )
		return nullptr;
	UIWindow* uiWin = UIWindow::New();
	uiWin->setId( "widget-tree-view" );
	uiWin->setMinWindowSize( 600, 400 );
	uiWin->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_RESIZEABLE | UI_WIN_MAXIMIZE_BUTTON );
	static const auto WIDGET_LAYOUT = R"xml(
	<vbox lw="mp" lh="mp">
		<hbox lw="wc" lh="wc">
			<PushButton id="pick_widget" lh="18dp" icon="icon(cursor-pointer, 16dp)" text='@string(pick_widget, "Pick Widget")' text-as-fallback="true" />
			<CheckBox id="debug-draw-highlight" text='@string(debug_draw_highlight, "Highlight Focus & Hover")' margin-left="4dp" lg="center" />
			<CheckBox id="debug-draw-boxes" text='@string(debug_draw_boxes, "Draw Boxes")' margin-left="4dp" lg="center" />
			<CheckBox id="debug-draw-debug-data" text='@string(debug_draw_debug_data, "Draw Debug Data")' margin-left="4dp" lg="center" />"
			<PushButton id="widget-tree-search-collapse" layout_width="wrap_content" layout_height="18dp" tooltip='@string(collapse_all, "Collapse All")' margin-left="8dp" icon="menu-fold" text-as-fallback="true" />
			<PushButton id="widget-tree-search-expand" layout_width="wrap_content" layout_height="18dp" tooltip='@string(expand_all, "Expand All")' margin-left="8dp" icon="menu-unfold" text-as-fallback="true" />
			<PushButton id="open-texture-viewer" lh="18dp" text="@string(texture_viewer, Texture Viewer)" margin-left="8dp" />
		</hbox>
		<Splitter layout_width="match_parent" lh="fixed" lw8="1" splitter-partition="50%">
			<treeview lw="fixed" lh="mp" />
			<tableview lw="fixed" lh="mp" />
		</Splitter>
	</vbox>
	)xml";
	UIWidget* cont = sceneNode->loadLayoutFromString( WIDGET_LAYOUT, uiWin->getContainer() );
	UITreeView* widgetTree = cont->findByType<UITreeView>( UI_TYPE_TREEVIEW );
	widgetTree->on( Event::OnRowCreated, [sceneNode]( const Event* event ) {
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
		row->on( Event::MouseLeave, [row]( const Event* ) {
			if ( row->getCurIndex().internalData() && row->getCurIndex().ref<Node>()->isUINode() )
				row->getCurIndex().ref<UINode>()->unsetFlags( UI_HIGHLIGHT );
		} );
	} );
	widgetTree->setHeadersVisible( true );
	widgetTree->setExpanderIconSize( menuIconSize );
	widgetTree->setAutoColumnsWidth( true );
	auto model = WidgetTreeModel::New( sceneNode );
	widgetTree->setModel( model );
	widgetTree->tryOpenModelIndex( model->getRoot() );
	UITableView* tableView = cont->findByType<UITableView>( UI_TYPE_TABLEVIEW );
	tableView->setAutoColumnsWidth( true );
	tableView->setHeadersVisible( true );
	widgetTree->setOnSelection( [tableView]( const ModelIndex& index ) {
		Node* node = static_cast<Node*>( index.internalData() );
		if ( node->isWidget() ) {
			tableView->setModel( node->isWidget()
									 ? CSSPropertiesModel::create( node->asType<UIWidget>() )
									 : CSSPropertiesModel::create() );
		}
	} );

	UIPushButton* button = cont->find<UIPushButton>( "pick_widget" );
	button->addEventListener(
		Event::MouseClick, [sceneNode, widgetTree, tableView]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
				bool wasHighlightOver = sceneNode->getHighlightOver();
				sceneNode->setHighlightOver( true );
				sceneNode->getEventDispatcher()->setDisableMousePress( true );
				sceneNode->runOnMainThread( [sceneNode, widgetTree, tableView, wasHighlightOver]() {
					checkWidgetPick( sceneNode, widgetTree, wasHighlightOver, tableView );
				} );
			}
		} );

	cont->find<UICheckBox>( "debug-draw-highlight" )
		->setChecked( sceneNode->getHighlightOver() )
		->addEventListener( Event::OnValueChange, [sceneNode, highlightToggle]( const auto* ) {
			if ( highlightToggle ) {
				highlightToggle();
			} else {
				sceneNode->setHighlightFocus( !sceneNode->getHighlightFocus() );
				sceneNode->setHighlightOver( !sceneNode->getHighlightOver() );
			}
		} );

	cont->find<UICheckBox>( "debug-draw-boxes" )
		->setChecked( sceneNode->getDrawBoxes() )
		->addEventListener( Event::OnValueChange, [sceneNode, drawBoxesToggle]( const auto* ) {
			if ( drawBoxesToggle ) {
				drawBoxesToggle();
			} else {
				sceneNode->setDrawBoxes( !sceneNode->getDrawBoxes() );
			}
		} );

	cont->find<UICheckBox>( "debug-draw-debug-data" )
		->setChecked( sceneNode->getDrawDebugData() )
		->addEventListener( Event::OnValueChange, [sceneNode, drawDebugDataToggle]( const auto* ) {
			if ( drawDebugDataToggle ) {
				drawDebugDataToggle();
			} else {
				sceneNode->setDrawDebugData( !sceneNode->getDrawDebugData() );
			}
		} );

	cont->find<UIPushButton>( "widget-tree-search-collapse" )
		->addEventListener( Event::MouseClick, [widgetTree]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
				widgetTree->collapseAll();
			}
		} );

	cont->find<UIPushButton>( "widget-tree-search-expand" )
		->addEventListener( Event::MouseClick, [widgetTree]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_LMASK ) {
				widgetTree->expandAll();
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

	Uint32 winCb = sceneNode->addEventListener( Event::OnWindowAdded, [sceneNode, uiWin](
																		  const Event* event ) {
		UIWindow* eWin = event->asWindowEvent()->getWindow()->asType<UIWindow>();
		if ( eWin != uiWin ) {
			Uint32 winRdCb =
				eWin->addEventListener( Event::OnWindowReady, [uiWin]( const Event* eWinEvent ) {
					uiWin->toFront();
					eWinEvent->getNode()->removeEventListener( eWinEvent->getCallbackId() );
				} );
			uiWin->addEventListener( Event::OnWindowClose, [sceneNode, winRdCb]( const Event* ) {
				if ( !SceneManager::instance()->isShuttingDown() )
					sceneNode->removeEventListener( winRdCb );
			} );
		}
	} );
	uiWin->addEventListener( Event::OnWindowClose, [sceneNode, winCb]( const Event* ) {
		if ( !SceneManager::instance()->isShuttingDown() )
			sceneNode->removeEventListener( winCb );
	} );
	return uiWin;
}

void UIWidgetInspector::checkWidgetPick( UISceneNode* sceneNode, UITreeView* widgetTree,
										 bool wasHighlightOver, UITableView* tableView ) {
	Input* input = sceneNode->getWindow()->getInput();
	if ( input->getClickTrigger() & EE_BUTTON_LMASK ) {
		Node* node = sceneNode->getEventDispatcher()->getMouseOverNode();
		WidgetTreeModel* model = static_cast<WidgetTreeModel*>( widgetTree->getModel() );
		ModelIndex index( model->getModelIndex( node ) );
		widgetTree->setSelection( index );
		sceneNode->setHighlightOver( wasHighlightOver );
		sceneNode->getEventDispatcher()->setDisableMousePress( false );
	} else {
		sceneNode->runOnMainThread( [sceneNode, widgetTree, wasHighlightOver, tableView]() {
			checkWidgetPick( sceneNode, widgetTree, wasHighlightOver, tableView );
		} );
	}
}

}}} // namespace EE::UI::Tools
