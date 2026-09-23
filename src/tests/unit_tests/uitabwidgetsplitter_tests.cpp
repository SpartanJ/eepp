#include "tabtransfer.hpp"
#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>
#include <nlohmann/json.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::UI::Tools;

class TestClient : public UITabWidgetSplitter::Client {
  public:
	int tabCreatedCount{ 0 };
	int focusChangeCount{ 0 };

	void onTabCreated( UITab*, UIWidget* ) override { tabCreatedCount++; }

	void onWidgetFocusChange( UIWidget* ) override { focusChangeCount++; }
};

class TransferTestTabWidget : public UITabWidget {
  public:
	static TransferTestTabWidget* New() { return eeNew( TransferTestTabWidget, () ); }

	void detachTab( UITab* tab ) { removeTab( tab, false, false, false ); }

	const Sizef& getContainerPixelsSize() const { return mNodeContainer->getPixelsSize(); }
};

class ThreeColumnModel : public Model {
  public:
	explicit ThreeColumnModel( size_t rows = 1 ) : mRows( rows ) {}
	void setRows( size_t rows ) { mRows = rows; }

	size_t rowCount( const ModelIndex& = ModelIndex() ) const override { return mRows; }
	size_t columnCount( const ModelIndex& = ModelIndex() ) const override { return 3; }
	std::string columnName( const size_t& column ) const override {
		return String::format( "Column %zu", column );
	}
	ModelIndex index( int row, int column,
					  const ModelIndex& parent = ModelIndex() ) const override {
		return row >= 0 && static_cast<size_t>( row ) < mRows && column >= 0 && column < 3 &&
					   !parent.isValid()
				   ? createIndex( row, column )
				   : ModelIndex{};
	}
	Variant data( const ModelIndex&, ModelRole = ModelRole::Display ) const override {
		return "Value";
	}

  protected:
	size_t mRows;
};

class SortableRowsModel : public ThreeColumnModel {
  public:
	SortableRowsModel() : ThreeColumnModel( 4 ) {}

	Variant data( const ModelIndex& index, ModelRole = ModelRole::Display ) const override {
		static constexpr int values[] = { 30, 10, 40, 20 };
		return values[index.row()];
	}
};

class PercentageTestTable : public UITableView {
  public:
	static PercentageTestTable* New() { return eeNew( PercentageTestTable, () ); }

	void userResizeColumn( size_t column, Float width ) {
		columnData( column ).setWidth( width, true );
		onColumnSizeChange( column, true );
	}

	void setColumnMinimumWidth( size_t column, Float width ) {
		columnData( column ).minWidth = width;
		createOrUpdateColumns( false );
	}

	void resizeColumnToContent( size_t column, Float width ) {
		mContentWidth = width;
		onColumnResizeToContent( column );
	}

	void updateScrollbars() { onContentSizeChange(); }

	void dragColumnTo( size_t column, Float centerX ) { reorderColumnAt( column, centerX ); }

	void refreshColumns() { createOrUpdateColumns( true ); }

	void layoutHeaders() { mHeader->updateLayout(); }

	UIPushButton* header( size_t column ) { return columnData( column ).widget; }

	Float getMaxColumnContentWidth( const size_t&, bool ) override { return mContentWidth; }

  protected:
	Float mContentWidth{ 0 };
};

UTEST( UISplitter, HideSplitterOnEdge ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 800, 600 );
	UIWidget::New()->setParent( splitter );
	UIWidget::New()->setParent( splitter );
	auto* separator = splitter->findByTag( "splitter::separator" );
	ASSERT_TRUE( separator != nullptr );

	splitter->setHideSplitterOnEdge( true );
	splitter->setAlwaysShowSplitter( false );
	splitter->setSplitPartition( StyleSheetLength( "100%" ) );
	splitter->updateLayout();
	EXPECT_FALSE( separator->isVisible() );
	EXPECT_EQ( splitter->getFirstWidget()->getPixelsSize().getWidth(), 800.f );
	EXPECT_EQ( splitter->getLastWidget()->getPixelsSize().getWidth(), 0.f );

	splitter->setSplitPartition( StyleSheetLength( "75%" ) );
	splitter->updateLayout();
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_TRUE( separator->isVisible() );

	splitter->setSplitPartition( StyleSheetLength( "0%" ) );
	splitter->updateLayout();
	EXPECT_FALSE( separator->isVisible() );
	EXPECT_EQ( splitter->getFirstWidget()->getPixelsSize().getWidth(), 0.f );
	EXPECT_EQ( splitter->getLastWidget()->getPixelsSize().getWidth(), 800.f );

	splitter->setAlwaysShowSplitter( true );
	splitter->updateLayout();
	EXPECT_TRUE( separator->isVisible() );
}

UTEST( UISplitter, ControlsMatchParentChildWidth ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 800, 600 );
	auto* first = UIWidget::New();
	first->setParent( splitter );
	auto* last = UIWidget::New();
	last->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	last->setParent( splitter );
	splitter->setSplitPartition( StyleSheetLength( "75%" ) );
	splitter->updateLayout();

	EXPECT_TRUE( first->getPixelsSize().getWidth() > 590.f &&
				 first->getPixelsSize().getWidth() < 600.f );
	EXPECT_TRUE( last->getPixelsSize().getWidth() > 190.f &&
				 last->getPixelsSize().getWidth() < 200.f );
}

UTEST( UIAbstractTableView, PercentageColumnWidthsScaleAndPreserveAdjacentTotal ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnWidth( 0, 100 );
	table->setColumnWidth( 1, 200 );
	table->setColumnWidth( 2, 100 );
	table->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Percentage );

	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 25.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 50.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 25.f ) < 0.01f );

	table->setPixelsSize( 800, 200 );
	Float contentWidth = table->getContentSpaceWidth();
	EXPECT_TRUE( std::abs( table->getColumnWidth( 0 ) - contentWidth * 0.25f ) < 1.f );
	EXPECT_TRUE( std::abs( table->getColumnWidth( 1 ) - contentWidth * 0.5f ) < 1.f );
	EXPECT_TRUE( std::abs( table->getColumnWidth( 2 ) - contentWidth * 0.25f ) < 1.f );

	table->userResizeColumn( 0, contentWidth * 0.4f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 40.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 35.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 25.f ) < 0.01f );

	table->setColumnWidthPercentage( 0, 70.f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 70.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 5.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 25.f ) < 0.01f );
	EXPECT_TRUE( table->getColumnWidth( 0 ) >= 0 && table->getColumnWidth( 1 ) >= 0 &&
				 table->getColumnWidth( 2 ) >= 0 );
}

UTEST( UIAbstractTableView, PercentageColumnWidthsPreserveMinimumWidths ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 150, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	table->setColumnMinimumWidth( 0, 80.f );
	table->setColumnMinimumWidth( 1, 80.f );
	table->setColumnMinimumWidth( 2, 80.f );
	app.getUI()->update( Milliseconds( 16 ) );

	EXPECT_TRUE( table->getColumnWidth( 0 ) >= 80.f );
	EXPECT_TRUE( table->getColumnWidth( 1 ) >= 80.f );
	EXPECT_TRUE( table->getColumnWidth( 2 ) >= 80.f );
	EXPECT_TRUE( table->getColumnWidth( 0 ) + table->getColumnWidth( 1 ) +
					 table->getColumnWidth( 2 ) >
				 table->getContentSpaceWidth() );
	EXPECT_TRUE( table->getHorizontalScrollBar()->isVisible() );
}

UTEST( UIAbstractTableView, PercentageColumnWidthsAcceptPartialAndSurplusValues ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );

	table->setColumnsWidthPercentage( { 20.f, 30.f } );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 20.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 30.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 50.f ) < 0.01f );

	table->setColumnsWidthPercentage( { 10.f, 20.f, 70.f, 500.f } );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 10.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 20.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 70.f ) < 0.01f );

	EXPECT_TRUE( table->unserializeColumnWidths( nlohmann::json{ 25.f, 25.f } ) );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 25.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 25.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 50.f ) < 0.01f );
}

UTEST( UIAbstractTableView, PercentageColumnResizeToContentPreservesAdjacentTotal ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	const Float contentWidth = table->getContentSpaceWidth();

	table->resizeColumnToContent( 0, contentWidth * 0.4f );

	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 40.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 35.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 25.f ) < 0.01f );

	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	table->setColumnMinimumWidth( 1, contentWidth * 0.3f );
	table->resizeColumnToContent( 0, contentWidth * 0.6f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 45.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 30.f ) < 0.01f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 25.f ) < 0.01f );
}

UTEST( UIAbstractTableView, ColumnWidthModeAndMenuCanBeConfiguredFromXML ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* defaultTable =
		app.getUI()->loadLayoutFromString( R"xml(<TableView />)xml" )->asType<UITableView>();
	EXPECT_FALSE( defaultTable->isColumnWidthModeMenuEnabled() );
	auto* table = app.getUI()
					  ->loadLayoutFromString( R"xml(<TableView column-width-mode="percentage"
				column-width-mode-menu="true" />)xml" )
					  ->asType<UITableView>();
	EXPECT_TRUE( table->getColumnWidthMode() == UIAbstractTableView::ColumnWidthMode::Percentage );
	EXPECT_TRUE( table->isColumnWidthModeMenuEnabled() );
	auto* cssRoot = app.getUI()->loadLayoutFromString( R"xml(
		<vbox>
			<style>#css_table { column-width-mode-menu: true; }</style>
			<TableView id="css_table" />
		</vbox>)xml" );
	auto* cssTable = cssRoot->querySelector( "#css_table" )->asType<UITableView>();
	EXPECT_TRUE( cssTable->isColumnWidthModeMenuEnabled() );
}

UTEST( UIAbstractTableView, ColumnOrderKeepsModelIndexesAndCellsAligned ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto model = std::make_shared<ThreeColumnModel>();
	auto* table = UITableView::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 200 );
	table->setModel( model );
	table->setColumnsWidth( 100 );

	EXPECT_FALSE( table->isColumnReorderingEnabled() );
	table->setColumnReorderingEnabled( true );
	EXPECT_TRUE( table->isColumnReorderingEnabled() );
	EXPECT_FALSE( table->setColumnOrder( { 0, 0, 2 } ) );
	EXPECT_FALSE( table->setColumnOrder( { 0, 1 } ) );
	EXPECT_TRUE( table->setColumnOrder( { 2, 0, 1 } ) );
	EXPECT_TRUE( table->getColumnOrder() == std::vector<size_t>( { 2, 0, 1 } ) );
	EXPECT_TRUE( table->getColumnPosition( 2 ).x < table->getColumnPosition( 0 ).x );
	EXPECT_TRUE( table->getColumnPosition( 0 ).x < table->getColumnPosition( 1 ).x );

	table->drawChildren();
	for ( size_t column = 0; column < model->columnCount(); ++column ) {
		auto* cell = table->getCellFromIndex( model->index( 0, column ) );
		ASSERT_TRUE( cell != nullptr );
		EXPECT_EQ( cell->getPixelsPosition().x, table->getColumnPosition( column ).x );
	}

	EXPECT_TRUE( table->moveColumn( 1, 0 ) );
	EXPECT_TRUE( table->getColumnOrder() == std::vector<size_t>( { 1, 2, 0 } ) );
	table->setColumnHidden( 2, true );
	table->setColumnHidden( 2, false );
	EXPECT_TRUE( table->getColumnPosition( 1 ).x < table->getColumnPosition( 2 ).x );
	EXPECT_TRUE( table->getColumnPosition( 2 ).x < table->getColumnPosition( 0 ).x );
}

UTEST( UIAbstractTableView, DragOrderAndResizeUseVisualNeighbors ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 800, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	EXPECT_TRUE( table->setColumnOrder( { 2, 0, 1 } ) );

	const Float contentWidth = table->getContentSpaceWidth();
	table->userResizeColumn( 2, contentWidth * 0.35f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 2 ) - 35.f ) < 0.5f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 15.f ) < 0.5f );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 1 ) - 50.f ) < 0.5f );

	const Float beyondLastColumn = table->getColumnPosition( 1 ).x + table->getColumnWidth( 1 );
	table->dragColumnTo( 2, beyondLastColumn );
	EXPECT_TRUE( table->getColumnOrder() == std::vector<size_t>( { 0, 1, 2 } ) );
}

UTEST( UIAbstractTableView, RefreshKeepsDraggedHeaderPosition ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI()->getRoot() );
	table->setPixelsSize( 500, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnsWidth( 100 );
	table->setColumnReorderingEnabled( true );
	app.getUI()->flushDirtyStyleAndLayout();
	app.getUI()->update( Milliseconds( 16 ) );
	auto* dragged = table->header( 0 );
	dragged->setDragging( true );
	dragged->setPixelsPosition( 47.f, dragged->getPixelsPosition().y );
	table->refreshColumns();
	EXPECT_EQ( dragged->getPixelsPosition().x, 47.f );
	table->layoutHeaders();
	EXPECT_EQ( dragged->getPixelsPosition().x, 47.f );
	dragged->setDragging( false, false );
}

UTEST( UIAbstractTableView, RightClickPreservesMultipleSelectedRows ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto model = std::make_shared<ThreeColumnModel>( 3 );
	auto* table = UITableView::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 500, 200 );
	table->setModel( model );
	table->setSelectionKind( UIAbstractView::SelectionKind::Multiple );
	table->drawChildren();
	table->selectAll();
	EXPECT_EQ( table->getSelection().size(), 3 );
	EXPECT_TRUE( table->getSelection().containsRow( 0 ) );
	EXPECT_TRUE( table->getSelection().containsRow( 1 ) );
	EXPECT_TRUE( table->getSelection().containsRow( 2 ) );

	const ModelIndex first = model->index( 0, 0 );
	const ModelIndex second = model->index( 1, 0 );
	const ModelIndex third = model->index( 2, 0 );
	table->getSelection().set( { first, second } );
	EXPECT_EQ( table->getSelection().size(), 2 );
	Input* input = app.getWindow()->getInput();
	InputEvent keyDown{};
	keyDown.Type = InputEvent::KeyDown;
	keyDown.WinID = app.getWindow()->getWindowID();
	keyDown.key.keysym.mod = KeyMod::getDefaultModifier();
	input->processEventForWindow( &keyDown );
	InputEvent mouseDown{};
	mouseDown.Type = InputEvent::MouseButtonDown;
	mouseDown.WinID = app.getWindow()->getWindowID();
	mouseDown.button.button = EE_BUTTON_RIGHT;
	input->processEventForWindow( &mouseDown );

	auto* secondRow = table->getCellFromIndex( second )->getParent();
	EXPECT_EQ( secondRow->asType<UITableRow>()->getCurIndex().row(), 1 );
	secondRow->sendMouseEvent( Event::MouseDown, { 0, 0 }, EE_BUTTON_RMASK );
	EXPECT_EQ( table->getSelection().size(), 2 );
	EXPECT_TRUE( table->getSelection().contains( first ) );
	EXPECT_TRUE( table->getSelection().contains( second ) );

	auto* thirdRow = table->getCellFromIndex( third )->getParent();
	EXPECT_EQ( thirdRow->asType<UITableRow>()->getCurIndex().row(), 2 );
	thirdRow->sendMouseEvent( Event::MouseDown, { 0, 0 }, EE_BUTTON_RMASK );
	EXPECT_EQ( table->getSelection().size(), 1 );
	EXPECT_TRUE( table->getSelection().containsRow( third.row() ) );
}

UTEST( UIAbstractTableView, SortPreservesMultipleSelectedSourceRows ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto source = std::make_shared<SortableRowsModel>();
	auto proxy = SortingProxyModel::New( source );
	auto* table = UITableView::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 500, 200 );
	table->setModel( proxy );
	table->setSelectionKind( UIAbstractView::SelectionKind::Multiple );
	table->getSelection().set( { proxy->index( 0, 2 ), proxy->index( 2, 1 ) } );
	auto* otherTable = UITableView::New();
	otherTable->setParent( app.getUI() );
	otherTable->setPixelsSize( 500, 200 );
	otherTable->setModel( proxy );
	otherTable->getSelection().set( proxy->index( 1, 0 ) );

	auto selectedSourceIndexes = [&]() {
		std::vector<std::pair<int, int>> indexes;
		for ( const auto& index : table->getSelection().indexes() )
			indexes.emplace_back( proxy->mapToSource( index ).row(), index.column() );
		std::sort( indexes.begin(), indexes.end() );
		return indexes;
	};
	const std::vector<std::pair<int, int>> expected{ { 0, 2 }, { 2, 1 } };
	EXPECT_TRUE( selectedSourceIndexes() == expected );
	table->sortByColumn( 0, SortOrder::Ascending );
	EXPECT_TRUE( selectedSourceIndexes() == expected );
	EXPECT_EQ( proxy->mapToSource( otherTable->getSelection().first() ).row(), 1 );
	table->sortByColumn( 0, SortOrder::Descending );
	EXPECT_TRUE( selectedSourceIndexes() == expected );
	EXPECT_EQ( proxy->mapToSource( otherTable->getSelection().first() ).row(), 1 );
}

UTEST( UIAbstractTableView, PercentageRoundingDoesNotCreatePhantomHorizontalScroll ) {
	Float previousDensity = PixelDensity::getPixelDensity();
	PixelDensity::setPixelDensity( 1.5f );
	UIApplication app(
		WindowSettings( 828, 300, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 828, 300 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnsWidthPercentage( { 100.f / 3.f, 100.f / 3.f, 100.f / 3.f } );
	app.getUI()->update( Milliseconds( 16 ) );

	EXPECT_EQ( table->getColumnWidth( 0 ) + table->getColumnWidth( 1 ) + table->getColumnWidth( 2 ),
			   table->getContentSpaceWidth() );
	EXPECT_FALSE( table->getHorizontalScrollBar()->isVisible() );

	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	table->setColumnMinimumWidth( 2, 208.f );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_EQ( table->getColumnWidth( 0 ) + table->getColumnWidth( 1 ) + table->getColumnWidth( 2 ),
			   table->getContentSpaceWidth() );
	EXPECT_TRUE( table->getColumnWidth( 2 ) >= 208.f );
	EXPECT_FALSE( table->getHorizontalScrollBar()->isVisible() );
	PixelDensity::setPixelDensity( previousDensity );
}

UTEST( UIAbstractTableView, PercentageColumnsRecomputeWhenVerticalScrollbarAppears ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 100 );
	table->setModel( std::make_shared<ThreeColumnModel>( 100 ) );
	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	app.getUI()->update( Milliseconds( 16 ) );

	EXPECT_TRUE( table->getVerticalScrollBar()->isVisible() );
	EXPECT_EQ( table->getColumnWidth( 0 ) + table->getColumnWidth( 1 ) + table->getColumnWidth( 2 ),
			   table->getContentSpaceWidth() );
	EXPECT_FALSE( table->getHorizontalScrollBar()->isVisible() );
}

UTEST( UIAbstractTableView, AutoPixelColumnsRecomputeWhenVerticalScrollbarAppears ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 100 );
	table->setModel( std::make_shared<ThreeColumnModel>( 100 ) );
	table->setAutoColumnsWidth( true );
	app.getUI()->update( Milliseconds( 16 ) );

	EXPECT_TRUE( table->getColumnWidthMode() == UIAbstractTableView::ColumnWidthMode::Pixels );
	EXPECT_TRUE( table->getVerticalScrollBar()->isVisible() );
	EXPECT_FALSE( table->getHorizontalScrollBar()->isVisible() );
}

UTEST( UIAbstractTableView, AutoExpandedSingleColumnAccountsForVerticalScrollbar ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 100 );
	table->setAutoExpandOnSingleColumn( true );
	table->setModel( std::make_shared<ThreeColumnModel>( 100 ) );
	table->setColumnsVisible( { 0 } );
	app.getUI()->update( Milliseconds( 16 ) );

	EXPECT_TRUE( table->getVerticalScrollBar()->isVisible() );
	EXPECT_EQ( table->getColumnWidth( 0 ), table->getContentSpaceWidth() );
	EXPECT_FALSE( table->getHorizontalScrollBar()->isVisible() );

	auto model = std::make_shared<ThreeColumnModel>();
	table->setModel( model );
	model->setRows( 100 );
	table->getVerticalScrollBar()->setVisible( true );
	table->updateScrollbars();
	EXPECT_TRUE( table->getVerticalScrollBar()->isVisible() );
	EXPECT_EQ( table->getColumnWidth( 0 ), table->getContentSpaceWidth() );
	EXPECT_FALSE( table->getHorizontalScrollBar()->isVisible() );
}

UTEST( UIAbstractTableView, ColumnWidthsSerializationRoundTripsBothModes ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* table = PercentageTestTable::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 400, 200 );
	table->setModel( std::make_shared<ThreeColumnModel>() );
	table->setColumnsWidthPercentage( { 20.f, 30.f, 50.f } );
	auto percentageWidths = table->serializeColumnWidths();
	table->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Pixels );
	EXPECT_TRUE( table->unserializeColumnWidths( percentageWidths ) );
	EXPECT_TRUE( table->getColumnWidthMode() == UIAbstractTableView::ColumnWidthMode::Percentage );
	EXPECT_TRUE( std::abs( table->getColumnWidthPercentage( 0 ) - 20.f ) < 0.01f );

	table->setColumnWidthMode( UIAbstractTableView::ColumnWidthMode::Pixels );
	table->setColumnWidth( 0, PixelDensity::dpToPx( 80.f ) );
	table->setColumnWidth( 1, PixelDensity::dpToPx( 120.f ) );
	table->setColumnWidth( 2, PixelDensity::dpToPx( 160.f ) );
	auto pixelWidths = table->serializeColumnWidths();
	table->setColumnsWidthPercentage( { 25.f, 50.f, 25.f } );
	EXPECT_TRUE( table->unserializeColumnWidths( pixelWidths ) );
	EXPECT_TRUE( table->getColumnWidthMode() == UIAbstractTableView::ColumnWidthMode::Pixels );
	EXPECT_EQ( table->getColumnWidth( 0 ), PixelDensity::dpToPx( 80.f ) );
	EXPECT_EQ( table->getColumnWidth( 1 ), PixelDensity::dpToPx( 120.f ) );
	EXPECT_EQ( table->getColumnWidth( 2 ), PixelDensity::dpToPx( 160.f ) );

	EXPECT_TRUE( table->unserializeColumnWidths( nlohmann::json{ 25.f, 50.f, 25.f } ) );
	EXPECT_TRUE( table->getColumnWidthMode() == UIAbstractTableView::ColumnWidthMode::Percentage );

	auto* deferredTable = PercentageTestTable::New();
	deferredTable->setParent( app.getUI() );
	deferredTable->setPixelsSize( 400, 200 );
	EXPECT_TRUE( deferredTable->unserializeColumnWidths( percentageWidths ) );
	EXPECT_TRUE( deferredTable->serializeColumnWidths() == percentageWidths );
	deferredTable->setModel( std::make_shared<ThreeColumnModel>() );
	EXPECT_TRUE( deferredTable->getColumnWidthMode() ==
				 UIAbstractTableView::ColumnWidthMode::Percentage );
	EXPECT_TRUE( std::abs( deferredTable->getColumnWidthPercentage( 0 ) - 20.f ) < 0.01f );
}

UTEST( UITabWidget, TransferTabPreservesDestinationTabsAndOwnedWidgets ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* source = TransferTestTabWidget::New();
	source->setParent( app.getUI() );
	auto* destination = TransferTestTabWidget::New();
	destination->setParent( app.getUI() );
	destination->setPixelsSize( 240, 180 );
	auto* sourceWidget = UIWidget::New();
	auto* destinationWidgetA = UIWidget::New();
	auto* destinationWidgetB = UIWidget::New();
	auto* movedTab = source->add( "Moved", sourceWidget );
	destination->add( "Existing A", destinationWidgetA );
	destination->add( "Existing B", destinationWidgetB );

	source->detachTab( movedTab );
	destination->add( movedTab );
	destination->setTabSelected( movedTab );

	EXPECT_EQ( source->getTabCount(), Uint32{ 0 } );
	EXPECT_EQ( destination->getTabCount(), Uint32{ 3 } );
	EXPECT_TRUE( destination->getTab( 0 )->getOwnedWidget() == destinationWidgetA );
	EXPECT_TRUE( destination->getTab( 1 )->getOwnedWidget() == destinationWidgetB );
	EXPECT_TRUE( destination->getTab( 2 )->getOwnedWidget() == sourceWidget );
	EXPECT_TRUE( destination->isParentOf( sourceWidget ) );
	EXPECT_TRUE( destination->isParentOf( destinationWidgetA ) );
	EXPECT_TRUE( destination->isParentOf( destinationWidgetB ) );
	EXPECT_TRUE( sourceWidget->getPixelsSize() == destination->getContainerPixelsSize() );
	EXPECT_TRUE( sourceWidget->isVisible() );
	EXPECT_FALSE( destinationWidgetA->isVisible() );
	EXPECT_FALSE( destinationWidgetB->isVisible() );
}

UTEST( UITabWidgetSplitter, ClosingSplitNotifiesBeforeTabWidgetDestruction ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	auto* container = UILayout::New();
	container->setParent( app.getUI() );
	auto* firstTabWidget = splitter->createTabWidget( container );
	auto* firstWidget = UIWidget::New();
	splitter->createWidgetInTabWidget( firstTabWidget, firstWidget, "First" );
	auto* secondTabWidget = splitter->splitTabWidget( SplitDirection::Right, firstTabWidget );
	auto* secondWidget = UIWidget::New();
	splitter->createWidgetInTabWidget( secondTabWidget, secondWidget, "Second" );
	UITabWidget* closedTabWidget = nullptr;
	splitter->setOnTabWidgetCloseCb(
		[&closedTabWidget]( UITabWidget* tabWidget ) { closedTabWidget = tabWidget; } );

	splitter->closeTab( secondWidget, UITabWidget::FocusTabBehavior::Default );
	app.getUI()->update( Milliseconds( 16 ) );

	EXPECT_EQ( closedTabWidget, secondTabWidget );
	EXPECT_EQ( splitter->getTabWidgets().size(), 1UL );
	eeDelete( splitter );
}

UTEST( UITabWidgetSplitter, Serialization ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	// --- Step 1: Serialize single tab widget with a registered type ---
	UTEST_PRINT_STEP( "Serialize single tab widget with registered type" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		auto* tabWidget = splitter->createTabWidget( (Node*)app.getUI() );

		bool saved = false;
		splitter->registerWidgetType( "testwidget",
									  { [&]( UIWidget* ) {
										   saved = true;
										   return nlohmann::json{ { "custom_key", 42 } };
									   },
										[]( const nlohmann::json& ) -> WidgetLoadResult {
											return { nullptr, nullptr, "" };
										} } );

		auto* w = UIWidget::New();
		w->addClass( "testwidget" );
		splitter->createWidgetInTabWidget( tabWidget, w, "Test Tab" );

		nlohmann::json j = splitter->toJSON();

		EXPECT_TRUE( j["type"].is_string() );
		EXPECT_TRUE( j["type"] == "tabwidget" );
		EXPECT_EQ( j["files"].size(), 1UL );
		EXPECT_EQ( j["current_page"].get<int>(), 0 );
		EXPECT_TRUE( saved );

		std::string fileType = j["files"][0]["type"];
		EXPECT_TRUE( fileType == "testwidget" );
		EXPECT_EQ( j["files"][0]["custom_key"].get<int>(), 42 );
		std::string fileTitle = j["files"][0]["title"];
		EXPECT_TRUE( fileTitle == "Test Tab" );

		eeDelete( splitter );
	}

	// --- Step 2: Empty tab widget serializes empty ---
	UTEST_PRINT_STEP( "Empty tab widget serializes empty" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		splitter->createTabWidget( (Node*)app.getUI() );

		nlohmann::json j = splitter->toJSON();

		EXPECT_TRUE( j["type"].is_string() );
		EXPECT_TRUE( j["type"] == "tabwidget" );
		EXPECT_EQ( j["files"].size(), 0UL );

		eeDelete( splitter );
	}

	// --- Step 3: Unregistered widget type is skipped ---
	UTEST_PRINT_STEP( "Unregistered widget type is skipped" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		auto* tabWidget = splitter->createTabWidget( (Node*)app.getUI() );

		auto* ghost = UIWidget::New();
		ghost->addClass( "unknown_type" );
		splitter->createWidgetInTabWidget( tabWidget, ghost, "Ghost" );

		splitter->registerWidgetType(
			"known", { []( UIWidget* ) { return nlohmann::json{ { "val", 1 } }; },
					   []( const nlohmann::json& ) -> WidgetLoadResult { return {}; } } );

		auto* reg = UIWidget::New();
		reg->addClass( "known" );
		splitter->createWidgetInTabWidget( tabWidget, reg, "Known" );

		nlohmann::json j = splitter->toJSON();
		EXPECT_EQ( j["files"].size(), 1UL );
		std::string regType = j["files"][0]["type"];
		EXPECT_TRUE( regType == "known" );

		eeDelete( splitter );
	}

	// --- Step 4: Serialize splitter tree ---
	UTEST_PRINT_STEP( "Serialize splitter tree" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		auto* tabWidget = splitter->createTabWidget( (Node*)app.getUI() );

		splitter->registerWidgetType(
			"panewidget", { []( UIWidget* ) { return nlohmann::json{ { "pane", true } }; },
							[]( const nlohmann::json& j ) -> WidgetLoadResult {
								auto* w = UIWidget::New();
								w->addClass( "panewidget" );
								return { w, nullptr, j.value( "title", "" ) };
							} } );

		auto* w1 = UIWidget::New();
		w1->addClass( "panewidget" );
		splitter->createWidgetInTabWidget( tabWidget, w1, "First" );

		splitter->splitTabWidget( SplitDirection::Right, tabWidget );
		auto* secondTab = splitter->getTabWidgets()[1];

		auto* w2 = UIWidget::New();
		w2->addClass( "panewidget" );
		splitter->createWidgetInTabWidget( secondTab, w2, "Second" );

		nlohmann::json j = splitter->toJSON();

		EXPECT_TRUE( j["type"].is_string() );
		EXPECT_TRUE( j["type"] == "splitter" );
		EXPECT_TRUE( j.contains( "first" ) );
		EXPECT_TRUE( j.contains( "last" ) );
		EXPECT_TRUE( j.contains( "orientation" ) );
		EXPECT_TRUE( j.contains( "split" ) );

		std::string firstType = j["first"]["type"];
		EXPECT_TRUE( firstType == "tabwidget" );
		std::string lastType = j["last"]["type"];
		EXPECT_TRUE( lastType == "tabwidget" );
		EXPECT_EQ( j["first"]["files"].size(), 1UL );
		EXPECT_EQ( j["last"]["files"].size(), 1UL );

		eeDelete( splitter );
	}

	// --- Step 5: Round-trip serialize/deserialize single tab widget ---
	UTEST_PRINT_STEP( "Round-trip serialize/deserialize single tab widget" );
	{
		TestClient client1;
		auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
		auto* tab1 = splitter1->createTabWidget( (Node*)app.getUI() );

		splitter1->registerWidgetType(
			"mywidget", { []( UIWidget* ) { return nlohmann::json{ { "data", "hello" } }; },
						  []( const nlohmann::json& j ) -> WidgetLoadResult {
							  auto* w = UIWidget::New();
							  w->addClass( "mywidget" );
							  return { w, nullptr, j.value( "title", "" ) };
						  } } );

		auto* wa = UIWidget::New();
		wa->addClass( "mywidget" );
		splitter1->createWidgetInTabWidget( tab1, wa, "Tab A" );

		auto* wb = UIWidget::New();
		wb->addClass( "mywidget" );
		splitter1->createWidgetInTabWidget( tab1, wb, "Tab B" );

		tab1->setTabSelected( 1 );
		nlohmann::json saved = splitter1->toJSON();

		TestClient client2;
		auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
		auto* tab2 = splitter2->createTabWidget( (Node*)app.getUI() );

		splitter2->registerWidgetType( "mywidget",
									   { []( UIWidget* ) { return nlohmann::json::object(); },
										 []( const nlohmann::json& j ) -> WidgetLoadResult {
											 auto* w = UIWidget::New();
											 w->addClass( "mywidget" );
											 return { w, nullptr, j.value( "title", "" ) };
										 } } );

		splitter2->fromJSON( saved );

		EXPECT_EQ( splitter2->getTabWidgets().size(), 1UL );
		EXPECT_EQ( tab2->getTabCount(), (Uint32)2 );
		EXPECT_EQ( client2.tabCreatedCount, 2 );
		EXPECT_EQ( tab2->getTabSelectedIndex(), (Uint32)1 );

		eeDelete( splitter1 );
		eeDelete( splitter2 );
	}

	// --- Step 6: Round-trip with splitter ---
	UTEST_PRINT_STEP( "Round-trip with splitter" );
	{
		TestClient client1;
		auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
		auto* tab1 = splitter1->createTabWidget( (Node*)app.getUI() );

		auto onSave = []( UIWidget* ) { return nlohmann::json::object(); };
		auto onLoad = []( const nlohmann::json& j ) -> WidgetLoadResult {
			auto* w = UIWidget::New();
			w->addClass( "splitwidget" );
			return { w, nullptr, j.value( "title", "" ) };
		};
		splitter1->registerWidgetType( "splitwidget", { onSave, onLoad } );

		auto* w1 = UIWidget::New();
		w1->addClass( "splitwidget" );
		splitter1->createWidgetInTabWidget( tab1, w1, "Pane 1" );

		splitter1->splitTabWidget( SplitDirection::Bottom, tab1 );
		auto* secondTab = splitter1->getTabWidgets()[1];

		auto* w2 = UIWidget::New();
		w2->addClass( "splitwidget" );
		splitter1->createWidgetInTabWidget( secondTab, w2, "Pane 2" );

		nlohmann::json saved = splitter1->toJSON();

		TestClient client2;
		auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
		splitter2->createTabWidget( (Node*)app.getUI() );
		splitter2->registerWidgetType( "splitwidget", { onSave, onLoad } );

		splitter2->fromJSON( saved );

		EXPECT_EQ( splitter2->getTabWidgets().size(), 2UL );
		EXPECT_EQ( splitter2->getTabWidgets()[0]->getTabCount(), (Uint32)1 );
		EXPECT_EQ( splitter2->getTabWidgets()[1]->getTabCount(), (Uint32)1 );

		eeDelete( splitter1 );
		eeDelete( splitter2 );
	}

	// --- Step 7: Preserve split partition ---
	UTEST_PRINT_STEP( "Preserve split partition" );
	{
		TestClient client1;
		auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
		auto* tab1 = splitter1->createTabWidget( (Node*)app.getUI() );

		splitter1->registerWidgetType( "partwidget",
									   { []( UIWidget* ) { return nlohmann::json::object(); },
										 []( const nlohmann::json& j ) -> WidgetLoadResult {
											 auto* w = UIWidget::New();
											 w->addClass( "partwidget" );
											 return { w, nullptr, j.value( "title", "" ) };
										 } } );

		auto* w1 = UIWidget::New();
		w1->addClass( "partwidget" );
		splitter1->createWidgetInTabWidget( tab1, w1, "P1" );

		splitter1->splitTabWidget( SplitDirection::Bottom, tab1 );
		auto* secondTab = splitter1->getTabWidgets()[1];

		auto* w2 = UIWidget::New();
		w2->addClass( "partwidget" );
		splitter1->createWidgetInTabWidget( secondTab, w2, "P2" );

		UISplitter* spl = tab1->getParent()->asType<UISplitter>();
		spl->setSplitPartition( StyleSheetLength( "30%" ) );

		nlohmann::json saved = splitter1->toJSON();
		EXPECT_TRUE( saved.contains( "split" ) );
		EXPECT_TRUE( saved["split"].is_string() );

		TestClient client2;
		auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
		splitter2->createTabWidget( (Node*)app.getUI() );

		splitter2->registerWidgetType( "partwidget",
									   { []( UIWidget* ) { return nlohmann::json::object(); },
										 []( const nlohmann::json& j ) -> WidgetLoadResult {
											 auto* w = UIWidget::New();
											 w->addClass( "partwidget" );
											 return { w, nullptr, j.value( "title", "" ) };
										 } } );

		splitter2->fromJSON( saved );

		EXPECT_EQ( splitter2->getTabWidgets().size(), 2UL );

		eeDelete( splitter1 );
		eeDelete( splitter2 );
	}
}

UTEST( UITabWidgetSplitter, WidgetMovedBetweenSplitsKeepsFocusAndTitleTracking ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	auto* container = UILayout::New();
	container->setParent( app.getUI() );
	auto* firstTabWidget = splitter->createTabWidget( container );
	UIWidget* keepWidget =
		splitter->createWidgetInTabWidget( firstTabWidget, UIWidget::New(), "Keep" ).second;
	auto tabAndWidget =
		splitter->createWidgetInTabWidget( firstTabWidget, UIWidget::New(), "Moved", false );
	UITab* tabW = tabAndWidget.first;
	UIWidget* widgetW = tabAndWidget.second;
	auto* secondTabWidget = splitter->splitTabWidget( SplitDirection::Right, firstTabWidget );
	splitter->createWidgetInTabWidget( secondTabWidget, UIWidget::New(), "Other", false );

	transferTabTo( tabW, secondTabWidget );

	keepWidget->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );

	widgetW->setFocus();
	EXPECT_EQ( splitter->getCurWidget(), widgetW );

	widgetW->sendTextEvent( Event::OnTitleChange, "Renamed" );
	EXPECT_TRUE( tabW->getText() == "Renamed" );

	eeDelete( splitter );
}

UTEST( UITabWidgetSplitter, WidgetMovedOutsideSplitterStopsNotifyingSplitter ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	auto* container = UILayout::New();
	container->setParent( app.getUI() );
	auto* firstTabWidget = splitter->createTabWidget( container );
	UIWidget* keepWidget =
		splitter->createWidgetInTabWidget( firstTabWidget, UIWidget::New(), "Keep" ).second;
	auto tabAndWidget =
		splitter->createWidgetInTabWidget( firstTabWidget, UIWidget::New(), "Moved", false );
	UITab* tabW = tabAndWidget.first;
	UIWidget* widgetW = tabAndWidget.second;
	auto* external = UITabWidget::New();
	external->setParent( app.getUI() );
	external->setAllowDragAndDropTabs( true );

	transferTabTo( tabW, external );

	keepWidget->setFocus();
	int focusChanges = client.focusChangeCount;

	widgetW->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );
	EXPECT_EQ( client.focusChangeCount, focusChanges );

	widgetW->sendTextEvent( Event::OnTitleChange, "Stale" );
	EXPECT_FALSE( tabW->getText() == "Stale" );

	eeDelete( splitter );
}

UTEST( UITabWidgetSplitter, WidgetMovedIntoSplitterStartsNotifyingSplitter ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	auto* container = UILayout::New();
	container->setParent( app.getUI() );
	auto* firstTabWidget = splitter->createTabWidget( container );
	UIWidget* keepWidget =
		splitter->createWidgetInTabWidget( firstTabWidget, UIWidget::New(), "Keep" ).second;
	auto* external = UITabWidget::New();
	external->setParent( app.getUI() );
	external->setAllowDragAndDropTabs( true );
	auto* widgetW = UIWidget::New();
	UITab* tabW = external->add( "External", widgetW );
	widgetW->setData( (UintPtr)tabW );

	transferTabTo( tabW, firstTabWidget );

	keepWidget->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );

	widgetW->setFocus();
	EXPECT_EQ( splitter->getCurWidget(), widgetW );

	widgetW->sendTextEvent( Event::OnTitleChange, "Renamed" );
	EXPECT_TRUE( tabW->getText() == "Renamed" );

	eeDelete( splitter );
}
