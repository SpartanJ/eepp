#include "utest.hpp"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/csspropertiesmodel.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/models/widgettreemodel.hpp>
#include <eepp/ui/tools/uiwidgetinspector.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwindow.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::UI::Models;

namespace {

WindowSettings inspectionWindowSettings() {
	return WindowSettings( 640, 480, "CSS inspection", WindowStyle::Default, WindowBackend::Default,
						   32 );
}

UIApplication::Settings inspectionSettings() {
	return UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
}

Int64 propertyRow( const CSSPropertiesModel& model, const char* name ) {
	for ( size_t row = 0; row < model.rowCount(); ++row ) {
		if ( model.data( model.index( row, 0 ) ).toString() == name )
			return row;
	}
	return -1;
}

} // namespace

UTEST( CSSPropertiesModel, inheritedImageAlignmentReportsValueAndWinningAncestor ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<style>
			#origin { text-align: right; --alignment: left; }
			#origin { text-align: var(--alignment); }
		</style>
		<vbox id="origin"><vbox id="middle"><Image id="target" /></vbox></vbox>
	)xml" );
	auto* image = app.getUI()->find<UIImage>( "target" );
	auto* origin = app.getUI()->find<UIWidget>( "origin" );
	auto model = CSSPropertiesModel::create( image );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	EXPECT_STDSTREQ( "Property", model->columnName( 0 ) );
	EXPECT_STDSTREQ( "Value", model->columnName( 1 ) );
	EXPECT_STDSTREQ( "left", model->data( model->index( row, 1 ) ).toString() );
	EXPECT_EQ( origin, model->sourceWidgetAt( row ) );
	EXPECT_TRUE( model->data( model->index( row, 2 ) )
					 .toString()
					 .find( "Inherited from linearlayout#origin" ) != std::string::npos );
	EXPECT_TRUE( model->getStyleText().find( "Inherited from linearlayout#origin" ) !=
				 std::string::npos );
	EXPECT_TRUE( model->getStyleText().find( "text-align: right; /* overridden */" ) !=
				 std::string::npos );
	EXPECT_TRUE( model->getStyleText().find( "text-align: var(--alignment);" ) !=
				 std::string::npos );
	EXPECT_TRUE( model->getStyleText().find( "--alignment: left;" ) != std::string::npos );
	EXPECT_FALSE( model->refresh() );
}

UTEST( CSSPropertiesModel, localAndExplicitInheritanceRespectCurrentCascade ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<style>
			#origin { text-align: right; opacity: 0.6; }
			#target { text-align: left; opacity: inherit; }
		</style>
		<vbox id="origin"><Image id="target" style="text-align: center" /></vbox>
	)xml" );
	auto* image = app.getUI()->find<UIImage>( "target" );
	auto* origin = app.getUI()->find<UIWidget>( "origin" );
	auto model = CSSPropertiesModel::create( image );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	EXPECT_STDSTREQ( "center", model->data( model->index( row, 1 ) ).toString() );
	EXPECT_STDSTREQ( "Element style", model->data( model->index( row, 2 ) ).toString() );
	EXPECT_EQ( image, model->sourceWidgetAt( row ) );
	EXPECT_EQ( origin, model->sourceWidgetAt( propertyRow( *model, "opacity" ) ) );
	EXPECT_TRUE( model->getStyleText().find( "text-align: right; /* overridden */" ) !=
				 std::string::npos );
	EXPECT_TRUE( model->getStyleText().find( "opacity: inherit;\n" ) != std::string::npos );
}

UTEST( CSSPropertiesModel, updatesVariablesPseudoStatesAndReparenting ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	app.getUI()->setStyleSheet( R"css(
			#origin { --alignment: left; text-align: var(--alignment); }
			#origin:selected { text-align: right; }
			#other { text-align: center; }
	)css" );
	app.getUI()->loadLayoutFromString( R"xml(
		<vbox id="origin"><Image id="target" /></vbox><vbox id="other" />
	)xml" );
	auto* image = app.getUI()->find<UIImage>( "target" );
	auto* origin = app.getUI()->find<UIWidget>( "origin" );
	auto model = CSSPropertiesModel::create( image );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	app.getUI()->setStyleSheet( R"css(
		#origin { --alignment: center; text-align: var(--alignment); }
		#origin:selected { text-align: right; }
		#other { text-align: center; }
	)css" );
	origin->reloadStyle( true, true, true, true, true );
	image->getUIStyle()->applyInheritedProperties();
	app.getUI()->update( Milliseconds( 16 ) );
	model->refresh();
	EXPECT_STDSTREQ( "center", model->data( model->index( row, 1 ) ).toString() );
	origin->pushState( UIState::StateSelected );
	app.getUI()->update( Milliseconds( 16 ) );
	model->refresh();
	EXPECT_STDSTREQ( "right", model->data( model->index( row, 1 ) ).toString() );
	EXPECT_TRUE( model->data( model->index( row, 2 ) ).toString().find( "#origin:selected" ) !=
				 std::string::npos );
	image->setParent( app.getUI()->find<UIWidget>( "other" ) );
	image->reloadStyle( true, true, true );
	image->getUIStyle()->applyInheritedProperties();
	app.getUI()->update( Milliseconds( 16 ) );
	model->refresh();
	EXPECT_EQ( app.getUI()->find<UIWidget>( "other" ), model->sourceWidgetAt( row ) );
	EXPECT_STDSTREQ( "center", model->data( model->index( row, 1 ) ).toString() );
}

UTEST( CSSPropertiesModel, sortingAllColumnsPreservesSourceMapping ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<style>#origin { text-align: right; }</style>
		<vbox id="origin"><Image id="target" /></vbox>
	)xml" );
	auto model = CSSPropertiesModel::create( app.getUI()->find<UIWidget>( "target" ) );
	auto proxy = SortingProxyModel::New( model );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	for ( size_t column = 0; column < 3; ++column ) {
		for ( auto order : { SortOrder::Ascending, SortOrder::Descending } ) {
			proxy->sort( column, order );
			const auto mapped = proxy->mapToProxy( model->index( row, 2 ) );
			EXPECT_EQ( row, proxy->mapToSource( mapped ).row() );
			EXPECT_EQ( app.getUI()->find<UIWidget>( "origin" ),
					   model->sourceWidgetAt( proxy->mapToSource( mapped ).row() ) );
			for ( size_t i = 1; i < proxy->rowCount(); ++i ) {
				const auto previous = proxy->data( proxy->index( i - 1, column ), ModelRole::Sort );
				const auto current = proxy->data( proxy->index( i, column ), ModelRole::Sort );
				EXPECT_FALSE( previous.toString().empty() && current.toString().empty() &&
							  column == 0 );
				// Proxy ordering is case insensitive by default, like its string comparison.
				const auto a = String::toLower( previous.toString() );
				const auto b = String::toLower( current.toString() );
				EXPECT_TRUE( order == SortOrder::Ascending ? a <= b : a >= b );
			}
		}
	}
}

UTEST( CSSPropertiesModel, nativeAlignmentAndWidgetDestructionRemainInspectable ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	auto* image = UIImage::New();
	image->setParent( app.getUI()->getRoot() );
	image->setHorizontalAlignment( UIHorizontalAlignment::End );
	image->setVerticalAlign( UI_VALIGN_BOTTOM );
	const auto flags = image->getFlags();
	auto model = CSSPropertiesModel::create( image );
	EXPECT_EQ( flags, image->getFlags() );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	EXPECT_STDSTREQ( "end", model->data( model->index( row, 1 ) ).toString() );
	EXPECT_STDSTREQ( "Native / default", model->data( model->index( row, 2 ) ).toString() );
	EXPECT_TRUE(
		image->getPropertyString( StyleSheetSpecification::instance()->getProperty( "gravity" ) )
			.find( "bottom" ) != std::string::npos );
	image->close();
	SceneManager::instance()->update();
	EXPECT_EQ( nullptr, model->getWidget() );
	EXPECT_EQ( 0u, model->rowCount() );
	EXPECT_TRUE( model->getStyleText().empty() );
	model->setWidget( app.getUI()->getRoot() );
	EXPECT_GT( model->rowCount(), 0u );
	model->setWidget( nullptr );
	EXPECT_EQ( 0u, model->rowCount() );
}

UTEST( CSSPropertiesModel, inspectorNavigatesSortedSourcesAndClearsClosedWidget ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<style>#origin { text-align: right; }</style>
		<vbox id="origin"><Image id="target" /></vbox>
	)xml" );
	auto* target = app.getUI()->find<UIImage>( "target" );
	auto* origin = app.getUI()->find<UIWidget>( "origin" );
	auto* inspector = Tools::UIWidgetInspector::create( app.getUI() );
	auto* tree = inspector->find<UITreeView>( "widget_inspector_nodetree" );
	auto* view = inspector->find<UITableView>( "widget_inspector_computed" );
	auto* editor = inspector->find<UICodeEditor>( "widget_inspector_style" );
	auto* treeModel = static_cast<WidgetTreeModel*>( tree->getModel() );
	auto* proxy = static_cast<SortingProxyModel*>( view->getModel() );
	auto model = std::static_pointer_cast<CSSPropertiesModel>( proxy->getSource() );
	tree->setSelection( treeModel->getModelIndex( target ) );
	EXPECT_EQ( target, model->getWidget() );
	EXPECT_EQ( 0, proxy->keyColumn() );
	proxy->sort( 2, SortOrder::Descending );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	const auto index = proxy->mapToProxy( model->index( row, 2 ) );
	view->setSelection( index );
	for ( int frame = 0; frame < 3; ++frame ) {
		app.getUI()->update( Milliseconds( 16 ) );
		app.getUI()->draw();
	}
	view->scrollToIndex( index );
	app.getUI()->update( Milliseconds( 16 ) );
	app.getUI()->draw();
	auto* cell = view->getCellFromIndex( index );
	ASSERT_NE( nullptr, cell );
	cell->sendMouseEvent( Event::MouseClick, { 0, 0 }, EE_BUTTON_LMASK );
	EXPECT_EQ( origin, model->getWidget() );
	EXPECT_EQ( 2, proxy->keyColumn() );
	EXPECT_TRUE( proxy->sortOrder() == SortOrder::Descending );
	tree->setSelection( treeModel->getModelIndex( target ) );
	target->close();
	SceneManager::instance()->update();
	EXPECT_EQ( nullptr, model->getWidget() );
	// Runnable intervals use wall-clock time rather than the scene's simulated delta.
	Sys::sleep( Milliseconds( 275 ) );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_TRUE( editor->getDocument().isEmpty() );
	inspector->close();
	SceneManager::instance()->update();
}

UTEST( CSSPropertiesModel, inheritanceChainsIncludeResolvedKeywordsAndIntermediateDeclarations ) {
	UIApplication app( inspectionWindowSettings(), inspectionSettings() );
	app.getUI()->loadLayoutFromString( R"xml(
		<style>
			#origin { text-align: right; }
			#middle { text-align: inherit; }
		</style>
		<vbox id="origin"><vbox id="middle">
			<Image id="target" style="--alignment: inherit; text-align: var(--alignment)" />
		</vbox></vbox>
	)xml" );
	auto* image = app.getUI()->find<UIImage>( "target" );
	auto model = CSSPropertiesModel::create( image );
	const auto row = propertyRow( *model, "text-align" );
	ASSERT_GE( row, 0 );
	EXPECT_EQ( app.getUI()->find<UIWidget>( "origin" ), model->sourceWidgetAt( row ) );
	EXPECT_TRUE( model->getStyleText().find( "text-align: inherit;\n" ) != std::string::npos );
	EXPECT_TRUE( model->getStyleText().find( "text-align: var(--alignment);\n" ) !=
				 std::string::npos );
	image->setStyleSheetInlineProperty( "text-align", "center" );
	image->reloadStyle( true, true, true, true );
	model->refresh();
	EXPECT_EQ( image, model->sourceWidgetAt( row ) );
	EXPECT_TRUE( model->getStyleText().find( "text-align: inherit; /* overridden */" ) !=
				 std::string::npos );
}
