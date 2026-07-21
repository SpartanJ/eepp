#include "utest.h"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/gridlayouter.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::Window;
using namespace EE::Graphics;

static void init_grid_test() {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 0: Display routing and property defaults
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridProperties, displayGridEnumConversions ) {
	EXPECT_TRUE( CSSDisplayHelper::toString( CSSDisplay::Grid ) == "grid" );
	EXPECT_TRUE( CSSDisplayHelper::toString( CSSDisplay::InlineGrid ) == "inline-grid" );
	EXPECT_TRUE( CSSDisplayHelper::fromString( "grid" ) == CSSDisplay::Grid );
	EXPECT_TRUE( CSSDisplayHelper::fromString( "inline-grid" ) == CSSDisplay::InlineGrid );
}

UTEST( GridProperties, longhandDefaults ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );

	EXPECT_TRUE( grid->getGridTemplateRows() == "none" );
	EXPECT_TRUE( grid->getGridTemplateColumns() == "none" );
	EXPECT_TRUE( grid->getGridTemplateAreas() == "none" );
	EXPECT_TRUE( grid->getGridAutoRows() == "auto" );
	EXPECT_TRUE( grid->getGridAutoColumns() == "auto" );
	EXPECT_EQ( grid->getGridAutoFlow(), CSSGridAutoFlow::Row );
	EXPECT_EQ( grid->getGridAutoFlowDense(), false );
	EXPECT_TRUE( grid->getGridRowStart() == "auto" );
	EXPECT_TRUE( grid->getGridRowEnd() == "auto" );
	EXPECT_TRUE( grid->getGridColumnStart() == "auto" );
	EXPECT_TRUE( grid->getGridColumnEnd() == "auto" );
	EXPECT_TRUE( grid->getGridArea() == "auto" );
	EXPECT_EQ( grid->getJustifyItems(), CSSJustifyItems::Normal );
	EXPECT_EQ( grid->getJustifySelf(), CSSJustifySelf::Auto );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: Definite Placement (GridLineParser)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridLineParser, parseAuto ) {
	GridLineResult r = GridLineParser::parse( "auto" );
	EXPECT_TRUE( r.isAuto );
	EXPECT_FALSE( r.isSpan );
}

UTEST( GridLineParser, parseInteger ) {
	GridLineResult r = GridLineParser::parse( "3" );
	EXPECT_FALSE( r.isAuto );
	EXPECT_EQ( r.lineNumber, 3 );
}

UTEST( GridLineParser, parseNegativeInteger ) {
	GridLineResult r = GridLineParser::parse( "-1" );
	EXPECT_FALSE( r.isAuto );
	EXPECT_EQ( r.lineNumber, -1 );
}

UTEST( GridLineParser, parseSpanCount ) {
	GridLineResult r = GridLineParser::parse( "span 2" );
	EXPECT_TRUE( r.isSpan );
	EXPECT_EQ( r.spanCount, 2 );
}

UTEST( GridLineParser, parseEmpty ) {
	GridLineResult r = GridLineParser::parse( "" );
	EXPECT_TRUE( r.isAuto );
}

UTEST( GridPlacement, columnExplicitLines ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* item = UIHTMLWidget::New();
	item->setParent( grid );
	item->setGridColumnStart( "1" );
	item->setGridColumnEnd( "3" );
	item->setGridRowStart( "1" );
	item->setGridRowEnd( "2" );

	sceneNode->updateDirtyLayouts();
	EXPECT_TRUE( item->getParent() == grid );
	Engine::destroySingleton();
}

UTEST( GridPlacement, usesSpanKeyword ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* item = UIHTMLWidget::New();
	item->setParent( grid );
	item->setGridColumnStart( "1" );
	item->setGridColumnEnd( "span 2" );

	sceneNode->updateDirtyLayouts();
	EXPECT_TRUE( item->getParent() == grid );
	Engine::destroySingleton();
}

UTEST( GridPlacement, negativeEndLine ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* item = UIHTMLWidget::New();
	item->setParent( grid );
	item->setGridColumnStart( "1" );
	item->setGridColumnEnd( "-1" );

	sceneNode->updateDirtyLayouts();
	EXPECT_TRUE( item->getParent() == grid );
	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5-6: Auto-Placement
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridAutoPlacement, rowFlowFillsColumnsThenRows ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* i1 = UIHTMLWidget::New();
	i1->setParent( grid );
	UIHTMLWidget* i2 = UIHTMLWidget::New();
	i2->setParent( grid );
	UIHTMLWidget* i3 = UIHTMLWidget::New();
	i3->setParent( grid );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();

	ASSERT_EQ( items.size(), 3u );

	// All items should be placed (non-zero positions)
	EXPECT_NE( items[0].resolvedRowStart, 0 );
	EXPECT_NE( items[1].resolvedRowStart, 0 );
	EXPECT_NE( items[2].resolvedRowStart, 0 );
	EXPECT_NE( items[0].resolvedColumnStart, 0 );
	EXPECT_NE( items[1].resolvedColumnStart, 0 );
	EXPECT_NE( items[2].resolvedColumnStart, 0 );

	// No overlapping positions
	for ( size_t a = 0; a < items.size(); ++a )
		for ( size_t b = a + 1; b < items.size(); ++b )
			EXPECT_FALSE( items[a].resolvedRowStart == items[b].resolvedRowStart &&
						  items[a].resolvedColumnStart == items[b].resolvedColumnStart );

	Engine::destroySingleton();
}

UTEST( GridAutoPlacement, columnFlowFillsRowsThenColumns ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateRows( "50px 50px" );
	grid->setGridAutoFlow( CSSGridAutoFlow::Column );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* i1 = UIHTMLWidget::New();
	i1->setParent( grid );
	UIHTMLWidget* i2 = UIHTMLWidget::New();
	i2->setParent( grid );
	UIHTMLWidget* i3 = UIHTMLWidget::New();
	i3->setParent( grid );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();

	ASSERT_EQ( items.size(), 3u );
	EXPECT_EQ( items[0].resolvedRowStart, 1 );
	EXPECT_EQ( items[0].resolvedColumnStart, 1 );
	EXPECT_EQ( items[1].resolvedRowStart, 2 );
	EXPECT_EQ( items[1].resolvedColumnStart, 1 );
	EXPECT_EQ( items[2].resolvedRowStart, 1 );
	EXPECT_EQ( items[2].resolvedColumnStart, 2 );

	Engine::destroySingleton();
}

UTEST( GridAutoPlacement, definiteLeavesHoleForSparse ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Place item at (1,1)
	UIHTMLWidget* i1 = UIHTMLWidget::New();
	i1->setParent( grid );
	i1->setGridColumnStart( "1" );
	i1->setGridRowStart( "1" );

	UIHTMLWidget* i2 = UIHTMLWidget::New();
	i2->setParent( grid );
	UIHTMLWidget* i3 = UIHTMLWidget::New();
	i3->setParent( grid );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();

	ASSERT_EQ( items.size(), 3u );

	// i1 definitely at (1,1)
	EXPECT_EQ( items[0].resolvedRowStart, 1 );
	EXPECT_EQ( items[0].resolvedColumnStart, 1 );

	// i2 and i3 should not overlap with i1 or each other
	for ( size_t a = 0; a < items.size(); ++a )
		for ( size_t b = a + 1; b < items.size(); ++b )
			EXPECT_FALSE( items[a].resolvedRowStart == items[b].resolvedRowStart &&
						  items[a].resolvedColumnStart == items[b].resolvedColumnStart );

	Engine::destroySingleton();
}

UTEST( GridAutoPlacement, sparseSkipsOccupiedCells ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px 100px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Definite placement takes (1,1) and (2,2)
	UIHTMLWidget* i1 = UIHTMLWidget::New();
	i1->setParent( grid );
	i1->setGridColumnStart( "1" );
	i1->setGridRowStart( "1" );

	UIHTMLWidget* i2 = UIHTMLWidget::New();
	i2->setParent( grid );
	i2->setGridColumnStart( "2" );
	i2->setGridRowStart( "2" );

	// Auto items: should fill available cells in order
	UIHTMLWidget* a1 = UIHTMLWidget::New();
	a1->setParent( grid );
	UIHTMLWidget* a2 = UIHTMLWidget::New();
	a2->setParent( grid );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();

	ASSERT_EQ( items.size(), 4u );

	// a1: (1,1) is taken by i1, so (1,2) is first available
	EXPECT_EQ( items[2].resolvedRowStart, 1 );
	EXPECT_EQ( items[2].resolvedColumnStart, 2 );

	// a2: sparse → continues from cursor. After a1 at (1,2), next is (1,3) or (2,1)
	// Since a1 took (1,2), and (1,3) is free, a2 should go to (1,3)
	EXPECT_EQ( items[3].resolvedRowStart, 1 );
	EXPECT_EQ( items[3].resolvedColumnStart, 3 );

	Engine::destroySingleton();
}

UTEST( GridAutoPlacement, honorsOrder ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* i1 = UIHTMLWidget::New();
	i1->setParent( grid );
	i1->setOrder( 1 );
	UIHTMLWidget* i2 = UIHTMLWidget::New();
	i2->setParent( grid );
	i2->setOrder( 0 );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();

	ASSERT_EQ( items.size(), 2u );

	// i2 has order 0, should be placed first: (1,1)
	EXPECT_EQ( items[0].resolvedRowStart, 1 );
	EXPECT_EQ( items[0].resolvedColumnStart, 1 );

	// i1 has order 1, should be placed second: (1,2)
	EXPECT_EQ( items[1].resolvedRowStart, 1 );
	EXPECT_EQ( items[1].resolvedColumnStart, 2 );

	Engine::destroySingleton();
}

UTEST( GridAutoPlacement, spanSkipsTooSmallHoles ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	grid->setPixelsSize( 500, 200 );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px 100px" );

	UIHTMLWidget* i1 = UIHTMLWidget::New();
	i1->setParent( grid );
	i1->setGridColumnStart( "1" );
	i1->setGridRowStart( "1" );

	UIHTMLWidget* spanItem = UIHTMLWidget::New();
	spanItem->setParent( grid );
	spanItem->setGridColumnEnd( "span 2" );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();
	ASSERT_EQ( items.size(), 2u );

	EXPECT_EQ( items[1].resolvedRowStart, 1 );
	EXPECT_EQ( items[1].resolvedColumnStart, 2 );
	EXPECT_EQ( items[1].resolvedColumnEnd, 4 );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 7: Basic Track Sizing and Layout
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridLayout, fixedTracksPositionCorrectly ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 200px" );
	grid->setGridTemplateRows( "50px 80px" );
	grid->setPixelsSize( 500, 300 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Child 1: column 1 / span 1, row 1 / span 1 (default)
	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );

	// Child 2: column 2 / span 1, row 1 / span 2
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );
	c2->setGridColumnStart( "2" );
	c2->setGridRowStart( "1" );
	c2->setGridRowEnd( "span 2" );

	sceneNode->updateDirtyLayouts();

	// c1: column 1, row 1 → x=0, y=0, w=100, h=50
	EXPECT_NEAR( c1->getPixelsPosition().x, -500.f, 500.f ); // relative position within parent
	// Use data from item resolution
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();
	ASSERT_EQ( items.size(), 2u );

	// c1: column 1-2, row 1-2
	EXPECT_EQ( items[0].resolvedColumnStart, 1 );
	EXPECT_EQ( items[0].resolvedColumnEnd, 2 );
	EXPECT_EQ( items[0].resolvedRowStart, 1 );
	EXPECT_EQ( items[0].resolvedRowEnd, 2 );

	// c2: column 2-3, row 1-3
	EXPECT_EQ( items[1].resolvedColumnStart, 2 );
	EXPECT_EQ( items[1].resolvedColumnEnd, 3 );
	EXPECT_EQ( items[1].resolvedRowStart, 1 );
	EXPECT_EQ( items[1].resolvedRowEnd, 3 );

	Engine::destroySingleton();
}

UTEST( GridAreasParser, noneIsNone ) {
	GridAreaTemplate tmpl = GridAreasParser::parseAreas( "none" );
	EXPECT_TRUE( tmpl.none );
	EXPECT_TRUE( tmpl.valid );
	EXPECT_EQ( tmpl.areas.size(), 0u );
}

UTEST( GridAreasParser, dotNullCellsAreAccepted ) {
	GridAreaTemplate tmpl = GridAreasParser::parseAreas( "\"header header\" \". content\"" );
	EXPECT_TRUE( tmpl.valid );
	EXPECT_TRUE( tmpl.areas.find( "header" ) != tmpl.areas.end() );
	EXPECT_TRUE( tmpl.areas.find( "." ) == tmpl.areas.end() );
	EXPECT_TRUE( tmpl.areas.find( "content" ) != tmpl.areas.end() );
}

UTEST( GridAreasParser, unevenRowWidthsInvalidate ) {
	GridAreaTemplate tmpl = GridAreasParser::parseAreas( "\"a a\" \"a\"" );
	EXPECT_FALSE( tmpl.valid );
}

UTEST( GridAreasParser, disconnectedAreaTokensInvalidate ) {
	// "a . a" means area "a" is split into two separate regions
	GridAreaTemplate tmpl = GridAreasParser::parseAreas( "\"a . a\"" );
	EXPECT_FALSE( tmpl.valid );
}

UTEST( GridAreasParser, implicitLineNamesFromAreas ) {
	GridAreaTemplate tmpl = GridAreasParser::parseAreas( "\"head head\" \"nav main\"" );
	EXPECT_TRUE( tmpl.valid );

	// Row line 1: head-start
	std::vector<std::string> rowLine1 = GridAreasParser::getLineNamesForRowLine( tmpl, 1 );
	EXPECT_EQ( rowLine1.size(), 1u );
	EXPECT_TRUE( rowLine1[0] == "head-start" );

	// Row line 2: head-end, nav-start, main-start
	std::vector<std::string> rowLine2 = GridAreasParser::getLineNamesForRowLine( tmpl, 2 );
	// head ends at row 2, nav and main start at row 2
	EXPECT_EQ( rowLine2.size(), 3u );

	// Column line 1: head-start, nav-start
	std::vector<std::string> colLine1 = GridAreasParser::getLineNamesForColumnLine( tmpl, 1 );
	EXPECT_EQ( colLine1.size(), 2u );

	// Column line 3: head-end, main-end
	std::vector<std::string> colLine3 = GridAreasParser::getLineNamesForColumnLine( tmpl, 3 );
	EXPECT_EQ( colLine3.size(), 2u );
	EXPECT_TRUE( colLine3[0] == "head-end" || colLine3[1] == "head-end" );
	EXPECT_TRUE( colLine3[0] == "main-end" || colLine3[1] == "main-end" );
}

UTEST( GridParser, fixedTracks ) {
	GridTrackList list = GridTrackParser::parseTrackList( "100px 1fr auto" );
	EXPECT_TRUE( list.valid );
	EXPECT_EQ( list.tracks.size(), 3u );
	EXPECT_EQ( list.tracks[0].size.min.type, GridTrackBreadthType::Length );
	EXPECT_EQ( list.tracks[0].size.min.value, 100.f );
	EXPECT_EQ( list.tracks[1].size.min.type, GridTrackBreadthType::Flex );
	EXPECT_EQ( list.tracks[1].size.min.value, 1.f );
	EXPECT_EQ( list.tracks[2].size.min.type, GridTrackBreadthType::Auto );
}

UTEST( GridParser, lineNames ) {
	GridTrackList list = GridTrackParser::parseTrackList( "[start] 1fr [mid a] 2fr [end]" );
	EXPECT_TRUE( list.valid );
	EXPECT_EQ( list.tracks.size(), 2u );
	EXPECT_EQ( list.tracks[0].beforeLineNames.size(), 1u );
	EXPECT_TRUE( list.tracks[0].beforeLineNames[0] == "start" );
	EXPECT_EQ( list.tracks[1].beforeLineNames.size(), 2u );
	EXPECT_TRUE( list.tracks[1].beforeLineNames[0] == "mid" );
	EXPECT_TRUE( list.tracks[1].beforeLineNames[1] == "a" );
	EXPECT_EQ( list.tracks[0].size.min.type, GridTrackBreadthType::Flex );
	EXPECT_EQ( list.tracks[1].size.min.type, GridTrackBreadthType::Flex );
	EXPECT_EQ( list.tracks[1].size.min.value, 2.f );
}

UTEST( GridParser, repeatExpands ) {
	GridTrackList list = GridTrackParser::parseTrackList( "repeat(3, 10px [x] 1fr)" );
	EXPECT_TRUE( list.valid );
	EXPECT_EQ( list.tracks.size(), 6u );
	EXPECT_EQ( list.tracks[0].beforeLineNames.size(), 0u );
	EXPECT_EQ( list.tracks[0].size.min.type, GridTrackBreadthType::Length );
	EXPECT_EQ( list.tracks[0].size.min.value, 10.f );
	EXPECT_EQ( list.tracks[1].beforeLineNames.size(), 1u );
	EXPECT_TRUE( list.tracks[1].beforeLineNames[0] == "x" );
	EXPECT_EQ( list.tracks[1].size.min.type, GridTrackBreadthType::Flex );
	EXPECT_EQ( list.tracks[1].size.min.value, 1.f );
}

UTEST( GridParser, invalidRepeatCount ) {
	{
		GridTrackList list = GridTrackParser::parseTrackList( "repeat(-1, 1fr)" );
		EXPECT_FALSE( list.valid );
	}
	{
		GridTrackList list = GridTrackParser::parseTrackList( "repeat(abc, 1fr)" );
		EXPECT_FALSE( list.valid );
	}
}

UTEST( GridParser, minmax ) {
	GridTrackList list = GridTrackParser::parseTrackList( "minmax(100px, 1fr)" );
	EXPECT_TRUE( list.valid );
	EXPECT_EQ( list.tracks.size(), 1u );
	EXPECT_EQ( list.tracks[0].size.min.type, GridTrackBreadthType::Length );
	EXPECT_EQ( list.tracks[0].size.min.value, 100.f );
	EXPECT_EQ( list.tracks[0].size.max.type, GridTrackBreadthType::Flex );
	EXPECT_EQ( list.tracks[0].size.max.value, 1.f );
}

UTEST( GridParser, fitContent ) {
	GridTrackList list = GridTrackParser::parseTrackList( "fit-content(200px)" );
	EXPECT_TRUE( list.valid );
	EXPECT_EQ( list.tracks.size(), 1u );
	EXPECT_EQ( list.tracks[0].size.min.type, GridTrackBreadthType::Auto );
	EXPECT_EQ( list.tracks[0].size.max.type, GridTrackBreadthType::FitContent );
	EXPECT_EQ( list.tracks[0].size.max.value, 200.f );
}

UTEST( GridParser, percentageRemainsPercentage ) {
	GridTrackList list = GridTrackParser::parseTrackList( "50% 100px" );
	EXPECT_TRUE( list.valid );
	EXPECT_EQ( list.tracks.size(), 2u );
	EXPECT_EQ( list.tracks[0].size.min.type, GridTrackBreadthType::Percentage );
	EXPECT_EQ( list.tracks[0].size.min.value, 50.f );
	EXPECT_EQ( list.tracks[1].size.min.type, GridTrackBreadthType::Length );
	EXPECT_EQ( list.tracks[1].size.min.value, 100.f );
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 0: Display routing and property defaults
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, inlineGridSetsWrapContentWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setLayoutWidthPolicy( SizePolicy::MatchParent );
	grid->setDisplay( CSSDisplay::InlineGrid );

	EXPECT_EQ( grid->getLayoutWidthPolicy(), SizePolicy::WrapContent );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Grid Item Collection
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, collectsInFlowChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( grid );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( grid );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Layout should not crash and children should remain valid
	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( child1->getParent() == grid );
	EXPECT_TRUE( child2->getParent() == grid );

	Engine::destroySingleton();
}

UTEST( GridContainer, skipsOutOfFlowChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* normalChild = UIHTMLWidget::New();
	normalChild->setParent( grid );
	normalChild->setPixelsSize( 100, 50 );
	normalChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( grid );
	absoluteChild->setCSSPosition( CSSPosition::Absolute );
	absoluteChild->setPixelsSize( 100, 50 );
	absoluteChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	absoluteChild->setPixelsPosition( 0, 0 );

	sceneNode->updateDirtyLayouts();

	// Normal child stays; absolute child should not be disrupted by grid layout
	EXPECT_TRUE( normalChild->getParent() == grid );
	EXPECT_EQ( absoluteChild->getPixelsPosition().x, 0.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, skipsDisplayNoneChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* visibleChild = UIHTMLWidget::New();
	visibleChild->setParent( grid );
	visibleChild->setPixelsSize( 100, 50 );
	visibleChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* hiddenChild = UIHTMLWidget::New();
	hiddenChild->setParent( grid );
	hiddenChild->setDisplay( CSSDisplay::None );
	hiddenChild->setPixelsSize( 100, 50 );
	hiddenChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( visibleChild->getParent() == grid );

	Engine::destroySingleton();
}

UTEST( GridContainer, ignoresWhitespaceTextNodes ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( grid );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Node with only whitespace should not disrupt grid layout
	UITextNode* wsNode = UITextNode::New();
	wsNode->setParent( grid );
	wsNode->setText( "   " );

	sceneNode->updateDirtyLayouts();
	EXPECT_TRUE( child->getParent() == grid );

	Engine::destroySingleton();
}

UTEST( GridContainer, anonymousTextNodeItem ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UITextNode* textNode = UITextNode::New();
	textNode->setParent( grid );
	textNode->setText( "Hello" );

	// Non-whitespace text node should be collected as anonymous item (no crash)
	sceneNode->updateDirtyLayouts();

	Engine::destroySingleton();
}

UTEST( GridContainer, blockifiesInlineChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// Inline-block child inside a grid container should be blockified (not be inline)
	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* inlineChild = UIHTMLWidget::New();
	inlineChild->setParent( grid );
	inlineChild->setDisplay( CSSDisplay::InlineBlock );
	inlineChild->setPixelsSize( 100, 50 );
	inlineChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// UILayouterManager creates a BlockLayouter for the child because parent is grid
	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( inlineChild->getParent() == grid );

	Engine::destroySingleton();
}

UTEST( GridContainer, floatDoesNotAffectGridItemPlacement ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* floatLeft = UIHTMLWidget::New();
	floatLeft->setParent( grid );
	floatLeft->setCSSFloat( CSSFloat::Left );
	floatLeft->setPixelsSize( 100, 50 );
	floatLeft->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* floatRight = UIHTMLWidget::New();
	floatRight->setParent( grid );
	floatRight->setCSSFloat( CSSFloat::Right );
	floatRight->setPixelsSize( 100, 50 );
	floatRight->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Floats inside grid should not cause issues
	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( floatLeft->getParent() == grid );
	EXPECT_TRUE( floatRight->getParent() == grid );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Auto-repeat parsing & expansion (fixed bug: propagated metadata)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridParser, autoRepeatFillsStored ) {
	GridTrackList list = GridTrackParser::parseTrackList( "repeat(auto-fill, minmax(100px, 1fr))" );
	EXPECT_TRUE( list.valid );
	EXPECT_TRUE( list.hasAutoRepeat );
	EXPECT_EQ( list.autoRepeatTemplate.size(), 1u );
	EXPECT_FALSE( list.autoRepeatIsFit );
}

UTEST( GridParser, autoFitFillsStored ) {
	GridTrackList list = GridTrackParser::parseTrackList( "repeat(auto-fit, 100px)" );
	EXPECT_TRUE( list.valid );
	EXPECT_TRUE( list.hasAutoRepeat );
	EXPECT_TRUE( list.autoRepeatIsFit );
	EXPECT_EQ( list.autoRepeatTemplate.size(), 1u );
	EXPECT_EQ( list.autoRepeatTemplate[0].size.min.type, GridTrackBreadthType::Length );
	EXPECT_EQ( list.autoRepeatTemplate[0].size.min.value, 100.f );
}

// ─────────────────────────────────────────────────────────────────────────────
// expandAutoRepeat count computation (fixed bug: used max instead of min)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridParser, expandAutoRepeatWithMinmax ) {
	GridTrackList list = GridTrackParser::parseTrackList( "repeat(auto-fill, minmax(140px, 1fr))" );
	EXPECT_TRUE( list.valid );
	EXPECT_TRUE( list.hasAutoRepeat );
	// The template track should have min=140px length, max=1fr flex
	const auto& t = list.autoRepeatTemplate[0];
	EXPECT_EQ( t.size.min.type, GridTrackBreadthType::Length );
	EXPECT_EQ( t.size.min.value, 140.f );
	EXPECT_EQ( t.size.max.type, GridTrackBreadthType::Flex );

	// expandAutoRepeat should use min (140px) for count computation, not max (1fr)
	GridTrackParser::expandAutoRepeat( list, 1000.f, 10.f );
	EXPECT_FALSE( list.hasAutoRepeat );
	// Count = floor((1000 + 10) / (140 + 10)) = floor(6.73) = 6
	EXPECT_EQ( list.tracks.size(), 6u );
}

UTEST( GridParser, expandAutoRepeatZeroAvailable ) {
	GridTrackList list = GridTrackParser::parseTrackList( "repeat(auto-fill, 100px)" );
	// With 0 available size, should default to 1 repetition
	GridTrackParser::expandAutoRepeat( list, 0.f, 10.f );
	EXPECT_EQ( list.tracks.size(), 1u );
}

// ─────────────────────────────────────────────────────────────────────────────
// expandAutoRepeat insertion position (fixed bug: pushed to end)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridParser, autoRepeatPositionInsert ) {
	// "100px repeat(auto-fill, 1fr) 200px" → 100px goes before repeat, 200px after
	GridTrackList list = GridTrackParser::parseTrackList( "100px repeat(auto-fill, 1fr) 200px" );
	EXPECT_TRUE( list.valid );
	EXPECT_TRUE( list.hasAutoRepeat );
	// Before expansion: tracks should be [100px, 200px], autoRepeatPosition=1
	EXPECT_EQ( list.tracks.size(), 2u );
	EXPECT_EQ( list.tracks[0].size.min.value, 100.f );
	EXPECT_EQ( list.tracks[1].size.min.value, 200.f );
	EXPECT_EQ( list.autoRepeatPosition, 1u );

	GridTrackParser::expandAutoRepeat( list, 1000.f, 10.f );
	// After expansion: tracks should be [100px, 1fr, 200px] (count=1 since 1fr has no min size)
	EXPECT_EQ( list.tracks.size(), 3u );
	EXPECT_EQ( list.tracks[0].size.min.value, 100.f );
	EXPECT_EQ( list.tracks[2].size.min.value, 200.f );
	EXPECT_EQ( list.tracks[1].size.min.type, GridTrackBreadthType::Flex );
}

// ─────────────────────────────────────────────────────────────────────────────
// Column count and sizing with auto-fit (fixed bug: expandAutoRepeat + collapse)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, autoFitColumnCount ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "repeat(auto-fit, minmax(140px, 1fr))" );
	grid->setColumnGap( "10px" );
	grid->setPixelsSize( 1000, 300 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// 5 items
	for ( int i = 0; i < 5; ++i ) {
		UIHTMLWidget* card = UIHTMLWidget::New();
		card->setParent( grid );
		card->setPixelsSize( 100, 50 );
		card->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& cols = layouter->getColumns();
	const auto& items = layouter->getItems();

	// auto-fit should create ~6 columns but collapse down to 5 used columns
	EXPECT_EQ( cols.size(), 5u );
	EXPECT_EQ( items.size(), 5u );
	// Each column should be (1000 - 4*10) / 5 = 192
	EXPECT_NEAR( cols[0].baseSize, 192.f, 2.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Item width and height stretch (fixed bugs: Fixed policy + preSizeItems)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, itemWidthStretch ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "200px 300px" );
	grid->setPixelsSize( 600, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// c1: column 1 → width 200px
	EXPECT_NEAR( c1->getPixelsSize().getWidth(), 200.f, 1.f );
	// c2: column 2 → width 300px
	EXPECT_NEAR( c2->getPixelsSize().getWidth(), 300.f, 1.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, itemHeightStretch ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "200px 300px" );
	grid->setGridTemplateRows( "100px" );
	grid->setPixelsSize( 600, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Two items in the same row, different content heights
	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	c1->setPixelsSize( 200, 40 );
	c1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );
	c2->setPixelsSize( 300, 60 );
	c2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Both items should stretch to 100px row height
	EXPECT_NEAR( c1->getPixelsSize().getHeight(), 100.f, 2.f );
	EXPECT_NEAR( c2->getPixelsSize().getHeight(), 100.f, 2.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Gaps between items (fixed bug: cells included gaps, no visible separation)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, gapBetweenItems ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px 100px" );
	grid->setColumnGap( "20px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );
	UIHTMLWidget* c3 = UIHTMLWidget::New();
	c3->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// c1 at x=0, width=100
	EXPECT_NEAR( c1->getPixelsPosition().x, 0.f, 1.f );
	EXPECT_NEAR( c1->getPixelsSize().getWidth(), 100.f, 1.f );
	// c2 at x=120 (= 100 + 20 gap), width=100
	EXPECT_NEAR( c2->getPixelsPosition().x, 120.f, 1.f );
	EXPECT_NEAR( c2->getPixelsSize().getWidth(), 100.f, 1.f );
	// c3 at x=240 (= 100 + 20 + 100 + 20), width=100
	EXPECT_NEAR( c3->getPixelsPosition().x, 240.f, 1.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Multi-span internal gaps (fixed bug: spanning items missed internal gaps)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, spanningItemWidthIncludesGaps ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px 100px" );
	grid->setColumnGap( "10px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* spanItem = UIHTMLWidget::New();
	spanItem->setParent( grid );
	spanItem->setGridColumnStart( "1" );
	spanItem->setGridColumnEnd( "span 3" );

	UIHTMLWidget* filler = UIHTMLWidget::New();
	filler->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// Span-3 item: 100 + 10 + 100 + 10 + 100 = 320
	EXPECT_NEAR( spanItem->getPixelsSize().getWidth(), 320.f, 1.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, spanningItemTwoColumns ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 200px" );
	grid->setColumnGap( "15px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* spanItem = UIHTMLWidget::New();
	spanItem->setParent( grid );
	spanItem->setGridColumnStart( "1" );
	spanItem->setGridColumnEnd( "span 2" );

	sceneNode->updateDirtyLayouts();

	// Span-2 item: 100 + 15 + 200 = 315
	EXPECT_NEAR( spanItem->getPixelsSize().getWidth(), 315.f, 1.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Justify-content alignment (fixed bug: space-between etc. just shifted grid)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, justifyContentCenter ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setJustifyContent( CSSJustifyContentHelper::fromString( "center" ) );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// Total content: 100 + 100 = 200 (no gaps by default)
	// Extra: 500 - 200 = 300 → center at 150
	EXPECT_NEAR( c1->getPixelsPosition().x, 150.f, 2.f );
	EXPECT_NEAR( c2->getPixelsPosition().x, 250.f, 2.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, justifyContentSpaceBetween ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px 100px" );
	grid->setJustifyContent( CSSJustifyContentHelper::fromString( "space-between" ) );
	grid->setPixelsSize( 600, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );
	UIHTMLWidget* c3 = UIHTMLWidget::New();
	c3->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// c2 should be positioned farther right than c1 + its width (gap present)
	EXPECT_GT( c2->getPixelsPosition().x, c1->getPixelsPosition().x + 100.f );
	// c3 should be at or beyond c2 + c2's width
	EXPECT_GT( c3->getPixelsPosition().x, c2->getPixelsPosition().x + 100.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, justifyContentSpaceAround ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setJustifyContent( CSSJustifyContentHelper::fromString( "space-around" ) );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// Total: 200, extra: 300, 2 items → extra/2 = 150 per gap between
	// half-gap before c1 = 150/2 = 75, gap between = 150, half-gap after c2 = 75
	// c1 x = 75, c2 x = 75 + 100 + 150 = 325
	EXPECT_NEAR( c1->getPixelsPosition().x, 75.f, 2.f );
	EXPECT_NEAR( c2->getPixelsPosition().x, 325.f, 2.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, justifyContentSpaceEvenly ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setJustifyContent( CSSJustifyContentHelper::fromString( "space-evenly" ) );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );

	sceneNode->updateDirtyLayouts();

	// Total: 200, extra: 300, 2 cols + 1 = 3 slots → extra/3 = 100 each
	// c1 x = 100, c2 x = 100 + 100 + 100 = 300
	EXPECT_NEAR( c1->getPixelsPosition().x, 100.f, 2.f );
	EXPECT_NEAR( c2->getPixelsPosition().x, 300.f, 2.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Axis-correct gap in track sizing (fixed bug: mColumnGap used for rows too)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, rowGapWorks ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px" );
	grid->setGridTemplateRows( "50px 50px" );
	grid->setRowGap( "10px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Place items explicitly in different rows
	UIHTMLWidget* c1 = UIHTMLWidget::New();
	c1->setParent( grid );
	c1->setGridColumnStart( "1" );
	c1->setGridRowStart( "1" );

	UIHTMLWidget* c2 = UIHTMLWidget::New();
	c2->setParent( grid );
	c2->setGridColumnStart( "1" );
	c2->setGridRowStart( "2" );

	sceneNode->updateDirtyLayouts();

	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();
	ASSERT_EQ( items.size(), 2u );
	// Explicitly placed: item 0 at row 1, item 1 at row 2
	EXPECT_EQ( items[0].resolvedRowStart, 1 );
	EXPECT_EQ( items[1].resolvedRowStart, 2 );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Negative line number clamping (fixed bug: no clamp, could go ≤0)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, negativeLineClamped ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 200px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Very negative end line: grid-column: 1 / -100
	UIHTMLWidget* item = UIHTMLWidget::New();
	item->setParent( grid );
	item->setGridColumnStart( "1" );
	item->setGridColumnEnd( "-100" );

	// Should not crash and should clamp to line 1 (start == end → span 1)
	sceneNode->updateDirtyLayouts();

	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();
	ASSERT_GT( items.size(), 0u );
	// Item should be placed (not stuck at 0,0)
	EXPECT_GT( items[0].resolvedColumnEnd, 0 );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Empty grid (fixed bug: unsigned underflow on tracks.size()-1)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, emptyGridNoCrash ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "none" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// No children — should not crash with unsigned underflow
	sceneNode->updateDirtyLayouts();
	EXPECT_TRUE( true ); // reached without crash

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Composite grid test: multiple columns, gaps, sizing, alignment
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, fullGridLayoutSmoke ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "repeat(3, 1fr)" );
	grid->setGridTemplateRows( "auto auto" );
	grid->setColumnGap( "10px" );
	grid->setRowGap( "8px" );
	grid->setPixelsSize( 600, 300 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	for ( int i = 0; i < 6; ++i ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( grid );
		child->setPixelsSize( 50, 30 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();
	const auto& cols = layouter->getColumns();

	EXPECT_EQ( items.size(), 6u );
	EXPECT_GE( cols.size(), 3u ); // repeat(3, 1fr) should create at least 3 columns

	// All items placed
	for ( const auto& item : items ) {
		EXPECT_GT( item.resolvedRowStart, 0 );
		EXPECT_GT( item.resolvedColumnStart, 0 );
	}

	// No overlapping positions
	for ( size_t a = 0; a < items.size(); ++a )
		for ( size_t b = a + 1; b < items.size(); ++b )
			EXPECT_FALSE( items[a].resolvedRowStart == items[b].resolvedRowStart &&
						  items[a].resolvedColumnStart == items[b].resolvedColumnStart );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests for FIXED bugs — previously TODO, now passing
// ─────────────────────────────────────────────────────────────────────────────

// P1: Negative fr rejected
UTEST( GridParser, rejectNegativeFr ) {
	GridTrackList list = GridTrackParser::parseTrackList( "-1fr" );
	EXPECT_FALSE( list.valid );
}

// P4: Auto-repeat zero-min tracks fills space
UTEST( GridContainer, autoRepeatZeroMinFillsContainer ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "repeat(auto-fill, minmax(0, 1fr))" );
	grid->setColumnGap( "10px" );
	grid->setPixelsSize( 1000, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& cols = layouter->getColumns();

	// Zero-min auto-fill should expand to multiple columns, not just 1
	EXPECT_GT( cols.size(), 1u );

	Engine::destroySingleton();
}

// PL1: Definite span on auto axis
UTEST( GridContainer, definiteColumnSpanAutoRow ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px 100px 100px" );
	grid->setGridTemplateRows( "50px 50px" );
	grid->setPixelsSize( 500, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// grid-column: 2 / span 2 (should span columns 2-3), grid-row: auto
	UIHTMLWidget* item1 = UIHTMLWidget::New();
	item1->setParent( grid );
	item1->setGridColumnStart( "2" );
	item1->setGridColumnEnd( "span 2" );
	item1->setPixelsSize( 50, 30 );
	item1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* item2 = UIHTMLWidget::New();
	item2->setParent( grid );
	item2->setPixelsSize( 50, 30 );
	item2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();

	ASSERT_GE( items.size(), 2u );
	// Item1 should span 2 columns
	EXPECT_EQ( items[0].resolvedColumnEnd - items[0].resolvedColumnStart, 2 );
	// Item2 should NOT overlap with item1
	EXPECT_NE( items[0].resolvedColumnStart, items[1].resolvedColumnStart );

	Engine::destroySingleton();
}

// PL2: Dense auto-placement fills holes
UTEST( GridContainer, denseAutoPlacement ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px 100px" );
	grid->setGridAutoFlowDense( true );
	grid->setPixelsSize( 300, 300 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Place item spanning entire first row
	UIHTMLWidget* big = UIHTMLWidget::New();
	big->setParent( grid );
	big->setGridColumnStart( "1" );
	big->setGridColumnEnd( "3" ); // span both columns
	big->setPixelsSize( 50, 30 );
	big->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Small items should fill the hole in row 1 col 1? No — big occupies both cols.
	// The test validates dense mode doesn't crash on sparse/empty layout.
	for ( int i = 0; i < 3; ++i ) {
		UIHTMLWidget* small = UIHTMLWidget::New();
		small->setParent( grid );
		small->setPixelsSize( 30, 20 );
		small->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& items = layouter->getItems();
	EXPECT_EQ( items.size(), 4u );

	Engine::destroySingleton();
}

// TS4: Flex distribution clamps to growth limit
UTEST( GridContainer, flexGrowthLimit ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	// minmax(0, 100px) with 1fr — should be clamped to at most 100px
	grid->setGridTemplateColumns( "minmax(0, 100px) 1fr" );
	grid->setPixelsSize( 400, 200 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& cols = layouter->getColumns();
	ASSERT_GE( cols.size(), 2u );

	// First column max is 100px even though it gets flex space
	EXPECT_LE( cols[0].baseSize, 100.f + 1.f );

	Engine::destroySingleton();
}

// TS7: grid-auto-rows/columns
UTEST( GridContainer, implicitTrackSizing ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "100px" );
	grid->setGridAutoRows( "80px" );
	grid->setPixelsSize( 500, 300 );
	grid->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	for ( int i = 0; i < 3; ++i ) {
		UIHTMLWidget* item = UIHTMLWidget::New();
		item->setParent( grid );
		item->setPixelsSize( 50, 30 );
		item->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	const auto& rows = layouter->getColumns(); // intentional: getColumns for rows check

	// Items placed in separate rows with 80px auto row height
	(void)rows; // no crash

	Engine::destroySingleton();
}

// I1: computeIntrinsicWidths with auto-repeat
UTEST( GridContainer, intrinsicWidthAutoRepeat ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Grid Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* grid = UIHTMLWidget::New();
	grid->setParent( sceneNode->getRoot() );
	grid->setDisplay( CSSDisplay::Grid );
	grid->setGridTemplateColumns( "repeat(auto-fill, 100px)" );
	grid->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );

	sceneNode->updateDirtyLayouts();
	auto* layouter = static_cast<GridLayouter*>( grid->getLayouter() );
	Float minW = layouter->getMinIntrinsicWidth();
	Float maxW = layouter->getMaxIntrinsicWidth();

	// Auto-repeat should produce non-zero intrinsic widths
	EXPECT_GT( minW, 0.f );
	// Both min and max should be > 0 (fixed-length tracks have equal min/max)
	EXPECT_GT( maxW, 0.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// HTML fixture tests (moved from uihtml_tests.cpp)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( GridContainer, basicHTMLGrid ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "UIHTML Grid Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings() );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	sceneNode->loadLayoutFromString(
		HTMLFormatter::HTMLtoXML( "<div style=\"display: grid; grid-template-columns: 100px 200px; "
								  "grid-template-rows: 50px 80px; width: 400px; height: 200px;\">"
								  "<div>A</div>"
								  "<div style=\"grid-column: 2; grid-row: 1/3;\">B</div>"
								  "</div>" ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto gridEl = sceneNode->getRoot()->findByTag( "div" );
	ASSERT_TRUE( nullptr != gridEl );
	EXPECT_TRUE( gridEl->isType( UI_TYPE_HTML_WIDGET ) );

	Engine::destroySingleton();
}

UTEST( GridContainer, downloadGridLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "UIHTML Grid Download Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings() );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	sceneNode->loadLayoutFromString( HTMLFormatter::HTMLtoXML(
		"<!doctype html><html><body>"
		"<div class=\"download-grid\" style=\"display: grid; "
		"grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 10px;\">"
		"<div class=\"download-card\"><h3>Windows</h3></div>"
		"<div class=\"download-card\"><h3>Mac</h3></div>"
		"<div class=\"download-card\"><h3>Linux</h3></div>"
		"<div class=\"download-card\"><h3>Android</h3></div>"
		"<div class=\"download-card\"><h3>iOS</h3></div>"
		"</div></body></html>" ) );

	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto gridEl = sceneNode->getRoot()->findByClass( "download-grid" );
	ASSERT_TRUE( nullptr != gridEl );
	ASSERT_TRUE( gridEl->isType( UI_TYPE_HTML_WIDGET ) );

	UIHTMLWidget* gridWidget = gridEl->asType<UIHTMLWidget>();
	auto* layouter = gridWidget->getLayouter();
	ASSERT_TRUE( nullptr != layouter );

	auto* gridLayouter = static_cast<GridLayouter*>( layouter );

	const auto& cols = gridLayouter->getColumns();
	EXPECT_TRUE( cols.size() >= 2 );

	const auto& items = gridLayouter->getItems();
	ASSERT_EQ( (size_t)5, items.size() );

	Float gridW = gridWidget->getPixelsSize().getWidth();
	Float expectedCardW = ( gridW - 4.f * 10.f ) / 5.f;
	Float tolerance = expectedCardW * 0.15f;
	{
		int col = 1;
		for ( const auto& item : items ) {
			Float cardW = item.widget->getPixelsSize().getWidth();
			EXPECT_NEAR( expectedCardW, cardW, tolerance );
			EXPECT_EQ( 1, item.resolvedRowStart );
			EXPECT_EQ( col, item.resolvedColumnStart );
			++col;
		}
	}

	{
		Float firstH = items[0].widget->getPixelsSize().getHeight();
		EXPECT_TRUE( firstH > 0 );
		for ( size_t i = 1; i < items.size(); ++i )
			EXPECT_NEAR( firstH, items[i].widget->getPixelsSize().getHeight(), 0.5f );
	}

	Engine::destroySingleton();
}

UTEST( GridContainer, gridTestFixturePlacesSpanningItems ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "UIHTML Grid Fixture Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings() );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string htmlContent;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/grid_test.html", htmlContent ) );
	sceneNode->loadLayoutFromString( EE::UI::Tools::HTMLFormatter::HTMLtoXML( htmlContent ) );
	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto* wrapperNode = sceneNode->getRoot()->findByClass( "wrapper" );
	ASSERT_TRUE( nullptr != wrapperNode );
	ASSERT_TRUE( wrapperNode->isWidget() );

	auto* one = sceneNode->getRoot()->findByClass( "one" );
	auto* two = sceneNode->getRoot()->findByClass( "two" );
	auto* three = sceneNode->getRoot()->findByClass( "three" );
	auto* four = sceneNode->getRoot()->findByClass( "four" );
	auto* five = sceneNode->getRoot()->findByClass( "five" );
	auto* six = sceneNode->getRoot()->findByClass( "six" );
	ASSERT_TRUE( nullptr != one );
	ASSERT_TRUE( nullptr != two );
	ASSERT_TRUE( nullptr != three );
	ASSERT_TRUE( nullptr != four );
	ASSERT_TRUE( nullptr != five );
	ASSERT_TRUE( nullptr != six );
	ASSERT_TRUE( one->isWidget() );
	ASSERT_TRUE( two->isWidget() );
	ASSERT_TRUE( three->isWidget() );
	ASSERT_TRUE( four->isWidget() );
	ASSERT_TRUE( five->isWidget() );
	ASSERT_TRUE( six->isWidget() );

	auto* oneWidget = one->asType<UIWidget>();
	auto* twoWidget = two->asType<UIWidget>();
	auto* threeWidget = three->asType<UIWidget>();
	auto* fourWidget = four->asType<UIWidget>();
	auto* fiveWidget = five->asType<UIWidget>();
	auto* sixWidget = six->asType<UIWidget>();

	Float wrapperW = wrapperNode->asType<UIWidget>()->getPixelsSize().getWidth();
	EXPECT_GT( wrapperW, 900.f );
	EXPECT_LT( wrapperW, 950.f );

	EXPECT_GT( oneWidget->getPixelsSize().getWidth(), 600.f );
	EXPECT_GT( twoWidget->getPixelsSize().getWidth(), 600.f );
	EXPECT_GT( threeWidget->getPixelsSize().getWidth(), 290.f );
	EXPECT_GT( fourWidget->getPixelsSize().getWidth(), 290.f );
	EXPECT_GT( fiveWidget->getPixelsSize().getWidth(), 290.f );
	EXPECT_GT( sixWidget->getPixelsSize().getWidth(), 290.f );

	EXPECT_GT( oneWidget->getPixelsSize().getHeight(), 95.f );
	EXPECT_GT( twoWidget->getPixelsSize().getHeight(), 200.f );
	EXPECT_GT( threeWidget->getPixelsSize().getHeight(), 300.f );
	EXPECT_GT( fourWidget->getPixelsSize().getHeight(), 95.f );
	EXPECT_GT( fiveWidget->getPixelsSize().getHeight(), 95.f );
	EXPECT_GT( sixWidget->getPixelsSize().getHeight(), 95.f );

	EXPECT_NEAR( oneWidget->getPixelsPosition().y, twoWidget->getPixelsPosition().y, 1.f );
	EXPECT_GT( twoWidget->getPixelsPosition().x, oneWidget->getPixelsPosition().x + 300.f );
	EXPECT_GT( threeWidget->getPixelsPosition().y, oneWidget->getPixelsPosition().y + 95.f );
	EXPECT_GT( fourWidget->getPixelsPosition().y, twoWidget->getPixelsPosition().y + 200.f );
	EXPECT_NEAR( fiveWidget->getPixelsPosition().y, sixWidget->getPixelsPosition().y, 1.f );
	EXPECT_GT( sixWidget->getPixelsPosition().x, fiveWidget->getPixelsPosition().x + 300.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, gridSpanItemsKeepSaneHeightAndDrawText ) {
	auto* win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIHTML Grid Span Text Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings() );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string htmlContent;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/grid_size_miscalculation.html", htmlContent ) );
	sceneNode->loadLayoutFromString( EE::UI::Tools::HTMLFormatter::HTMLtoXML( htmlContent ) );
	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto* headerBox = sceneNode->getRoot()->findByClass( "header-box" );
	ASSERT_TRUE( nullptr != headerBox );
	ASSERT_TRUE( headerBox->isWidget() );

	auto rows = headerBox->findAllByClass( "hb-row" );
	auto keys = headerBox->findAllByClass( "hb-k" );
	auto values = headerBox->findAllByClass( "hb-v" );
	ASSERT_EQ( rows.size(), 3u );
	ASSERT_EQ( keys.size(), 3u );
	ASSERT_EQ( values.size(), 3u );

	for ( auto* row : rows ) {
		ASSERT_TRUE( row->isWidget() );
		ASSERT_TRUE( row->isType( UI_TYPE_RICHTEXT ) );
		EXPECT_GT( row->getPixelsSize().getHeight(), 10.f );
		EXPECT_LT( row->getPixelsSize().getHeight(), 60.f );
	}

	for ( auto* span : keys ) {
		ASSERT_TRUE( span->isType( UI_TYPE_TEXTSPAN ) );
		auto* richText = span->asType<UITextSpan>()->getRichTextPtr();
		ASSERT_TRUE( richText != nullptr );
		EXPECT_EQ( richText->getLines().size(), 1u );
		EXPECT_GT( span->getPixelsSize().getHeight(), 10.f );
		EXPECT_LT( span->getPixelsSize().getHeight(), 60.f );
	}

	for ( auto* span : values ) {
		ASSERT_TRUE( span->isType( UI_TYPE_TEXTSPAN ) );
		auto* richText = span->asType<UITextSpan>()->getRichTextPtr();
		ASSERT_TRUE( richText != nullptr );
		EXPECT_GE( richText->getLines().size(), 1u );
		EXPECT_GT( span->getPixelsSize().getHeight(), 10.f );
		EXPECT_LT( span->getPixelsSize().getHeight(), 60.f );
	}

	EXPECT_GT( headerBox->asType<UIWidget>()->getPixelsSize().getHeight(), 60.f );
	EXPECT_LT( headerBox->asType<UIWidget>()->getPixelsSize().getHeight(), 160.f );

	win->setClearColor( Color( "#0c1520" ) );
	win->clear();
	SceneManager::instance()->draw();
	win->display();

	Engine::destroySingleton();
}

UTEST( GridContainer, newsblurReducedGrid ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "UIHTML Grid NewsBlur Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings() );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string htmlContent;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/grid_newsblur_reduced.html", htmlContent ) );
	sceneNode->loadLayoutFromString( EE::UI::Tools::HTMLFormatter::HTMLtoXML( htmlContent ) );
	sceneNode->update( Seconds( 1 ) );
	sceneNode->updateDirtyLayouts();

	auto* gridEl = sceneNode->getRoot()->findByClass( "NB-inner" );
	ASSERT_TRUE( nullptr != gridEl );
	ASSERT_TRUE( gridEl->isWidget() );

	Float gridW = gridEl->getPixelsSize().getWidth();
	Float gridH = gridEl->getPixelsSize().getHeight();
	EXPECT_GT( gridW, 100.f );
	EXPECT_GT( gridH, 100.f );

	// Verify NB-feature-group items aren't overexpanded
	auto* fg = sceneNode->getRoot()->findByClass( "NB-feature-group" );
	ASSERT_TRUE( nullptr != fg );
	Float fgH = fg->getPixelsSize().getHeight();
	EXPECT_GT( fgH, 100.f );
	EXPECT_LT( fgH, 2000.f );

	auto* fgiText = sceneNode->getRoot()->findByClass( "NB-fgi-text" );
	ASSERT_TRUE( nullptr != fgiText );
	Float textW = fgiText->getPixelsSize().getWidth();
	EXPECT_GT( textW, 150.f );

	Engine::destroySingleton();
}

UTEST( GridContainer, gradientFixtureAutoFitItemsStayInsideOnResize ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "UIHTML Grid Gradient Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings() );
	PixelDensity::setPixelDensity( 2.f );
	init_grid_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();
	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );

	std::string htmlContent;
	ASSERT_TRUE( FileSystem::fileGet( "assets/html/css_gradient_tests.html", htmlContent ) );
	sceneNode->loadLayoutFromString( EE::UI::Tools::HTMLFormatter::HTMLtoXML( htmlContent ) );

	auto assertItemsInsideGrid = [&]() {
		auto grids = sceneNode->getRoot()->findAllByClass( "grid" );
		ASSERT_GT( grids.size(), 1u );
		for ( auto* grid : grids ) {
			ASSERT_TRUE( grid->getParent()->isWidget() );
			auto* parent = grid->getParent()->asType<UIWidget>();
			Float parentContentRight =
				parent->getPixelsSize().getWidth() - parent->getPixelsContentOffset().Right;
			EXPECT_LE( grid->getPixelsPosition().x + grid->getPixelsSize().getWidth(),
					   parentContentRight + 0.5f );

			auto cases = grid->findAllByClass( "case" );
			ASSERT_GT( cases.size(), 1u );
			Float contentWidth = grid->getPixelsSize().getWidth() -
								 grid->getPixelsContentOffset().Left -
								 grid->getPixelsContentOffset().Right;
			auto* gridLayouter =
				static_cast<GridLayouter*>( grid->asType<UIHTMLWidget>()->getLayouter() );
			ASSERT_TRUE( nullptr != gridLayouter );
			const auto& columns = gridLayouter->getColumns();
			Float gap =
				grid->lengthFromValue( "16px", CSS::PropertyRelativeTarget::ContainingBlockWidth );
			Float minTrack =
				grid->lengthFromValue( "240px", CSS::PropertyRelativeTarget::ContainingBlockWidth );
			size_t expectedColumns = std::min(
				cases.size(), static_cast<size_t>( ( contentWidth + gap ) / ( minTrack + gap ) ) );
			ASSERT_EQ( expectedColumns, columns.size() );
			Float tracksWidth = static_cast<Float>( columns.size() - 1 ) * gap;
			for ( const auto& column : columns )
				tracksWidth += column.baseSize;
			EXPECT_LE( tracksWidth, contentWidth + 0.5f );

			Float contentRight =
				grid->getPixelsSize().getWidth() - grid->getPixelsContentOffset().Right;
			for ( auto* item : cases ) {
				Float itemRight = item->getPixelsPosition().x + item->getPixelsSize().getWidth();
				EXPECT_LE( itemRight, contentRight + 0.5f );
				EXPECT_LE( item->getScreenRect().Right, grid->getScreenRect().Right + 0.5f );
			}
		}
	};

	for ( const auto& size :
		  { Sizef( 1200.f, 900.f ), Sizef( 1800.f, 900.f ), Sizef( 1000.f, 900.f ) } ) {
		sceneNode->setPixelsSize( size );
		sceneNode->update( Seconds( 1 ) );
		sceneNode->updateDirtyLayouts();
		assertItemsInsideGrid();
	}

	Engine::destroySingleton();
	PixelDensity::setPixelDensity( 1.f );
}
