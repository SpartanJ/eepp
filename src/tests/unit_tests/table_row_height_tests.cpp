#include "utest.hpp"
#include <cmath>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitreeview.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::System;

namespace {

class RowHeightModel : public Model {
  public:
	size_t rowCount( const ModelIndex& parent = {} ) const override {
		return parent.isValid() ? 0 : 40;
	}

	size_t columnCount( const ModelIndex& = {} ) const override { return 2; }

	std::string columnName( const size_t& column ) const override {
		return column == 0 ? "Name" : "Details";
	}

	Variant data( const ModelIndex&, ModelRole = ModelRole::Display ) const override {
		return "Sample text";
	}
};

class SortedRowHeightModel : public RowHeightModel {
  public:
	bool isSortable() override { return true; }

	int keyColumn() const override { return 0; }

	SortOrder sortOrder() const override { return SortOrder::Ascending; }
};

class RowHeightIcon : public UIIcon {
  public:
	RowHeightIcon() : UIIcon( "row-height-test" ) {}

	const DrawablePtr& getSource( const int& size ) const override {
		requestedSize = size;
		return UIIcon::getSource( size );
	}

	mutable int requestedSize{ 0 };
};

class IconRowHeightModel : public RowHeightModel {
  public:
	IconRowHeightModel() : mIcon( std::make_shared<RowHeightIcon>() ) {
		auto region = TextureRegion::New();
		region->setOriDestSize( { 32, 24 } );
		mDrawable = region;
		mIcon->setSource( 32, mDrawable );
	}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( role == ModelRole::Icon )
			return index.column() == 0 ? Variant( mIcon.get() ) : Variant( mDrawable );
		return RowHeightModel::data( index, role );
	}

	int requestedIconSize() const { return mIcon->requestedSize; }

  private:
	std::shared_ptr<RowHeightIcon> mIcon;
	DrawablePtr mDrawable;
};

class GlyphRowHeightModel : public RowHeightModel {
  public:
	explicit GlyphRowHeightModel( UIIconPtr icon ) : mIcon( std::move( icon ) ) {}

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override {
		if ( role == ModelRole::Icon )
			return index.column() == 0 ? Variant( mIcon.get() ) : Variant();
		return RowHeightModel::data( index, role );
	}

  private:
	UIIconPtr mIcon;
};

template <typename View> class RowHeightProbe : public View {
  public:
	RowHeightProbe() : View() {}

	int geometryUpdates{ 0 };

  protected:
	void createOrUpdateColumns( bool resetColumnData = false ) override {
		++geometryUpdates;
		View::createOrUpdateColumns( resetColumnData );
	}
};

void renderRows( UIApplication& app ) {
	for ( int i = 0; i < 5; ++i ) {
		app.getUI()->update( Milliseconds( 16 ) );
		app.getWindow()->clear();
		app.getUI()->draw();
		app.getWindow()->display( false, false );
	}
}

} // namespace

UTEST( TableRowHeight, GlyphIconsFollowTextSizeInsteadOfTheLargerIconBox ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Glyph icon sizing" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		ASSERT_TRUE( app.getUI() != nullptr );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto font = FontTrueType::New( "RowHeightNonicons", *app.getUI()->getResourceScope() );
		ASSERT_TRUE( font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/nonicons.ttf" ) );
		font->setIsEmojiFont( true );
		auto icon = UIGlyphIcon::New( "filetype-cpp", font.get(), 61719 );
		auto model = std::make_shared<GlyphRowHeightModel>( icon );
		UIAbstractTableView* views[] = { UITableView::New(), UITreeView::New() };
		for ( auto* view : views ) {
			view->setParent( app.getUI() );
			view->setPixelsSize( 360, 180 );
			view->setIconSize( 12 * density );
			view->setModel( model );
		}
		for ( Uint32 fontSize : { 12u, 24u, 12u } ) {
			app.getUI()->applyDefaultFont( nullptr, fontSize * density );
			renderRows( app );
			const auto expected = icon->getSource( fontSize * density )->getPixelsSize();
			for ( auto* view : views ) {
				auto* cell = view->getCellFromIndex( model->index( 0 ) );
				ASSERT_TRUE( cell != nullptr && cell->hasIcon() );
				EXPECT_TRUE( cell->getIcon()->getDrawable()->getPixelsSize() == expected );
				EXPECT_GT( cell->getIcon()->getPixelsSize().x, expected.x );
			}
		}
	}
}

UTEST( TableRowHeight, EmSizedIconsFollowCellFontChanges ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Font relative icons" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		ASSERT_TRUE( app.getUI() != nullptr );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto model = std::make_shared<IconRowHeightModel>();
		auto* table = UITableView::New();
		auto* tree = UITreeView::New();
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			view->setParent( app.getUI() );
			view->setPixelsSize( 360, 180 );
			view->setModel( model );
		}
		tree->setPixelsPosition( 400, 0 );
		for ( Uint32 fontSize : { 12u, 6u, 24u, 48u, 3u } ) {
			app.getUI()->applyDefaultFont( nullptr, fontSize * density );
			renderRows( app );
			for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
								 static_cast<UIAbstractTableView*>( tree ) } ) {
				for ( int column = 0; column < 2; ++column ) {
					auto* cell = view->getCellFromIndex( model->index( 0, column ) );
					ASSERT_TRUE( cell != nullptr );
					ASSERT_TRUE( cell->hasIcon() );
					const Float iconSize = std::round( fontSize * density * 1.5f );
					EXPECT_EQ( cell->getIcon()->getPixelsSize().x, iconSize );
					EXPECT_GE( cell->getIcon()->getPixelsSize().y,
							   eemin( iconSize, view->getRowHeight() ) );
					EXPECT_LE( cell->getIcon()->getPixelsSize().y, view->getRowHeight() );
					EXPECT_GE( model->requestedIconSize(), iconSize );
					EXPECT_TRUE( cell->getIcon()->getDrawable()->getPixelsSize() ==
								 Sizef( 32 * density, 24 * density ) );
				}
			}
		}
		// A panel can override a cell's font independently of the scene's default font.
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			view->setRowHeight( 30 * density );
			view->getCellFromIndex( model->index( 0 ) )
				->applyProperty( CSS::StyleSheetProperty( "font-size", "9dp" ) );
		}
		renderRows( app );
		const Float overrideSize = std::round( 9 * density * 1.5f );
		const Float defaultSize = std::round( 3 * density * 1.5f );
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			EXPECT_EQ( view->getCellFromIndex( model->index( 0 ) )->getIcon()->getPixelsSize().x,
					   overrideSize );
			EXPECT_EQ( view->getCellFromIndex( model->index( 0, 1 ) )->getIcon()->getPixelsSize().x,
					   defaultSize );
		}
	}
}

UTEST( TableRowHeight, AutomaticRowsFollowFontChangesWithoutModelRefresh ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Automatic row heights" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		ASSERT_TRUE( app.getUI() != nullptr );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto model = std::make_shared<RowHeightModel>();
		auto* table = UITableView::New();
		auto* tree = UITreeView::New();
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			view->setParent( app.getUI() );
			view->setPixelsSize( 360, 180 );
			view->setModel( model );
		}
		tree->setPixelsPosition( 400, 0 );
		renderRows( app );
		const Float tableHeight = table->getRowHeight();
		const Float treeHeight = tree->getRowHeight();
		const Float pageStep = table->getVerticalScrollBar()->getPageStep();
		// Preserve the existing theme/header-derived default spacing.
		EXPECT_EQ( tableHeight, table->getHeaderColumn( 0 )->getPixelsSize().getHeight() );
		EXPECT_EQ( treeHeight, tree->getHeaderColumn( 0 )->getPixelsSize().getHeight() );
		const auto fontSize =
			table->getCellFromIndex( model->index( 0 ) )->getTextView()->getFontSize();
		app.getUI()->applyDefaultFont( nullptr, fontSize * 3 );
		renderRows( app );
		EXPECT_LT( table->getVerticalScrollBar()->getPageStep(), pageStep );
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			auto* cell = view->getCellFromIndex( model->index( 0 ) );
			ASSERT_TRUE( cell != nullptr );
			EXPECT_GE( view->getRowHeight(), cell->getTextView()->getTextHeight() );
			EXPECT_GT( view->getRowHeight(), tableHeight );
			EXPECT_EQ( cell->getPixelsSize().getHeight(), view->getRowHeight() );
			EXPECT_EQ( view->getContentSize().getHeight(),
					   view->getHeaderHeight() + 40 * view->getRowHeight() );
			view->scrollToBottom();
		}
		renderRows( app );
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			auto* last = view->getCellFromIndex( model->index( 39 ) );
			ASSERT_TRUE( last != nullptr );
			EXPECT_LE( last->getWorldBounds().Bottom, view->getWorldBounds().Bottom );
			EXPECT_GT( last->getWorldBounds().Bottom, view->getWorldBounds().Top );
		}
		app.getUI()->applyDefaultFont( nullptr, fontSize );
		renderRows( app );
		EXPECT_EQ( tableHeight, table->getRowHeight() );
		EXPECT_EQ( treeHeight, tree->getRowHeight() );
		EXPECT_EQ( pageStep, table->getVerticalScrollBar()->getPageStep() );
		table->setHeadersVisible( false );
		tree->setHeadersVisible( false );
		renderRows( app );
		EXPECT_EQ( tableHeight, table->getRowHeight() );
		EXPECT_EQ( treeHeight, tree->getRowHeight() );
	}
}

UTEST( TableRowHeight, CellFontAndPaddingGrowAutomaticRowsButExplicitHeightsStayFixed ) {
	UIApplication app(
		WindowSettings( 800, 600, "Cell row heights" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	ASSERT_TRUE( app.getUI() != nullptr );
	auto model = std::make_shared<RowHeightModel>();
	auto* table = UITableView::New();
	auto* tree = UITreeView::New();
	for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
						 static_cast<UIAbstractTableView*>( tree ) } ) {
		view->setParent( app.getUI() );
		view->setPixelsSize( 360, 180 );
		view->setModel( model );
	}
	tree->setPixelsPosition( 400, 0 );
	renderRows( app );
	for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
						 static_cast<UIAbstractTableView*>( tree ) } ) {
		view->setHeadersVisible( false );
		auto* cell = view->getCellFromIndex( model->index( 0, 1 ) );
		ASSERT_TRUE( cell != nullptr );
		cell->getTextView()->setFontSize( 36 );
		cell->setPaddingPixelsTop( 5 );
		cell->setPaddingPixelsBottom( 7 );
	}
	renderRows( app );
	for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
						 static_cast<UIAbstractTableView*>( tree ) } ) {
		auto* cell = view->getCellFromIndex( model->index( 0, 1 ) );
		ASSERT_TRUE( cell != nullptr );
		EXPECT_GE( view->getRowHeight(), cell->getTextView()->getTextHeight() + 12 );
		view->setRowHeight( 18 );
	}
	renderRows( app );
	EXPECT_EQ( table->getRowHeight(), 18.f );
	EXPECT_EQ( tree->getRowHeight(), 18.f );
	app.getUI()->applyDefaultFont( nullptr, 48 );
	renderRows( app );
	EXPECT_EQ( table->getRowHeight(), 18.f );
	EXPECT_EQ( tree->getRowHeight(), 18.f );
	table->setRowHeight( 0 );
	tree->setRowHeight( 0 );
	renderRows( app );
	EXPECT_GT( table->getRowHeight(), 36.f );
	EXPECT_GT( tree->getRowHeight(), 36.f );
}

UTEST( TableRowHeight, FontBurstsShareOneUpdateAndStableHeightsDoNotRebuildGeometry ) {
	UIApplication app(
		WindowSettings( 800, 600, "Debounced row heights" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	ASSERT_TRUE( app.getUI() != nullptr );
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto model = std::make_shared<RowHeightModel>();
	auto* table = new RowHeightProbe<UITableView>();
	auto* tree = new RowHeightProbe<UITreeView>();
	for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
						 static_cast<UIAbstractTableView*>( tree ) } ) {
		view->setParent( app.getUI() );
		view->setPixelsSize( 360, 180 );
		view->setModel( model );
	}
	tree->setPixelsPosition( 400, 0 );
	renderRows( app );
	table->geometryUpdates = tree->geometryUpdates = 0;
	const Float initialHeight = table->getRowHeight();
	// A burst changes all header/cell fonts repeatedly before the next UI update.
	for ( Uint32 size = 20; size <= 40; ++size )
		app.getUI()->applyDefaultFont( nullptr, size );
	EXPECT_EQ( table->getRowHeight(), initialHeight );
	renderRows( app );
	EXPECT_GT( table->getRowHeight(), initialHeight );
	EXPECT_EQ( table->geometryUpdates, 1 );
	EXPECT_EQ( tree->geometryUpdates, 1 );
	table->geometryUpdates = tree->geometryUpdates = 0;
	// Recycling text of the same height and scrolling must not rebuild the columns.
	for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
						 static_cast<UIAbstractTableView*>( tree ) } ) {
		view->getCellFromIndex( model->index( 0 ) )->setText( "Different width, same height" );
		view->scrollToBottom();
	}
	renderRows( app );
	EXPECT_EQ( table->geometryUpdates, 0 );
	EXPECT_EQ( tree->geometryUpdates, 0 );
	// Settled views retain no pending row-height work across idle frames.
	renderRows( app );
	EXPECT_TRUE( table->getActionsByTag( String::hash( "updateTableRowHeight" ) ).empty() );
	EXPECT_TRUE( tree->getActionsByTag( String::hash( "updateTableRowHeight" ) ).empty() );
	// Layout-assigned cell geometry and unrelated child text are not row-height inputs.
	for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
						 static_cast<UIAbstractTableView*>( tree ) } ) {
		auto* cell = view->getCellFromIndex( model->index( 39 ) );
		ASSERT_TRUE( cell != nullptr );
		cell->setPixelsSize( cell->getPixelsSize().getWidth() + 1, view->getRowHeight() );
		auto* embedded = UITextView::New();
		embedded->setParent( cell );
		embedded->setFontSize( 80 );
		embedded->setText( "Embedded content" );
		EXPECT_TRUE( view->getActionsByTag( String::hash( "updateTableRowHeight" ) ).empty() );
	}
}

UTEST( TableRowHeight, CellMetricsPassThroughIntermediateLayouts ) {
	UIApplication app(
		WindowSettings( 800, 600, "Cell metrics through layouts" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto model = std::make_shared<RowHeightModel>();
	auto* table = UITableView::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 360, 180 );
	table->setModel( model );
	renderRows( app );
	auto* cell = table->getCellFromIndex( model->index( 0 ) );
	ASSERT_TRUE( cell != nullptr );
	const auto originalHeight = table->getRowHeight();
	const auto originalFontSize = cell->getTextView()->getFontSize();
	auto* layout = UILinearLayout::NewHorizontal();
	layout->setParent( cell->getParent() );
	layout->setPixelsSize( cell->getPixelsSize() );
	cell->setParent( layout );
	renderRows( app );
	// The layout consumes ordinary LayoutAttributeChange messages. The cell's specific
	// notification still reaches the table without depending on the cell's parent depth.
	cell->getTextView()->setFontSize( 48 );
	renderRows( app );
	EXPECT_GE( table->getRowHeight(), cell->getTextView()->getTextHeight() );
	EXPECT_GT( table->getRowHeight(), originalHeight );
	cell->getTextView()->setFontSize( originalFontSize );
	renderRows( app );
	EXPECT_EQ( table->getRowHeight(), originalHeight );
}

UTEST( TableRowHeight, HeaderMetricsUpdateWhileRowsHaveExplicitHeight ) {
	UIApplication app(
		WindowSettings( 800, 600, "Header metrics with fixed rows" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto model = std::make_shared<RowHeightModel>();
	auto* table = UITableView::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 360, 180 );
	table->setModel( model );
	table->setRowHeight( 18 );
	renderRows( app );
	auto* header = table->getHeaderColumn( 1 );
	header->getTextView()->setFontSize( 36 );
	header->setPaddingPixelsTop( 5 );
	header->setPaddingPixelsBottom( 7 );
	renderRows( app );
	const auto expandedHeight = table->getHeaderHeight();
	EXPECT_GE( expandedHeight, header->getTextView()->getTextHeight() + 12 );
	EXPECT_EQ( table->getRowHeight(), 18.f );
	// Header constraints also invalidate the natural height, even without a text change.
	for ( size_t column = 0; column < 2; ++column )
		table->getHeaderColumn( column )->setMaxHeightEq( "15px" );
	renderRows( app );
	EXPECT_EQ( table->getHeaderHeight(), 15.f );
	for ( size_t column = 0; column < 2; ++column )
		table->getHeaderColumn( column )->setMaxHeightEq( "" );
	renderRows( app );
	EXPECT_EQ( table->getHeaderHeight(), expandedHeight );
	EXPECT_EQ( table->getRowHeight(), 18.f );
}

UTEST( TableRowHeight, NestedTablesConsumeTheirOwnMetricsNotifications ) {
	UIApplication app(
		WindowSettings( 800, 600, "Nested table metrics" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto model = std::make_shared<RowHeightModel>();
	auto* outer = UITableView::New();
	outer->setParent( app.getUI() );
	outer->setPixelsSize( 700, 500 );
	outer->setModel( model );
	renderRows( app );
	auto* outerCell = outer->getCellFromIndex( model->index( 0 ) );
	ASSERT_TRUE( outerCell != nullptr );
	auto* inner = UITableView::New();
	inner->setParent( outerCell );
	inner->setPixelsSize( 360, 180 );
	inner->setModel( model );
	renderRows( app );
	const auto outerHeight = outer->getRowHeight();
	for ( Float innerRowHeight : { 0.f, 18.f } ) {
		inner->setRowHeight( innerRowHeight );
		renderRows( app );
		auto* innerCell = inner->getCellFromIndex( model->index( 0 ) );
		ASSERT_TRUE( innerCell != nullptr );
		const Uint32 fontSize = innerRowHeight == 0 ? 36 : 48;
		inner->getHeaderColumn( 0 )->getTextView()->setFontSize( fontSize );
		innerCell->getTextView()->setFontSize( fontSize );
		EXPECT_TRUE( outer->getActionsByTag( String::hash( "updateTableRowHeight" ) ).empty() );
		renderRows( app );
		EXPECT_EQ( outer->getRowHeight(), outerHeight );
	}
}

UTEST( TableRowHeight, SortedHeadersDoNotClampRowsWhenFontsShrinkBelowInitialSize ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Shrinking automatic rows" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		app.getUI()->applyDefaultFont( nullptr, PixelDensity::dpToPxI( 12 ) );
		auto model = std::make_shared<SortedRowHeightModel>();
		auto* table = UITableView::New();
		auto* tree = UITreeView::New();
		for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
							 static_cast<UIAbstractTableView*>( tree ) } ) {
			view->setParent( app.getUI() );
			view->setPixelsSize( 360, 180 );
			view->setModel( model );
		}
		tree->setPixelsPosition( 400, 0 );
		renderRows( app );
		Float previousTableHeight = table->getRowHeight();
		Float previousTreeHeight = tree->getRowHeight();
		for ( Uint32 fontSize : { 6u, 3u } ) {
			app.getUI()->applyDefaultFont( nullptr, PixelDensity::dpToPxI( fontSize ) );
			renderRows( app );
			EXPECT_LT( table->getRowHeight(), previousTableHeight );
			EXPECT_LT( tree->getRowHeight(), previousTreeHeight );
			previousTableHeight = table->getRowHeight();
			previousTreeHeight = tree->getRowHeight();
			for ( auto* view : { static_cast<UIAbstractTableView*>( table ),
								 static_cast<UIAbstractTableView*>( tree ) } ) {
				auto* header = view->getHeaderColumn( 0 );
				EXPECT_GE( view->getHeaderHeight(),
						   header->getExtraInnerWidget()->getPixelsSize().getHeight() );
				EXPECT_EQ( header->getPixelsSize().getHeight(), view->getHeaderHeight() );
			}
		}
		EXPECT_LT( table->getRowHeight(), table->getHeaderHeight() );
		EXPECT_LT( tree->getRowHeight(), tree->getHeaderHeight() );
	}
}

UTEST( TableRowHeight, ColumnVisibilityInvalidatesCachedCellMetrics ) {
	UIApplication app(
		WindowSettings( 800, 600, "Column visibility and row heights" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto model = std::make_shared<RowHeightModel>();
	for ( auto* view : { static_cast<UIAbstractTableView*>( UITableView::New() ),
						 static_cast<UIAbstractTableView*>( UITreeView::New() ) } ) {
		view->setParent( app.getUI() );
		view->setPixelsSize( 360, 180 );
		view->setModel( model );
		renderRows( app );
		const auto originalHeight = view->getRowHeight();
		auto* cell = view->getCellFromIndex( model->index( 0, 1 ) );
		ASSERT_TRUE( cell != nullptr );
		cell->getTextView()->setFontSize( 48 );
		renderRows( app );
		const auto tallHeight = view->getRowHeight();
		EXPECT_GT( tallHeight, originalHeight );
		view->setColumnHidden( 1, true );
		renderRows( app );
		EXPECT_EQ( view->getRowHeight(), originalHeight );
		view->setColumnsHidden( { 1 }, false );
		renderRows( app );
		EXPECT_EQ( view->getRowHeight(), tallHeight );
		view->setColumnsHidden( { 1 }, true );
		renderRows( app );
		EXPECT_EQ( view->getRowHeight(), originalHeight );
	}
}

UTEST( TableRowHeight, HidingEveryColumnKeepsPositiveRowStride ) {
	UIApplication app(
		WindowSettings( 800, 600, "Hidden column row stride" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto model = std::make_shared<RowHeightModel>();
	for ( auto* view : { static_cast<UIAbstractTableView*>( UITableView::New() ),
						 static_cast<UIAbstractTableView*>( UITreeView::New() ) } ) {
		view->setParent( app.getUI() );
		view->setPixelsSize( 360, 180 );
		view->setModel( model );
		// Hide columns before any cells have been allocated, then change the font too.
		view->setColumnsHidden( { 0, 1 }, true );
		EXPECT_GT( view->getRowHeight(), 0.f );
		renderRows( app );
		app.getUI()->applyDefaultFont( nullptr, 36 );
		renderRows( app );
		EXPECT_GT( view->getRowHeight(), 0.f );
		view->setColumnsHidden( { 0, 1 }, false );
		renderRows( app );
		EXPECT_GE( view->getRowHeight(),
				   view->getCellFromIndex( model->index( 0 ) )->getTextView()->getTextHeight() );
	}
}

UTEST( TableRowHeight, HeaderSkinsAndConstraintsDoNotClampBodyRows ) {
	for ( Float density : { 1.f, 2.f } ) {
		UIApplication app( WindowSettings( 800, 600, "Independent header skin height" ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
		auto* table = UITableView::New();
		table->setParent( app.getUI() );
		table->setPixelsSize( 360, 300 );
		table->setModel( std::make_shared<RowHeightModel>() );
		renderRows( app );
		const auto rowHeight = table->getRowHeight();
		auto skin = UISkin::New( "row-height-header" );
		// Texture-free regions provide fixed intrinsic dimensions without raster assets.
		auto normal = TextureRegion::New();
		normal->setOriDestSize( { 100, 60 } );
		auto selected = TextureRegion::New();
		selected->setOriDestSize( { 100, 80 } );
		skin->setStateDrawable( UIState::StateFlagNormal, normal );
		skin->setStateDrawable( UIState::StateFlagSelected, selected );
		auto* header = table->getHeaderColumn( 0 );
		header->setSkin( *skin );
		renderRows( app );
		const auto& padding = header->getPixelsPadding();
		EXPECT_GE( table->getHeaderHeight(),
				   PixelDensity::dpToPx( 80 ) + padding.Top + padding.Bottom );
		EXPECT_EQ( table->getHeaderHeight(), header->getPixelsSize().getHeight() );
		EXPECT_EQ( table->getRowHeight(), rowHeight );
		header->setMinHeight( 100 );
		renderRows( app );
		EXPECT_GE( table->getHeaderHeight(), PixelDensity::dpToPx( 100 ) );
		EXPECT_EQ( table->getRowHeight(), rowHeight );
	}
}

UTEST( TableRowHeight, HiddenHeaderFontChangesApplyWhenHeaderIsShown ) {
	UIApplication app(
		WindowSettings( 800, 600, "Hidden header metrics" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* table = UITableView::New();
	table->setParent( app.getUI() );
	table->setPixelsSize( 360, 180 );
	table->setModel( std::make_shared<RowHeightModel>() );
	renderRows( app );
	const auto rowHeight = table->getRowHeight();
	table->setHeadersVisible( false );
	auto* header = table->getHeaderColumn( 0 );
	header->getTextView()->setFontSize( 48 );
	renderRows( app );
	EXPECT_EQ( table->getRowHeight(), rowHeight );
	EXPECT_EQ( table->getHeaderHeight(), 0.f );
	table->setHeadersVisible( true );
	renderRows( app );
	EXPECT_GE( table->getHeaderHeight(), header->getTextView()->getTextHeight() );
	EXPECT_EQ( table->getHeaderHeight(), header->getPixelsSize().getHeight() );
	EXPECT_EQ( table->getRowHeight(), rowHeight );
}
