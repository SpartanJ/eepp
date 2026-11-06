#include "tabtransfer.hpp"
#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/sortingproxymodel.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>
#include <nlohmann/json.hpp>
#include <optional>

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

static void dragSeparator( UIWidget* separator, const Vector2f& position ) {
	separator->setDragging( true, false );
	separator->setPixelsPosition( position );
	separator->setDragging( false, false );
}

static UIWidget* findSeparator( UIWidget* node ) {
	return node->findByTag( "splitter::separator" )->asType<UIWidget>();
}

UTEST( UISplitter, DragPreservesPartitionUnit ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	for ( const auto orientation : { UIOrientation::Horizontal, UIOrientation::Vertical } ) {
		const bool horizontal = orientation == UIOrientation::Horizontal;
		auto* splitter = UISplitter::New();
		splitter->setParent( app.getUI() );
		splitter->setPixelsSize( 800, 800 );
		splitter->setOrientation( orientation );
		UIWidget::New()->setParent( splitter );
		UIWidget::New()->setParent( splitter );
		auto* separator = findSeparator( splitter );
		int changes = 0;
		splitter->on( Event::OnValueChange, [&changes]( const Event* ) { changes++; } );

		const auto dragAndGetUnit = [&]( const char* partition ) {
			splitter->setSplitPartition( StyleSheetLength( partition ) );
			splitter->updateLayout();
			dragSeparator( separator, horizontal ? Vector2f( 300, 0 ) : Vector2f( 0, 300 ) );
			return splitter->getSplitPartition();
		};

		EXPECT_TRUE( dragAndGetUnit( "50%" ).getUnit() == StyleSheetLength::Percentage );
		EXPECT_NEAR( splitter->getSplitPartition().getValue(), 300.f / 796.f * 100.f, 0.01f );
		EXPECT_TRUE( dragAndGetUnit( "200dp" ).getUnit() == StyleSheetLength::Dp );
		EXPECT_NEAR( splitter->getSplitPartition().getValue(), 300.f, 0.01f );
		EXPECT_TRUE( dragAndGetUnit( "150px" ).getUnit() == StyleSheetLength::Px );
		EXPECT_NEAR( splitter->getSplitPartition().getValue(), 300.f, 0.01f );
		// Units that can't follow a drag materialize as a dp size.
		EXPECT_TRUE( dragAndGetUnit( "10em" ).getUnit() == StyleSheetLength::Dp );
		EXPECT_NEAR( splitter->getSplitPartition().getValue(), 300.f, 0.01f );
		EXPECT_EQ( changes, 4 );

		splitter->setSplitPartition( StyleSheetLength( "120dp" ) );
		splitter->updateLayout();
		EXPECT_EQ( changes, 4 );

		// An absolute partition doesn't scale with the splitter after being dragged.
		dragSeparator( separator, horizontal ? Vector2f( 250, 0 ) : Vector2f( 0, 250 ) );
		splitter->setPixelsSize( 1000, 1000 );
		splitter->updateLayout();
		EXPECT_NEAR( horizontal ? splitter->getFirstWidget()->getPixelsSize().getWidth()
								: splitter->getFirstWidget()->getPixelsSize().getHeight(),
					 250.f, 0.01f );
		splitter->close();
		app.getUI()->update( Milliseconds( 16 ) );
	}
}

UTEST( UISplitter, DragConversionExcludesPadding ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 800, 600 );
	splitter->setPadding( Rectf( 20, 0, 20, 0 ) );
	UIWidget::New()->setParent( splitter );
	UIWidget::New()->setParent( splitter );
	splitter->updateLayout();

	dragSeparator( findSeparator( splitter ), { 400, 0 } );
	EXPECT_NEAR( splitter->getFirstWidget()->getPixelsSize().getWidth(), 380.f, 0.01f );

	// Laying out the stored partition must keep the separator where the user dropped it.
	splitter->setLayoutDirty();
	splitter->updateLayout();
	EXPECT_NEAR( findSeparator( splitter )->getPixelsPosition().x, 400.f, 0.01f );
}

UTEST( UISplitter, EndOriginMeasuresTheTrailingPane ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	for ( const auto orientation : { UIOrientation::Horizontal, UIOrientation::Vertical } ) {
		const bool horizontal = orientation == UIOrientation::Horizontal;
		auto* splitter = UISplitter::New();
		splitter->setParent( app.getUI() );
		splitter->setPixelsSize( 804, 804 );
		splitter->setOrientation( orientation );
		auto* first = UIWidget::New();
		first->setParent( splitter );
		auto* last = UIWidget::New();
		last->setParent( splitter );
		const auto axis = [horizontal]( UIWidget* widget ) {
			return horizontal ? widget->getPixelsSize().getWidth()
							  : widget->getPixelsSize().getHeight();
		};

		splitter->setPartitionOrigin( UISplitter::PartitionOrigin::End );
		splitter->setSplitPartition( StyleSheetLength( "200dp" ) );
		splitter->updateLayout();
		EXPECT_NEAR( axis( last ), 200.f, 0.01f );
		EXPECT_NEAR( axis( first ), 600.f, 0.01f );

		splitter->setPixelsSize( 1004, 1004 );
		splitter->updateLayout();
		EXPECT_NEAR( axis( last ), 200.f, 0.01f );
		EXPECT_NEAR( axis( first ), 800.f, 0.01f );

		dragSeparator( findSeparator( splitter ),
					   horizontal ? Vector2f( 750, 0 ) : Vector2f( 0, 750 ) );
		EXPECT_TRUE( splitter->getSplitPartition().getUnit() == StyleSheetLength::Dp );
		EXPECT_NEAR( splitter->getSplitPartition().getValue(), 250.f, 0.01f );

		splitter->setSplitPartition( StyleSheetLength( "25%" ) );
		splitter->setPixelsSize( 804, 804 );
		splitter->updateLayout();
		EXPECT_NEAR( axis( last ), 200.f, 0.01f );
		dragSeparator( findSeparator( splitter ),
					   horizontal ? Vector2f( 400, 0 ) : Vector2f( 0, 400 ) );
		EXPECT_TRUE( splitter->getSplitPartition().getUnit() == StyleSheetLength::Percentage );
		EXPECT_NEAR( splitter->getSplitPartition().getValue(), 50.f, 0.01f );

		// Swapping keeps the origin: the partition now sizes the pane that took the trailing edge.
		splitter->setSplitPartition( StyleSheetLength( "200dp" ) );
		splitter->swap();
		splitter->updateLayout();
		EXPECT_TRUE( splitter->getLastWidget() == first );
		EXPECT_NEAR( axis( first ), 200.f, 0.01f );
		splitter->close();
		app.getUI()->update( Milliseconds( 16 ) );
	}
}

UTEST( UISplitter, SwapPartitionKeepsAbsolutePaneSizes ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 804, 600 );
	auto* first = UIWidget::New();
	first->setParent( splitter );
	auto* last = UIWidget::New();
	last->setParent( splitter );
	splitter->setSplitPartition( StyleSheetLength( "200dp" ) );
	splitter->updateLayout();

	splitter->swap( true );
	splitter->updateLayout();
	EXPECT_TRUE( splitter->getSplitPartition().getUnit() == StyleSheetLength::Dp );
	EXPECT_NEAR( first->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_NEAR( last->getPixelsSize().getWidth(), 600.f, 0.01f );

	splitter->setSplitPartition( StyleSheetLength( "30%" ) );
	splitter->swap( true );
	EXPECT_TRUE( splitter->getSplitPartition().getUnit() == StyleSheetLength::Percentage );
	EXPECT_NEAR( splitter->getSplitPartition().getValue(), 70.f, 0.01f );
}

UTEST( UISplitter, NonResizableKeepsDivider ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 800, 600 );
	UIWidget::New()->setParent( splitter );
	UIWidget::New()->setParent( splitter );
	splitter->updateLayout();
	auto* separator = findSeparator( splitter );
	EXPECT_TRUE( splitter->isResizable() );
	EXPECT_TRUE( separator->isEnabled() );

	separator->setDragging( true, false );
	splitter->setResizable( false );
	EXPECT_FALSE( separator->isDragging() );
	EXPECT_FALSE( splitter->isSeparatorDragging() );
	EXPECT_TRUE( separator->isVisible() );
	EXPECT_FALSE( separator->isEnabled() );

	// A later layout must not enable the separator again.
	splitter->setSplitPartition( StyleSheetLength( "40%" ) );
	splitter->updateLayout();
	EXPECT_TRUE( separator->isVisible() );
	EXPECT_FALSE( separator->isEnabled() );

	splitter->setResizable( true );
	EXPECT_TRUE( separator->isEnabled() );
	EXPECT_TRUE( separator->isDragEnabled() );
}

UTEST( UISplitter, HideOnEdgeSupportsAbsolutePartitions ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 800, 600 );
	auto* first = UIWidget::New();
	first->setParent( splitter );
	auto* last = UIWidget::New();
	last->setParent( splitter );
	auto* separator = findSeparator( splitter );
	splitter->setHideSplitterOnEdge( true );
	splitter->setAlwaysShowSplitter( false );

	splitter->setSplitPartition( StyleSheetLength( "0dp" ) );
	splitter->updateLayout();
	EXPECT_FALSE( separator->isVisible() );
	EXPECT_EQ( first->getPixelsSize().getWidth(), 0.f );
	EXPECT_EQ( last->getPixelsSize().getWidth(), 800.f );
	EXPECT_EQ( last->getPixelsPosition().x, 0.f );

	splitter->setPartitionOrigin( UISplitter::PartitionOrigin::End );
	splitter->setLayoutDirty();
	splitter->updateLayout();
	EXPECT_FALSE( separator->isVisible() );
	EXPECT_EQ( first->getPixelsSize().getWidth(), 800.f );
	EXPECT_EQ( last->getPixelsSize().getWidth(), 0.f );

	splitter->setSplitPartition( StyleSheetLength( "900dp" ) );
	splitter->updateLayout();
	EXPECT_FALSE( separator->isVisible() );
	EXPECT_EQ( last->getPixelsSize().getWidth(), 800.f );

	// A pane with a minimum can't be squeezed out by a partition that doesn't fit.
	first->setMinWidthEq( "100dp" );
	splitter->setLayoutDirty();
	splitter->updateLayout();
	EXPECT_TRUE( separator->isVisible() );
	EXPECT_NEAR( first->getPixelsSize().getWidth(), 100.f, 0.01f );
	EXPECT_NEAR( last->getPixelsSize().getWidth(), 696.f, 0.01f );
}

UTEST( UISplitter, LayoutHonorsBothMinimumsWithoutNegativeGeometry ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* splitter = UISplitter::New();
	splitter->setParent( app.getUI() );
	splitter->setPixelsSize( 804, 600 );
	auto* first = UIWidget::New();
	first->setParent( splitter );
	auto* last = UIWidget::New();
	last->setParent( splitter );
	first->setMinWidthEq( "100dp" );
	last->setMinWidthEq( "150dp" );

	splitter->setSplitPartition( StyleSheetLength( "95%" ) );
	splitter->updateLayout();
	EXPECT_NEAR( last->getPixelsSize().getWidth(), 150.f, 0.01f );
	EXPECT_NEAR( first->getPixelsSize().getWidth(), 650.f, 0.01f );

	splitter->setSplitPartition( StyleSheetLength( "1%" ) );
	splitter->updateLayout();
	EXPECT_NEAR( first->getPixelsSize().getWidth(), 100.f, 0.01f );

	// When both can't fit the first pane wins, and nothing is ever negative.
	splitter->setPixelsSize( 124, 600 );
	splitter->updateLayout();
	EXPECT_NEAR( first->getPixelsSize().getWidth(), 100.f, 0.01f );
	EXPECT_TRUE( last->getPixelsSize().getWidth() >= 0.f );

	dragSeparator( findSeparator( splitter ), { 500, 0 } );
	EXPECT_TRUE( first->getPixelsSize().getWidth() >= 0.f );
	EXPECT_TRUE( last->getPixelsSize().getWidth() >= 0.f );

	// A collapsed edge partition still hides a pane with a minimum (ecode's closed right panel).
	splitter->setPixelsSize( 804, 600 );
	splitter->setHideSplitterOnEdge( true );
	splitter->setAlwaysShowSplitter( false );
	splitter->setSplitPartition( StyleSheetLength( "100%" ) );
	splitter->updateLayout();
	EXPECT_EQ( first->getPixelsSize().getWidth(), 804.f );
}

class TabPositionTest {
  public:
	static constexpr Float TabHeight = 24.f;
	UIApplication app{
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) };
	TransferTestTabWidget* tabs{ TransferTestTabWidget::New() };
	std::vector<UIWidget*> pages;

	explicit TabPositionTest( size_t tabCount, const Sizef& size = { 800, 600 } ) {
		tabs->setParent( app.getUI() );
		tabs->setPixelsSize( size );
		tabs->setTabsHeight( TabHeight );
		for ( size_t i = 0; i < tabCount; i++ ) {
			pages.push_back( UIWidget::New() );
			tabs->add( String::format( "Tab %zu", i ), pages.back() );
		}
	}

	void tick() { app.getUI()->update( Milliseconds( 16 ) ); }

	size_t countSplitters() {
		size_t count = 0;
		for ( Node* node = tabs->getFirstChild(); node; node = node->getNextNode() )
			count += node->isType( UI_TYPE_SPLITTER ) ? 1 : 0;
		return count;
	}
};

static bool hasNoDraggingNode( TabPositionTest& test ) {
	return test.app.getUI()->getEventDispatcher()->getNodeDragging() == nullptr;
}

UTEST( UITabWidget, PositionsLayOutTabBarAndContainer ) {
	TabPositionTest test( 3 );
	auto* tabs = test.tabs;
	auto* container = tabs->getContainerNode();
	auto* selected = test.pages[0];
	const Float height = TabPositionTest::TabHeight;

	EXPECT_TRUE( tabs->getTabPosition() == UITabWidget::TabPosition::Top );
	EXPECT_TRUE( tabs->hasClass( "tabs-top" ) );
	EXPECT_TRUE( tabs->hasClass( "tabs-horizontal" ) );
	EXPECT_FALSE( tabs->hasClass( "tabs-vertical" ) );
	EXPECT_TRUE( tabs->getTabBar()->getParent() == tabs );
	EXPECT_EQ( test.countSplitters(), size_t{ 0 } );
	EXPECT_TRUE( Vector2f( 0, 0 ) == tabs->getTabBar()->getPixelsPosition() );
	EXPECT_TRUE( Vector2f( 0, height ) == container->getPixelsPosition() );
	EXPECT_TRUE( Sizef( 800, 600 - height ) == container->getPixelsSize() );

	tabs->setTabPosition( UITabWidget::TabPosition::Bottom );
	EXPECT_TRUE( tabs->hasClass( "tabs-bottom" ) );
	EXPECT_FALSE( tabs->hasClass( "tabs-top" ) );
	EXPECT_TRUE( tabs->hasClass( "tabs-horizontal" ) );
	EXPECT_EQ( test.countSplitters(), size_t{ 0 } );
	EXPECT_TRUE( Vector2f( 0, 600 - height ) == tabs->getTabBar()->getPixelsPosition() );
	EXPECT_TRUE( Vector2f( 0, 0 ) == container->getPixelsPosition() );
	EXPECT_TRUE( Sizef( 800, 600 - height ) == selected->getPixelsSize() );

	tabs->setTabPosition( UITabWidget::TabPosition::Left );
	EXPECT_TRUE( tabs->hasClass( "tabs-left" ) );
	EXPECT_TRUE( tabs->hasClass( "tabs-vertical" ) );
	EXPECT_FALSE( tabs->hasClass( "tabs-horizontal" ) );
	EXPECT_TRUE( tabs->isTabBarVertical() );
	EXPECT_EQ( test.countSplitters(), size_t{ 1 } );
	EXPECT_TRUE( tabs->isParentOf( tabs->getTabBar() ) );
	EXPECT_TRUE( tabs->isParentOf( container ) );
	EXPECT_EQ( tabs->getTab( 0 )->getTabWidget(), (UITabWidget*)tabs );
	EXPECT_EQ( UITabWidget::fromOwnedNode( selected ), (UITabWidget*)tabs );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_NEAR( container->convertToWorldSpace( Vector2f::Zero ).x, 204.f, 0.01f );
	EXPECT_NEAR( selected->getPixelsSize().getWidth(), 596.f, 0.01f );
	EXPECT_NEAR( selected->getPixelsSize().getHeight(), 600.f, 0.01f );
	for ( Uint32 i = 0; i < tabs->getTabCount(); i++ ) {
		EXPECT_NEAR( tabs->getTab( i )->getPixelsSize().getWidth(), 200.f, 0.01f );
		EXPECT_NEAR( tabs->getTab( i )->getPixelsSize().getHeight(), height, 0.01f );
		EXPECT_NEAR( tabs->getTab( i )->getPixelsPosition().y, i * height, 0.01f );
		EXPECT_NEAR( tabs->getTab( i )->getPixelsPosition().x, 0.f, 0.01f );
	}

	tabs->setTabPosition( UITabWidget::TabPosition::Right );
	EXPECT_TRUE( tabs->hasClass( "tabs-right" ) );
	EXPECT_EQ( test.countSplitters(), size_t{ 1 } );
	EXPECT_NEAR( container->convertToWorldSpace( Vector2f::Zero ).x, 0.f, 0.01f );
	EXPECT_NEAR( tabs->getTabBar()->convertToWorldSpace( Vector2f::Zero ).x, 600.f, 0.01f );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_NEAR( selected->getPixelsSize().getWidth(), 596.f, 0.01f );

	// The side width doesn't scale with the widget.
	tabs->setPixelsSize( 1000, 600 );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_NEAR( selected->getPixelsSize().getWidth(), 796.f, 0.01f );
}

UTEST( UITabWidget, PositionCyclePreservesTabsAndTopGeometry ) {
	TabPositionTest test( 4 );
	auto* tabs = test.tabs;
	tabs->setTabsClosable( true );
	tabs->setTabSelected( Uint32{ 2 } );
	tabs->setTabSelected( Uint32{ 1 } );
	test.tick();

	std::vector<UITab*> identity;
	std::vector<Rectf> geometry;
	for ( Uint32 i = 0; i < tabs->getTabCount(); i++ ) {
		auto* tab = tabs->getTab( i );
		identity.push_back( tab );
		geometry.emplace_back( tab->getPixelsPosition(), tab->getPixelsSize() );
	}
	const auto history = tabs->getFocusHistory();
	const Sizef containerSize( tabs->getContainerNode()->getPixelsSize() );
	int selections = 0;
	tabs->on( Event::OnTabSelected, [&selections]( const Event* ) { selections++; } );

	using Position = UITabWidget::TabPosition;
	for ( int cycle = 0; cycle < 2; cycle++ ) {
		for ( const auto position : { Position::Left, Position::Right, Position::Bottom,
									  Position::Left, Position::Top } ) {
			tabs->setTabPosition( position );
			test.tick();
			EXPECT_LE( test.countSplitters(), size_t{ 1 } );
			EXPECT_TRUE( tabs->getTabSelected() == identity[1] );
			EXPECT_EQ( tabs->getTabSelectedIndex(), Uint32{ 1 } );
			EXPECT_TRUE( test.pages[1]->isVisible() );
			EXPECT_TRUE( tabs->isParentOf( test.pages[1] ) );
		}
	}

	EXPECT_EQ( selections, 0 );
	EXPECT_TRUE( history == tabs->getFocusHistory() );
	EXPECT_EQ( test.countSplitters(), size_t{ 0 } );
	EXPECT_TRUE( tabs->getTabBar()->getParent() == tabs );
	EXPECT_TRUE( tabs->getContainerNode()->getParent() == tabs );
	EXPECT_TRUE( containerSize == tabs->getContainerNode()->getPixelsSize() );
	EXPECT_TRUE( tabs->getContainerNode()->getMinWidthEq().empty() );
	tabs->setPixelsSize( 40, 600 );
	EXPECT_NEAR( tabs->getContainerNode()->getPixelsSize().getWidth(), 40.f, 0.01f );
	tabs->setPixelsSize( 800, 600 );
	EXPECT_TRUE( tabs->getTabScroll()->getOrientation() == UIOrientation::Horizontal );
	for ( Uint32 i = 0; i < tabs->getTabCount(); i++ ) {
		auto* tab = tabs->getTab( i );
		EXPECT_TRUE( tab == identity[i] );
		EXPECT_FALSE( tab->expandTextView() );
		EXPECT_NEAR( tab->getPixelsPosition().x, geometry[i].Left, 0.01f );
		EXPECT_NEAR( tab->getPixelsSize().getWidth(), geometry[i].getWidth(), 0.01f );
		EXPECT_NEAR( tab->getPixelsSize().getHeight(), geometry[i].getHeight(), 0.01f );
	}
}

UTEST( UITabWidget, PositionCycleRestoresTabTheming ) {
	TabPositionTest test( 3 );
	auto* tabs = test.tabs;
	tabs->setTabsClosable( true );
	tabs->setTabSelected( Uint32{ 1 } );
	test.tick();
	// Past the color transition of the tabs.
	test.app.getUI()->update( Milliseconds( 500 ) );
	test.tick();

	// The properties that the side rows of the theme change.
	static constexpr const char* themed[] = {
		"padding-left",
		"padding-right",
		"padding-top",
		"padding-bottom",
		"border-left-width",
		"border-right-width",
		"border-top-width",
		"border-bottom-width",
		"border-top-left-radius",
		"border-top-right-radius",
		"border-bottom-left-radius",
		"border-bottom-right-radius",
		"background-image",
		"background-size",
		"background-position-x",
		"background-position-y",
		"foreground-image",
		"foreground-size",
		"foreground-position-x",
		"foreground-position-y",
		"color",
		"background-color",
	};
	const auto property = []( UIWidget* widget, const char* name ) {
		return widget->getPropertyString(
			StyleSheetSpecification::instance()->getProperty( name ) );
	};
	const auto snapshot = [&]( UITab* tab ) {
		std::vector<std::string> values;
		for ( const char* name : themed )
			values.push_back( property( tab, name ) );
		values.push_back( property( tab->getCloseButton(), "opacity" ) );
		values.push_back( property( tab->getTextView(), "text-overflow" ) );
		values.push_back( property( tab->getParent()->asType<UIWidget>(), "padding-top" ) );
		values.push_back( property( tab->getParent()->asType<UIWidget>(), "background-image" ) );
		return values;
	};
	const auto selected = snapshot( tabs->getTab( 1 ) );
	// Not the first tab, which is under the mouse once it's back at the top.
	const auto unselected = snapshot( tabs->getTab( 2 ) );
	const Rectf selectedPadding( tabs->getTab( 1 )->getPixelsPadding() );

	using Position = UITabWidget::TabPosition;
	for ( const auto position : { Position::Left, Position::Right, Position::Bottom } ) {
		tabs->setTabPosition( position );
		test.tick();
		test.tick();
		EXPECT_TRUE( position == Position::Bottom || snapshot( tabs->getTab( 2 ) ) != unselected );
		tabs->setTabPosition( Position::Top );
		test.tick();
		// Past the color transition of the tabs.
		test.app.getUI()->update( Milliseconds( 500 ) );
		test.tick();

		const auto selectedAfter = snapshot( tabs->getTab( 1 ) );
		const auto unselectedAfter = snapshot( tabs->getTab( 2 ) );
		for ( size_t i = 0; i < selected.size(); i++ ) {
			EXPECT_STREQ( selected[i].c_str(), selectedAfter[i].c_str() );
			EXPECT_STREQ( unselected[i].c_str(), unselectedAfter[i].c_str() );
		}
		EXPECT_TRUE( selectedPadding == tabs->getTab( 1 )->getPixelsPadding() );
	}
}

UTEST( UITabWidget, PositionChangeKeepsPageFocus ) {
	TabPositionTest test( 2 );
	auto* tabs = test.tabs;
	test.pages[0]->setFocus();
	EXPECT_TRUE( test.pages[0]->hasFocus() );

	for ( const auto position :
		  { UITabWidget::TabPosition::Left, UITabWidget::TabPosition::Right,
			UITabWidget::TabPosition::Top, UITabWidget::TabPosition::Bottom } ) {
		tabs->setTabPosition( position );
		test.tick();
		EXPECT_TRUE( test.pages[0]->hasFocus() );
	}
}

UTEST( UITabWidget, HiddenSideBarReleasesItsSpaceAndKeepsItsWidth ) {
	using Position = UITabWidget::TabPosition;
	for ( const auto position : { Position::Left, Position::Right } ) {
		TabPositionTest test( 1 );
		auto* tabs = test.tabs;
		tabs->setTabPosition( position );
		tabs->setTabBarWidth( 260 );
		auto* separator = findSeparator( tabs );
		tabs->setHideTabBarOnSingleTab( true );
		EXPECT_FALSE( tabs->getTabBar()->isVisible() );
		EXPECT_FALSE( separator->isVisible() );
		EXPECT_FALSE( separator->isEnabled() );
		EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 800.f, 0.01f );
		EXPECT_NEAR( tabs->getContainerNode()->convertToWorldSpace( Vector2f::Zero ).x, 0.f,
					 0.01f );

		for ( int i = 0; i < 2; i++ ) {
			auto* page = UIWidget::New();
			auto* tab = tabs->add( "Second", page );
			EXPECT_TRUE( tabs->getTabBar()->isVisible() );
			EXPECT_TRUE( separator->isVisible() );
			EXPECT_TRUE( separator->isEnabled() );
			EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 260.f, 0.01f );
			EXPECT_NEAR( tabs->getTab( 1 )->getPixelsSize().getWidth(), 260.f, 0.01f );
			EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 536.f, 0.01f );

			tabs->removeTab( tab );
			EXPECT_FALSE( tabs->getTabBar()->isVisible() );
			EXPECT_FALSE( separator->isVisible() );
			EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 800.f, 0.01f );
			EXPECT_NEAR( tabs->getTabBarWidth(), 260.f, 0.01f );
			test.tick();
		}

		tabs->removeAllTabs();
		test.tick();
		EXPECT_FALSE( tabs->getTabBar()->isVisible() );
	}
}

UTEST( UITabWidget, SeparatorDragResizesRowsAndRemembersWidth ) {
	TabPositionTest test( 30 );
	auto* tabs = test.tabs;
	tabs->setTabPosition( UITabWidget::TabPosition::Right );
	auto* separator = findSeparator( tabs );
	test.tick();
	EXPECT_TRUE( separator->isEnabled() );
	int widthChanges = 0;
	tabs->on( Event::OnValueChange, [&widthChanges]( const Event* ) { widthChanges++; } );

	tabs->setTabBarWidth( 210 );
	EXPECT_EQ( widthChanges, 0 );

	dragSeparator( separator, { 500, 0 } );

	EXPECT_EQ( widthChanges, 1 );
	EXPECT_NEAR( tabs->getTabBarWidth(), 296.f, 0.01f );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 296.f, 0.01f );
	EXPECT_NEAR( tabs->getTab( 29 )->getPixelsSize().getWidth(), 296.f, 0.01f );
	EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 500.f, 0.01f );
	EXPECT_NEAR( tabs->getTabScroll()->getPixelsPosition().x,
				 296.f - tabs->getTabScroll()->getPixelsSize().getWidth(), 0.01f );

	// The width survives leaving the vertical positions.
	tabs->setTabPosition( UITabWidget::TabPosition::Top );
	test.tick();
	tabs->setTabPosition( UITabWidget::TabPosition::Left );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 296.f, 0.01f );
}

UTEST( UITabWidget, CollapsingTheSideBarCancelsSeparatorDrag ) {
	using Position = UITabWidget::TabPosition;
	for ( const auto position : { Position::Left, Position::Right } ) {
		TabPositionTest test( 2 );
		auto* tabs = test.tabs;
		tabs->setTabPosition( position );
		auto* separator = findSeparator( tabs );
		test.tick();
		const Vector2f dragPosition( position == Position::Left ? 260 : 500, 0 );

		const auto expectCollapsed = [&]() {
			EXPECT_FALSE( separator->isDragging() );
			EXPECT_TRUE( hasNoDraggingNode( test ) );
			EXPECT_FALSE( separator->isVisible() );
			EXPECT_FALSE( separator->isEnabled() );
			EXPECT_NEAR( tabs->getContainerNode()->convertToWorldSpace( Vector2f::Zero ).x, 0.f,
						 0.01f );
		};

		separator->setDragging( true, false );
		test.app.getUI()->getEventDispatcher()->setNodeDragging( separator );
		separator->setPixelsPosition( dragPosition );
		const Float draggedWidth = tabs->getTabBarWidth();
		EXPECT_NE( draggedWidth, 200.f );

		tabs->setHideTabBar( true );
		expectCollapsed();
		EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 800.f, 0.01f );
		EXPECT_NEAR( tabs->getTabBarWidth(), draggedWidth, 0.01f );

		tabs->setHideTabBar( false );
		EXPECT_TRUE( separator->isVisible() );
		EXPECT_TRUE( separator->isEnabled() );
		EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), draggedWidth, 0.01f );

		separator->setDragging( true, false );
		test.app.getUI()->getEventDispatcher()->setNodeDragging( separator );
		tabs->setPixelsSize( 40, 600 );
		expectCollapsed();
		EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 40.f, 0.01f );

		tabs->setPixelsSize( 800, 600 );
		EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), draggedWidth, 0.01f );
	}
}

UTEST( UITabWidget, NarrowSideTabsKeepUsablePanes ) {
	using Position = UITabWidget::TabPosition;
	for ( const auto position : { Position::Left, Position::Right } ) {
		TabPositionTest test( 2, { 150, 300 } );
		auto* tabs = test.tabs;
		tabs->setTabPosition( position );
		auto* separator = findSeparator( tabs );
		EXPECT_TRUE( separator->isVisible() );
		EXPECT_TRUE( tabs->getTabBar()->getPixelsSize().getWidth() >= 64.f );
		EXPECT_TRUE( test.pages[0]->getPixelsSize().getWidth() >= 64.f );

		// Without room for both panes the content takes the whole widget, instead of being pushed
		// out of it by the minimum sizes.
		tabs->setPixelsSize( 40, 300 );
		EXPECT_FALSE( tabs->getTabBar()->isVisible() );
		EXPECT_FALSE( separator->isVisible() );
		EXPECT_NEAR( tabs->getContainerNode()->convertToWorldSpace( Vector2f::Zero ).x, 0.f,
					 0.01f );
		EXPECT_NEAR( test.pages[0]->getPixelsSize().getWidth(), 40.f, 0.01f );

		tabs->setPixelsSize( 800, 300 );
		EXPECT_TRUE( tabs->getTabBar()->isVisible() );
		EXPECT_TRUE( separator->isVisible() );
		EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getWidth(), 200.f, 0.01f );
		EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getWidth(), 200.f, 0.01f );
		EXPECT_NEAR( tabs->getTabBarWidth(), 200.f, 0.01f );
	}
}

UTEST( UITabWidget, VerticalRowsIgnoreHorizontalWidthLimits ) {
	TabPositionTest test( 2 );
	auto* tabs = test.tabs;
	tabs->setMinTabWidth( "300dp" );
	EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getWidth(), 300.f, 0.01f );

	tabs->setTabPosition( UITabWidget::TabPosition::Left );
	EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_NEAR( tabs->getTab( 1 )->getPixelsSize().getWidth(), 200.f, 0.01f );

	tabs->setMaxTabWidth( "100dp" );
	tabs->setMinTabWidth( "32dp" );
	EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getWidth(), 200.f, 0.01f );

	tabs->setTabPosition( UITabWidget::TabPosition::Top );
	EXPECT_TRUE( tabs->getTab( 0 )->getPixelsSize().getWidth() <= 100.f );

	tabs->setMaxTabWidth( "300dp" );
	tabs->setMinTabWidth( "300dp" );
	EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getWidth(), 300.f, 0.01f );
}

UTEST( UITabWidget, VerticalOverflowScrollsSelectedRowIntoView ) {
	TabPositionTest test( 40, { 800, 240 } );
	auto* tabs = test.tabs;
	tabs->setTabPosition( UITabWidget::TabPosition::Left );
	test.tick();
	// The theme can pad the tab bar, which scrolls with the rows.
	const Float rowsHeight = 40.f * tabs->getTabsHeight() +
							 tabs->getTabBar()->getPixelsPadding().Top +
							 tabs->getTabBar()->getPixelsPadding().Bottom;
	auto* scroll = tabs->getTabScroll();
	EXPECT_TRUE( scroll->getOrientation() == UIOrientation::Vertical );
	EXPECT_TRUE( scroll->isVisible() );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getHeight(), rowsHeight, 0.01f );
	EXPECT_NEAR( scroll->getMaxValue(), rowsHeight - 240.f, 0.01f );
	EXPECT_NEAR( scroll->getPixelsSize().getHeight(), 240.f, 0.01f );

	tabs->setTabSelected( Uint32{ 39 } );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsPosition().y, -( rowsHeight - 240.f ), 0.01f );
	EXPECT_NEAR( scroll->getPixelsPosition().y, rowsHeight - 240.f, 0.01f );

	tabs->setTabSelected( Uint32{ 0 } );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsPosition().y, 0.f, 0.01f );

	tabs->setTabSelected( Uint32{ 20 } );
	Float rowTop = tabs->getTab( 20 )->convertToWorldSpace( Vector2f::Zero ).y;
	EXPECT_TRUE( rowTop >= 0.f && rowTop + tabs->getTabsHeight() <= 240.f );

	// Once everything fits the strip can't stay scrolled.
	tabs->setPixelsSize( 800, 2000 );
	EXPECT_FALSE( scroll->isVisible() );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsPosition().y, 0.f, 0.01f );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getHeight(), 2000.f, 0.01f );

	// Back to horizontal the scrollbar must behave as a strip scrollbar again.
	tabs->setPixelsSize( 200, 240 );
	tabs->setTabPosition( UITabWidget::TabPosition::Top );
	scroll = tabs->getTabScroll();
	EXPECT_TRUE( scroll->getOrientation() == UIOrientation::Horizontal );
	EXPECT_NEAR( scroll->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_TRUE( scroll->isVisible() );
}

UTEST( UITabWidget, SidePositionsAreIndependentOfTextDirection ) {
	using Position = UITabWidget::TabPosition;
	for ( const auto position : { Position::Left, Position::Right } ) {
		TabPositionTest test( 3 );
		auto* tabs = test.tabs;
		tabs->setDirection( UIDirection::RightToLeft );
		tabs->setTabPosition( position );

		const Float barX = tabs->getTabBar()->convertToWorldSpace( Vector2f::Zero ).x;
		EXPECT_NEAR( barX, position == Position::Left ? 0.f : 600.f, 0.01f );
		for ( Uint32 i = 0; i < tabs->getTabCount(); i++ ) {
			EXPECT_NEAR( tabs->getTab( i )->getPixelsPosition().y, i * 24.f, 0.01f );
			EXPECT_NEAR( tabs->getTab( i )->getPixelsPosition().x, 0.f, 0.01f );
		}

		tabs->setDirection( UIDirection::LeftToRight );
		EXPECT_NEAR( tabs->getTabBar()->convertToWorldSpace( Vector2f::Zero ).x, barX, 0.01f );
	}
}

UTEST( UITabWidget, PositionClassStylesFollowThePosition ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	app.getUI()->loadLayoutFromString( R"xml(
		<style>
			TabWidget { tab-height: 24dp; }
			TabWidget.tabs-vertical { tab-height: 30dp; }
		</style>
		<TabWidget id="tabs" layout_width="400dp" layout_height="300dp">
			<Tab text="First" owns="page_a" />
			<widget id="page_a" />
		</TabWidget>
	)xml" );
	app.getUI()->update( Milliseconds( 16 ) );

	auto* tabs = app.getUI()->find( "tabs" )->asType<UITabWidget>();
	EXPECT_NEAR( tabs->getTabsHeight(), 24.f, 0.01f );

	tabs->setTabPosition( UITabWidget::TabPosition::Left );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_NEAR( tabs->getTabsHeight(), 30.f, 0.01f );
	EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getHeight(), 30.f, 0.01f );

	tabs->setTabPosition( UITabWidget::TabPosition::Top );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_NEAR( tabs->getTabsHeight(), 24.f, 0.01f );
	EXPECT_NEAR( tabs->getTabBar()->getPixelsSize().getHeight(), 24.f, 0.01f );
	EXPECT_NEAR( tabs->getContainerNode()->getPixelsPosition().y, 24.f, 0.01f );
	EXPECT_NEAR( tabs->getTab( 0 )->getPixelsSize().getHeight(), 24.f, 0.01f );
}

UTEST( UITabWidget, TabScrollBarFollowsTheOrientation ) {
	TabPositionTest test( 40, { 400, 240 } );
	auto* tabs = test.tabs;
	test.tick();
	EXPECT_TRUE( tabs->getTabScroll()->isVisible() );
	const Float thickness = tabs->getTabScroll()->getPixelsSize().getHeight();
	EXPECT_TRUE( thickness > 0.f && thickness < 40.f );

	for ( int cycle = 0; cycle < 2; cycle++ ) {
		for ( const auto position :
			  { UITabWidget::TabPosition::Left, UITabWidget::TabPosition::Right } ) {
			tabs->setTabPosition( position );
			test.tick();
			auto* scroll = tabs->getTabScroll();
			const Sizef barSize( tabs->getTabBar()->getPixelsSize() );
			EXPECT_TRUE( scroll->isVisible() );
			EXPECT_NEAR( scroll->getPixelsSize().getWidth(), thickness, 0.01f );
			EXPECT_NEAR( scroll->getPixelsSize().getHeight(), 240.f, 0.01f );
			EXPECT_NEAR( scroll->getPixelsPosition().x, barSize.getWidth() - thickness, 0.01f );
			EXPECT_NEAR( scroll->getSlider()->getPixelsSize().getHeight(), 240.f, 0.01f );
			EXPECT_NEAR( scroll->getSlider()->getPixelsSize().getWidth(), thickness, 0.01f );

			tabs->setTabPosition( UITabWidget::TabPosition::Top );
			test.tick();
			scroll = tabs->getTabScroll();
			EXPECT_TRUE( scroll->isVisible() );
			EXPECT_TRUE( scroll->getOrientation() == UIOrientation::Horizontal );
			EXPECT_NEAR( scroll->getPixelsSize().getHeight(), thickness, 0.01f );
			EXPECT_NEAR( scroll->getPixelsSize().getWidth(), 400.f, 0.01f );
			EXPECT_NEAR( scroll->getPixelsPosition().y,
						 tabs->getTabBar()->getPixelsSize().getHeight() - thickness, 0.01f );
			EXPECT_NEAR( scroll->getSlider()->getPixelsSize().getWidth(), 400.f, 0.01f );
			EXPECT_NEAR( scroll->getSlider()->getPixelsSize().getHeight(), thickness, 0.01f );

			// The bar only spans its own edge, so it must not cover the tabs above it.
			const Vector2f tabCenter = tabs->getTab( 1 )->convertToWorldSpace(
				tabs->getTab( 1 )->getPixelsSize().asFloat() * Vector2f( 0.5f, 0.25f ) );
			EXPECT_TRUE( test.app.getUI()->overFind( tabCenter ) == tabs->getTab( 1 ) ||
						 tabs->getTab( 1 )->isParentOf( test.app.getUI()->overFind( tabCenter ) ) );

			// Each replaced scrollbar leaves with the tick.
			test.tick();
			size_t scrollBars = 0;
			for ( Node* node = tabs->getTabBar()->getFirstChild(); node;
				  node = node->getNextNode() ) {
				scrollBars += node->isType( UI_TYPE_SCROLLBAR ) ? 1 : 0;
			}
			EXPECT_EQ( scrollBars, size_t{ 1 } );
		}
	}
}

UTEST( UITabWidget, TabPositionFromStyleSheet ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* root = app.getUI()->loadLayoutFromString( R"xml(
		<style>
			.side { tab-position: right; tab-bar-width: 150dp; }
		</style>
		<TabWidget id="tabs" class="side" layout_width="400dp" layout_height="300dp" tab-height="24dp">
			<Tab id="first" text="First" owns="page_a" />
			<Tab id="second" text="Second" owns="page_b" />
			<widget id="page_a" />
			<widget id="page_b" />
		</TabWidget>
	)xml" );
	ASSERT_TRUE( root != nullptr );
	app.getUI()->update( Milliseconds( 16 ) );

	auto* tabs = app.getUI()->find( "tabs" )->asType<UITabWidget>();
	EXPECT_TRUE( tabs->getTabPosition() == UITabWidget::TabPosition::Right );
	EXPECT_TRUE( tabs->hasClass( "tabs-right" ) );
	EXPECT_TRUE( tabs->hasClass( "side" ) );
	EXPECT_NEAR( tabs->getTabBarWidth(), 150.f, 0.01f );
	EXPECT_EQ( tabs->getTabCount(), Uint32{ 2 } );
	EXPECT_TRUE( tabs->getTab( 1 )->getOwnedWidget() == app.getUI()->find( "page_b" ) );
	EXPECT_TRUE( tabs->getContainerNode()->isParentOf( app.getUI()->find( "page_a" ) ) );

	// Replacing the classes can't lose the one of the current position.
	tabs->setClass( "other" );
	EXPECT_TRUE( tabs->hasClass( "tabs-right" ) );
	EXPECT_TRUE( tabs->hasClass( "tabs-vertical" ) );
	EXPECT_TRUE( tabs->hasClass( "other" ) );
}

class TabDragTest : public TabPositionTest {
  public:
	explicit TabDragTest( size_t tabCount ) : TabPositionTest( tabCount ) {
		tabs->setAllowRearrangeTabs( true );
		tabs->setAllowDragAndDropTabs( true );
		tabs->setSplitFunction(
			[this]( SplitDirection direction, UITabWidget* ) -> UITabWidget* {
				splitDirection = direction;
				splits++;
				auto* newTabs = UITabWidget::New();
				newTabs->setParent( app.getUI() );
				newTabs->setPixelsSize( 400, 300 );
				newTabs->setAllowDragAndDropTabs( true );
				return newTabs;
			},
			0.1f );
		tick();
	}

	int splits{ 0 };
	std::optional<SplitDirection> splitDirection;

	void moveMouse( const Vector2f& position ) {
		app.getWindow()->getInput()->setMousePos( position.asInt() );
		tick();
	}

	void mouseButton( bool down ) {
		auto* input = app.getWindow()->getInput();
		input->cleanStates();
		InputEvent event{};
		event.WinID = app.getWindow()->getWindowID();
		event.Type = down ? InputEvent::MouseButtonDown : InputEvent::MouseButtonUp;
		const Vector2i pos( input->getMousePos() );
		event.button = { 0, EE_BUTTON_LEFT, static_cast<Uint8>( down ? 1 : 0 ),
						 static_cast<Int16>( pos.x ), static_cast<Int16>( pos.y ) };
		input->processEventForWindow( &event );
		tick();
	}

	void dragTab( UITab* tab, const std::vector<Vector2f>& path ) {
		moveMouse( tab->convertToWorldSpace( tab->getPixelsSize().asFloat() * 0.5f ) );
		mouseButton( true );
		// Multiple steps: the first ones start the drag.
		for ( const auto& position : path )
			moveMouse( position );
		mouseButton( false );
		tick();
	}

	bool allTabsEnabled() {
		for ( Uint32 i = 0; i < tabs->getTabCount(); i++ ) {
			if ( !tabs->getTab( i )->isEnabled() )
				return false;
		}
		return true;
	}
};

UTEST( UITabWidget, TopTabDragStillReordersAndSplits ) {
	TabDragTest test( 3 );
	auto* tabs = test.tabs;
	auto* first = tabs->getTab( 0 );

	// Along the strip it reorders, without detaching.
	const Vector2f barCenter =
		first->convertToWorldSpace( first->getPixelsSize().asFloat() * 0.5f );
	const Float endX = tabs->getTab( 1 )->convertToWorldSpace( Vector2f::Zero ).x +
					   tabs->getTab( 1 )->getPixelsSize().getWidth() * 0.75f;
	test.dragTab( first, { barCenter + Vector2f( 8, 0 ), barCenter + Vector2f( 16, 0 ),
						   Vector2f( endX, barCenter.y ), Vector2f( endX, barCenter.y ) } );
	EXPECT_EQ( tabs->getTabIndex( first ), Uint32{ 1 } );
	EXPECT_EQ( test.splits, 0 );
	EXPECT_TRUE( test.allTabsEnabled() );
	EXPECT_TRUE( first->getParent() == tabs->getTabBar() );

	// Across the strip it detaches, and the right content edge requests a split.
	test.dragTab( first, { { 400, 100 }, { 400, 200 }, { 500, 300 }, { 790, 300 }, { 790, 301 } } );
	EXPECT_EQ( test.splits, 1 );
	EXPECT_TRUE( test.splitDirection == SplitDirection::Right );
	EXPECT_EQ( tabs->getTabCount(), Uint32{ 2 } );
	EXPECT_EQ( tabs->getTabIndex( first ), eeINDEX_NOT_FOUND );
	EXPECT_TRUE( test.allTabsEnabled() );
}

UTEST( UITabWidget, VerticalTabDragUsesTheOppositeAxes ) {
	TabDragTest test( 4 );
	auto* tabs = test.tabs;
	tabs->setTabPosition( UITabWidget::TabPosition::Left );
	test.tick();
	auto* first = tabs->getTab( 0 );

	// Down the bar it reorders: the X coordinate must not move the row.
	test.dragTab( first, { { 100, 20 }, { 100, 26 }, { 104, 30 }, { 104, 36 }, { 104, 36 } } );
	EXPECT_EQ( tabs->getTabIndex( first ), Uint32{ 1 } );
	EXPECT_EQ( test.splits, 0 );
	EXPECT_NEAR( first->getPixelsPosition().x, 0.f, 0.01f );
	EXPECT_NEAR( first->getPixelsPosition().y,
				 tabs->getTabBar()->getPixelsPadding().Top + tabs->getTabsHeight(), 0.01f );
	EXPECT_TRUE( test.allTabsEnabled() );
	EXPECT_TRUE( first->getParent() == tabs->getTabBar() );

	// Dragged into the content, inside its central area, it doesn't request a split.
	auto* second = tabs->getTab( 2 );
	test.dragTab( second, { { 100, 70 }, { 200, 70 }, { 300, 100 }, { 500, 300 }, { 501, 300 } } );
	EXPECT_EQ( test.splits, 0 );
	EXPECT_TRUE( second->getTabWidget() == tabs );
	EXPECT_EQ( tabs->getTabCount(), Uint32{ 4 } );
	EXPECT_TRUE( second->getParent() == tabs->getTabBar() );
	EXPECT_TRUE( test.allTabsEnabled() );

	// The left edge of the content, not of the widget, requests the split.
	test.dragTab( second, { { 100, 70 }, { 200, 70 }, { 300, 100 }, { 230, 300 }, { 231, 300 } } );
	EXPECT_EQ( test.splits, 1 );
	EXPECT_TRUE( test.splitDirection == SplitDirection::Left );
	EXPECT_EQ( tabs->getTabCount(), Uint32{ 3 } );
}

UTEST( UITabWidget, VerticalDropOnTheTabBarDoesNotSplit ) {
	TabDragTest test( 3 );
	auto* tabs = test.tabs;
	tabs->setTabPosition( UITabWidget::TabPosition::Right );
	test.tick();
	auto* tab = tabs->getTab( 0 );

	// Out of the bar and back to it, and over the separator.
	test.dragTab( tab, { { 700, 20 }, { 600, 20 }, { 500, 300 }, { 700, 300 }, { 701, 300 } } );
	EXPECT_EQ( test.splits, 0 );
	test.dragTab( tab, { { 700, 20 }, { 600, 20 }, { 500, 300 }, { 598, 300 }, { 598, 301 } } );
	EXPECT_EQ( test.splits, 0 );
	EXPECT_TRUE( tab->getTabWidget() == tabs );
	EXPECT_EQ( tabs->getTabCount(), Uint32{ 3 } );
}

UTEST( UITabWidget, TransferBetweenPositionsAdaptsTheTab ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	auto* source = UITabWidget::New();
	source->setParent( app.getUI() );
	source->setPixelsSize( 400, 300 );
	source->setTabsHeight( 24 );
	source->setAllowDragAndDropTabs( true );
	source->setTabPosition( UITabWidget::TabPosition::Left );
	auto* destination = UITabWidget::New();
	destination->setParent( app.getUI() );
	destination->setPixelsSize( 400, 300 );
	destination->setTabsHeight( 24 );
	destination->setAllowDragAndDropTabs( true );
	destination->add( "Existing", UIWidget::New() );
	auto* page = UIWidget::New();
	auto* tab = source->add( "Moved", page );
	EXPECT_NEAR( tab->getPixelsSize().getWidth(), 200.f, 0.01f );

	transferTabTo( tab, destination );

	EXPECT_TRUE( tab->getTabWidget() == destination );
	EXPECT_FALSE( tab->expandTextView() );
	EXPECT_TRUE( tab->getPixelsSize().getWidth() < 200.f );
	EXPECT_TRUE( destination->getContainerNode()->isParentOf( page ) );

	destination->setTabPosition( UITabWidget::TabPosition::Right );
	EXPECT_NEAR( tab->getPixelsSize().getWidth(), 200.f, 0.01f );
	transferTabTo( tab, source );
	EXPECT_TRUE( tab->getTabWidget() == source );
	EXPECT_NEAR( tab->getPixelsSize().getWidth(), 200.f, 0.01f );
	EXPECT_NEAR( page->getPixelsSize().getWidth(), 196.f, 0.01f );
}

UTEST( UITabWidgetSplitter, SideTabsDoNotAlterTheSplitTree ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	splitter->setHideTabBarOnSingleTab( false );
	auto* container = UILayout::New();
	container->setParent( app.getUI() );
	splitter->setOnTabWidgetCreateCb( []( UITabWidget* tabWidget ) {
		tabWidget->setTabPosition( UITabWidget::TabPosition::Left );
	} );
	auto* firstTabs = splitter->createTabWidget( container );
	auto* firstWidget = UIWidget::New();
	splitter->createWidgetInTabWidget( firstTabs, firstWidget, "First" );
	auto* secondTabs = splitter->splitTabWidget( SplitDirection::Right, firstTabs );
	ASSERT_TRUE( secondTabs != nullptr );
	auto* secondWidget = UIWidget::New();
	splitter->createWidgetInTabWidget( secondTabs, secondWidget, "Second" );

	EXPECT_TRUE( secondTabs->getTabPosition() == UITabWidget::TabPosition::Left );
	EXPECT_TRUE( splitter->splitterFromWidget( firstWidget ) == firstTabs->getParent() );
	EXPECT_TRUE( splitter->splitterFromWidget( secondWidget ) == secondTabs->getParent() );

	const auto layout = splitter->toJSON();
	EXPECT_TRUE( layout["type"] == "splitter" );
	EXPECT_TRUE( layout["first"]["type"] == "tabwidget" );
	EXPECT_TRUE( layout["last"]["type"] == "tabwidget" );

	splitter->closeTab( secondWidget, UITabWidget::FocusTabBehavior::Default );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_EQ( splitter->getTabWidgets().size(), 1UL );
	EXPECT_TRUE( firstTabs->getParent() == container );
	eeDelete( splitter );
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

UTEST( UIAbstractTableView, SmoothScrollingSurvivesRubberBandCleanup ) {
	UIApplication app(
		WindowSettings( 600, 300, "eepp - table scrolling test" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* input = app.getWindow()->getInput();
	for ( int cleanup = 0; cleanup < 3; ++cleanup ) {
		auto model = std::make_shared<ThreeColumnModel>();
		auto* table = UITableView::New();
		table->setParent( scene->getRoot() );
		table->setPixelsSize( 500, 240 );
		table->setRowHeight( 24 );
		table->setModel( model );
		table->setColumnsWidth( 100 );
		table->setSelectionKind( UIAbstractView::SelectionKind::Multiple );
		table->setSelectionType( UIAbstractView::SelectionType::Row );
		table->setRubberBandSelection( true );
		scene->flushDirtyStyleAndLayout();
		table->drawChildren();

		if ( cleanup == 0 ) {
			// Applying table flags can disable rubber-band selection before smooth scrolling
			// is first enabled.
			table->setRubberBandSelection( false );
		} else {
			// Both a click and a rectangle selection in the empty area end the band.
			input->cleanStates();
			input->setMousePos( table->convertToWorldSpace( { 40, 180 } ).asInt() );
			input->injectButtonPress( EE_BUTTON_LEFT );
			scene->update( Milliseconds( 16 ) );
			if ( cleanup == 2 ) {
				input->setMousePos( table->convertToWorldSpace( { 80, 80 } ).asInt() );
				scene->update( Milliseconds( 16 ) );
				EXPECT_TRUE( table->rubberBandDragged() );
			}
			input->cleanStates();
			input->injectButtonRelease( EE_BUTTON_LEFT );
			scene->update( Milliseconds( 16 ) );
		}
		input->cleanStates();
		// Scheduled removals take effect on the next scene update.
		scene->update( Milliseconds( 16 ) );
		table->setSmoothScrollEnabled( true );
		model->setRows( 100 );
		model->invalidate();
		scene->flushDirtyStyleAndLayout();
		table->drawChildren();
		ASSERT_TRUE( table->getVerticalScrollBar()->isEnabled() );

		// Wheel routing uses the hovered row even while another widget owns keyboard focus.
		scene->getRoot()->setFocus();
		auto* cell = table->getCellFromIndex( model->index( 0, 0 ) );
		ASSERT_TRUE( cell != nullptr );
		input->setMousePos( cell->convertToWorldSpace( { 10, 10 } ).asInt() );
		InputEvent wheel{};
		wheel.Type = InputEvent::MouseWheel;
		wheel.WinID = app.getWindow()->getWindowID();
		wheel.wheel.y = -0.5f;
		wheel.wheel.direction = InputEvent::WheelEvent::Normal;
		input->processEventForWindow( &wheel );
		scene->update( Milliseconds( 16 ) );
		EXPECT_GT( table->getVerticalScrollBar()->getValue(), 0.f );
		eeDelete( table );
		scene->update( Milliseconds( 16 ) );
	}
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

UTEST( UITabWidgetSplitter, ShowsEveryTabBarWhileSplitWhenEnabled ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	auto* container = UILayout::New();
	container->setParent( app.getUI() );
	auto* firstTabs = splitter->createTabWidget( container );
	auto* firstWidget = UIWidget::New();
	splitter->createWidgetInTabWidget( firstTabs, firstWidget, "First" );
	EXPECT_FALSE( firstTabs->getTabBar()->isVisible() );

	auto* secondTabs = splitter->splitTabWidget( SplitDirection::Right, firstTabs );
	ASSERT_TRUE( secondTabs != nullptr );
	EXPECT_FALSE( splitter->getShowTabBarWhenSplit() );
	EXPECT_FALSE( firstTabs->getTabBar()->isVisible() );
	EXPECT_FALSE( secondTabs->getTabBar()->isVisible() );
	splitter->setShowTabBarWhenSplit( true );
	EXPECT_TRUE( splitter->getShowTabBarWhenSplit() );
	EXPECT_TRUE( firstTabs->getTabBar()->isVisible() );
	EXPECT_TRUE( secondTabs->getTabBar()->isVisible() );
	splitter->setShowTabBarWhenSplit( false );
	EXPECT_FALSE( firstTabs->getTabBar()->isVisible() );
	EXPECT_FALSE( secondTabs->getTabBar()->isVisible() );
	splitter->setShowTabBarWhenSplit( true );
	splitter->setHideTabBar( true );
	EXPECT_FALSE( firstTabs->getTabBar()->isVisible() );
	EXPECT_FALSE( secondTabs->getTabBar()->isVisible() );
	splitter->setHideTabBar( false );
	EXPECT_TRUE( firstTabs->getTabBar()->isVisible() );
	EXPECT_TRUE( secondTabs->getTabBar()->isVisible() );
	auto* secondWidget = UIWidget::New();
	splitter->createWidgetInTabWidget( secondTabs, secondWidget, "Second" );
	splitter->closeTab( secondWidget, UITabWidget::FocusTabBehavior::Default );
	EXPECT_EQ( splitter->getTabWidgets().size(), 1UL );
	EXPECT_FALSE( firstTabs->getTabBar()->isVisible() );
	auto* newSplit = splitter->splitTabWidget( SplitDirection::Bottom, firstTabs );
	ASSERT_TRUE( newSplit != nullptr );
	EXPECT_TRUE( firstTabs->getTabBar()->isVisible() );
	EXPECT_TRUE( newSplit->getTabBar()->isVisible() );
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

UTEST( UITabWidgetSplitter, RestoresNestedSplitPartitions ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	const WidgetTypeCallback widgetType{ []( UIWidget* ) { return nlohmann::json::object(); },
										 []( const nlohmann::json& j ) -> WidgetLoadResult {
											 auto* widget = UIWidget::New();
											 widget->addClass( "nestedwidget" );
											 return { widget, nullptr, j.value( "title", "" ) };
										 } };
	const auto addWidget = []( UITabWidgetSplitter* splitter, UITabWidget* tabs ) {
		auto* widget = UIWidget::New();
		widget->addClass( "nestedwidget" );
		splitter->createWidgetInTabWidget( tabs, widget, "Pane" );
	};

	TestClient client1;
	auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
	splitter1->registerWidgetType( "nestedwidget", widgetType );
	auto* left = splitter1->createTabWidget( (Node*)app.getUI() );
	addWidget( splitter1, left );
	auto* right = splitter1->splitTabWidget( SplitDirection::Right, left );
	addWidget( splitter1, right );
	auto* top = splitter1->splitTabWidget( SplitDirection::Bottom, left );
	addWidget( splitter1, top );

	auto* inner = left->getParent()->asType<UISplitter>();
	auto* outer = right->getParent()->asType<UISplitter>();
	ASSERT_TRUE( inner != outer );
	inner->setSplitPartition( StyleSheetLength( "30%" ) );
	outer->setSplitPartition( StyleSheetLength( "70%" ) );
	const nlohmann::json saved = splitter1->toJSON();

	TestClient client2;
	auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
	splitter2->registerWidgetType( "nestedwidget", widgetType );
	splitter2->createTabWidget( (Node*)app.getUI() );
	splitter2->fromJSON( saved );

	ASSERT_EQ( splitter2->getTabWidgets().size(), 3UL );
	auto* restoredInner = splitter2->getFirstTabWidget()->getParent()->asType<UISplitter>();
	auto* restoredOuter = restoredInner->getParent()->asType<UISplitter>();
	EXPECT_STDSTREQ( restoredInner->getSplitPartition().toString(), "30%" );
	EXPECT_STDSTREQ( restoredOuter->getSplitPartition().toString(), "70%" );

	eeDelete( splitter1 );
	eeDelete( splitter2 );
}

UTEST( UITabWidgetSplitter, UnserializeRejectsWhatItCannotRestore ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	TestClient client;
	auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
	auto* tabs = splitter->createTabWidget( (Node*)app.getUI() );
	splitter->registerWidgetType( "failing",
								  { []( UIWidget* ) { return nlohmann::json::object(); },
									[]( const nlohmann::json& ) { return WidgetLoadResult{}; } } );

	// A widget that couldn't be created leaves no tab behind.
	splitter->fromJSON( nlohmann::json::parse(
		R"({"type":"tabwidget","current_page":0,"files":[{"type":"failing"}]})" ) );
	EXPECT_EQ( tabs->getTabCount(), (Uint32)0 );

	// Damaged data reports an error instead of reading missing keys.
	const auto throws = [&splitter]( const char* text ) {
		try {
			splitter->fromJSON( nlohmann::json::parse( text ) );
		} catch ( const nlohmann::json::exception& ) {
			return true;
		}
		return false;
	};
	EXPECT_TRUE( throws( R"({"type":"tabwidget"})" ) );
	EXPECT_TRUE( throws( R"({"type":"splitter","orientation":"horizontal"})" ) );

	eeDelete( splitter );
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
