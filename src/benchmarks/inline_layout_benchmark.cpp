#include "../tests/unit_tests/utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/richtext.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uimarkdownview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>

#include <cstdlib>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::Window;

static constexpr int numBoxes = 100;
static constexpr int numSpansPerBox = 20;
static constexpr int layoutIterations = 50;
static constexpr int markdownFlushIterations = 1;
static constexpr int selectorMatchingIterations = 20000;
static constexpr Float maxWidth = 800;

static int getSelectorMatchingIterations() {
	if ( const char* env = std::getenv( "EE_CSS_SELECTOR_BENCH_ITERATIONS" ) ) {
		Int32 val = selectorMatchingIterations;
		if ( String::fromString( val, std::string( env ) ) )
			return eemax<Int32>( 1, val );
	}
	return selectorMatchingIterations;
}

UTEST( Benchmark, CSSSelectorMatching ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "CSS selector bench",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );

	UIWidget* widget = UIWidget::NewWithTag( "div" );
	widget->setId( "selector-benchmark-target" );
	widget->setClasses( { "button", "primary", "toolbar-item", "interactive", "selected", "compact",
						  "theme-light", "enabled" } );
	widget->setParent( sceneNode->getRoot() );

	static constexpr int selectorCount = 1024;
	const int matchingIterations = getSelectorMatchingIterations();
	std::vector<StyleSheetSelector> selectors;
	selectors.reserve( selectorCount );

	Clock constructionClock;
	for ( int i = 0; i < selectorCount; ++i ) {
		if ( i % 16 == 0 )
			selectors.emplace_back( "div.button.primary" );
		else if ( i % 16 == 1 )
			selectors.emplace_back( "#selector-benchmark-target.toolbar-item" );
		else
			selectors.emplace_back( ".unmatched-class-" + String::toString( i ) );
	}
	const Time constructionElapsed = constructionClock.getElapsedTime();

	Uint64 matchCount = 0;
	Clock matchingClock;
	for ( int iteration = 0; iteration < matchingIterations; ++iteration ) {
		for ( const auto& selector : selectors )
			matchCount += selector.select( widget, false );
	}
	const Time matchingElapsed = matchingClock.getElapsedTime();

	EXPECT_EQ( static_cast<Uint64>( matchingIterations ) * 128u, matchCount );
	UTEST_PRINT_INFO( String::format( "UIWidget size: %zu bytes", sizeof( UIWidget ) ).c_str() );
	UTEST_PRINT_INFO(
		String::format( "StyleSheetSelectorRule size: %zu bytes", sizeof( StyleSheetSelectorRule ) )
			.c_str() );
	UTEST_PRINT_INFO(
		String::format( "Selector construction: %lld us", constructionElapsed.asMicroseconds() )
			.c_str() );
	UTEST_PRINT_INFO(
		String::format( "Selector matching: %lld us", matchingElapsed.asMicroseconds() ).c_str() );
	UTEST_PRINT_INFO(
		String::format( "Selector calls: %d", selectorCount * matchingIterations ).c_str() );

	Engine::destroySingleton();
}

UTEST( Benchmark, CSSClassIndexLookup ) {
	Engine::instance()->createWindow( WindowSettings( 800, 600, "CSS class index bench",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ) );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );

	UIWidget* widget = UIWidget::NewWithTag( "div" );
	widget->setClasses( { "button", "primary", "toolbar-item" } );
	widget->setParent( sceneNode->getRoot() );

	static constexpr int selectorCount = 1024;
	std::string css;
	for ( int i = 0; i < selectorCount; ++i ) {
		const std::string selector =
			i % 16 == 0 ? ".button" : ".unmatched-class-" + String::toString( i );
		css += selector + " { width: " + String::toString( i + 1 ) + "px; }\n";
	}

	StyleSheetParser parser;
	Clock constructionClock;
	ASSERT_TRUE( parser.loadFromString( css ) );
	const Time constructionElapsed = constructionClock.getElapsedTime();

	const int matchingIterations = getSelectorMatchingIterations();
	Uint64 legacyMatchCount = 0;
	Clock legacyMatchingClock;
	for ( int iteration = 0; iteration < matchingIterations; ++iteration ) {
		for ( const auto& style : parser.getStyleSheet().getStyles() ) {
			if ( style->isMediaValid() && style->getSelector().select( widget, false ) )
				++legacyMatchCount;
		}
	}
	const Time legacyMatchingElapsed = legacyMatchingClock.getElapsedTime();

	Uint64 matchCount = 0;
	Clock matchingClock;
	for ( int iteration = 0; iteration < matchingIterations; ++iteration ) {
		auto definition = parser.getStyleSheet().getElementStyles( widget, false );
		matchCount += definition ? definition->getStyles().size() : 0;
	}
	const Time matchingElapsed = matchingClock.getElapsedTime();

	EXPECT_EQ( static_cast<Uint64>( matchingIterations ) * 64u, legacyMatchCount );
	EXPECT_EQ( legacyMatchCount, matchCount );
	UTEST_PRINT_INFO(
		String::format( "Stylesheet construction: %lld us", constructionElapsed.asMicroseconds() )
			.c_str() );
	UTEST_PRINT_INFO(
		String::format( "Legacy full scan: %lld us", legacyMatchingElapsed.asMicroseconds() )
			.c_str() );
	UTEST_PRINT_INFO(
		String::format( "Class index lookup: %lld us", matchingElapsed.asMicroseconds() ).c_str() );
	UTEST_PRINT_INFO( String::format( "Stylesheet lookups: %d", matchingIterations ).c_str() );

	Engine::destroySingleton();
}

static int getMarkdownFlushIterations() {
	if ( const char* env = std::getenv( "EE_MARKDOWN_BENCH_FLUSH_ITERATIONS" ) ) {
		Int32 val = markdownFlushIterations;
		if ( String::fromString( val, std::string( env ) ) )
			return eemax<Int32>( 0, val );
	}
	return markdownFlushIterations;
}

UTEST( Benchmark, InlineLayout ) {
	Engine::instance()->createWindow( WindowSettings(
		800, 600, "bench", WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	if ( !font->loaded() ) {
		Engine::destroySingleton();
		UTEST_PRINT_INFO( "Failed to load font, skipping benchmark" );
		return;
	}
	FontFamily::loadFromRegular( font );

	RichText rt;
	rt.getFontStyleConfig().Font = font;
	rt.getFontStyleConfig().CharacterSize = 12;
	rt.setMaxWidth( maxWidth );

	String lorem = "Lorem ipsum dolor sit amet consectetur adipiscing elit sed do "
				   "eiusmod tempor incididunt ut labore et dolore magna aliqua ";
	String boldText = " **bold** ";

	for ( int b = 0; b < numBoxes; ++b ) {
		rt.pushInlineBox( { 0, 0, 0, 0 }, { 4, 0, 4, 0 }, 0,
						  RichText::BaselineAlignValue( RichText::BaselineAlignment::Top ) );
		for ( int s = 0; s < numSpansPerBox; ++s ) {
			rt.addInlineText( lorem, rt.getFontStyleConfig(), { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0,
							  RichText::BaselineAlignValue() );
			if ( s % 3 == 0 ) {
				FontStyleConfig boldStyle = rt.getFontStyleConfig();
				boldStyle.Style = Text::Bold;
				rt.addInlineText( boldText, boldStyle, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0,
								  RichText::BaselineAlignValue() );
			}
		}
		rt.popInlineBox();
		rt.addInlineText( "\n", rt.getFontStyleConfig(), { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0,
						  RichText::BaselineAlignValue() );
	}

	rt.updateLayout();
	size_t lineCount = rt.getLines().size();
	size_t fragmentCount = rt.getInlineFragments().size();

	Clock clock;
	for ( int i = 0; i < layoutIterations; ++i ) {
		rt.invalidateLayout();
		rt.updateLayout();
	}
	Time elapsed = clock.getElapsedTime();

	UTEST_PRINT_INFO( String::format( "Boxes: %d, spans/box: %d, iterations: %d", numBoxes,
									  numSpansPerBox, layoutIterations )
						  .c_str() );
	UTEST_PRINT_INFO(
		String::format( "Lines: %zu, fragments: %zu", lineCount, fragmentCount ).c_str() );
	UTEST_PRINT_INFO( String::format( "Total: %s", elapsed.toString().c_str() ).c_str() );
	UTEST_PRINT_INFO(
		String::format( "Per iteration: %lld us", elapsed.asMicroseconds() / layoutIterations )
			.c_str() );

	Engine::destroySingleton();
}

UTEST( Benchmark, MarkdownReadme ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	const std::string readmePath = "../../README.md";
	std::string markdown;
	if ( !FileSystem::fileGet( readmePath, markdown ) ) {
		UTEST_PRINT_INFO( String::format( "Failed to load %s from cwd '%s', skipping benchmark",
										  readmePath.c_str(),
										  FileSystem::getCurrentWorkingDirectory().c_str() )
							  .c_str() );
		return;
	}

	EE::Window::Window* window = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, "markdown bench", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ) );
	if ( !window || !window->isOpen() ) {
		Engine::destroySingleton();
		UTEST_PRINT_INFO( "Failed to create window, skipping benchmark" );
		return;
	}
	Engine::instance()->disableSharedGLContext();

	PixelDensity::setPixelDensity( 1.f );
	UISceneNode* ui = UISceneNode::New( window );
	SceneManager::instance()->add( ui );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontTrueType* monoFont = FontTrueType::New( "monospace" );
	monoFont->loadFromFile( "../assets/fonts/DejaVuSansMono.ttf" );
	if ( !font->loaded() || !monoFont->loaded() ) {
		Engine::destroySingleton();
		UTEST_PRINT_INFO( "Failed to load fonts, skipping benchmark" );
		return;
	}
	monoFont->setEnableDynamicMonospace( true );
	FontFamily::loadFromRegular( font );
	FontFamily::loadFromRegular( monoFont );
	ui->getUIThemeManager()->setDefaultFont( font );

	UIScrollView* scrollView = UIScrollView::New();
	scrollView->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	scrollView->setParent( ui->getRoot() );

	UIMarkdownView* markdownView = UIMarkdownView::New();
	markdownView->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	markdownView->setPadding( Rectf( 16, 16, 16, 16 ) );
	markdownView->setParent( scrollView->getContainer() );

	UILayout::resetMetrics();

	Clock loadClock;
	markdownView->loadFromString( markdown );
	Time loadElapsed = loadClock.getElapsedTime();
	UILayout::Metrics afterLoad = UILayout::getMetrics();

	int flushIterations = getMarkdownFlushIterations();
	Clock updateClock;
	for ( int i = 0; i < flushIterations; ++i )
		ui->update( Seconds( 0 ) );
	Time updateElapsed = updateClock.getElapsedTime();
	UILayout::Metrics afterUpdate = UILayout::getMetrics();

	if ( flushIterations > 0 )
		EXPECT_GT( markdownView->getPixelsSize().getHeight(), 0.f );

	UTEST_PRINT_INFO( String::format( "README path: %s", readmePath.c_str() ).c_str() );
	UTEST_PRINT_INFO( String::format( "README bytes: %zu", markdown.size() ).c_str() );
	UTEST_PRINT_INFO( String::format( "Flush iterations: %d", flushIterations ).c_str() );
	UTEST_PRINT_INFO( String::format( "Load: %s", loadElapsed.toString().c_str() ).c_str() );
	UTEST_PRINT_INFO(
		String::format( "Layout flush: %s", updateElapsed.toString().c_str() ).c_str() );
	UTEST_PRINT_INFO(
		String::format( "After load - child changes: %llu, auto-size children: %llu, rich text "
						"rebuilds: %llu, invalidations: %llu, sync updates: %llu, tree updates: "
						"%llu",
						(unsigned long long)afterLoad.childCountChanges,
						(unsigned long long)afterLoad.autoSizeChildren,
						(unsigned long long)afterLoad.richTextRebuilds,
						(unsigned long long)afterLoad.invalidations,
						(unsigned long long)afterLoad.synchronousUpdates,
						(unsigned long long)afterLoad.treeUpdates )
			.c_str() );
	UTEST_PRINT_INFO(
		String::format( "After flush - child changes: %llu, auto-size children: %llu, rich text "
						"rebuilds: %llu, invalidations: %llu, sync updates: %llu, tree updates: "
						"%llu",
						(unsigned long long)afterUpdate.childCountChanges,
						(unsigned long long)afterUpdate.autoSizeChildren,
						(unsigned long long)afterUpdate.richTextRebuilds,
						(unsigned long long)afterUpdate.invalidations,
						(unsigned long long)afterUpdate.synchronousUpdates,
						(unsigned long long)afterUpdate.treeUpdates )
			.c_str() );

	UILayout::setMetricsEnabled( false );
	SceneManager::instance()->remove( ui );
	eeDelete( ui );
	Engine::destroySingleton();
}
