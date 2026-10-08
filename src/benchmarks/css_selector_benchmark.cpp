#include "../tests/unit_tests/utest.hpp"

#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>

using namespace EE;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::Window;

namespace {

constexpr size_t SampleCount = 5;

int selectorIterations() {
	Int32 iterations = 20000;
	if ( const char* env = std::getenv( "EE_CSS_SELECTOR_BENCH_ITERATIONS" ) ) {
		String::fromString( iterations, std::string( env ) );
	}
	return eemax<Int32>( 1, iterations );
}

UIWidget* appendElement( UIWidget* parent, const char* tag ) {
	auto* element = UIWidget::NewWithTag( tag );
	element->setParent( parent );
	return element;
}

// Construction, widget setup, and warmup are outside the measured loops. Keep the checksum
// observable and report the median rather than asserting machine-dependent timing thresholds.
template <typename Match>
double medianNanoseconds( Match&& match, int iterations, Uint64& checksum ) {
	for ( int i = 0; i < 256; ++i )
		checksum += match();
	std::array<double, SampleCount> samples;
	for ( auto& sample : samples ) {
		Clock clock;
		for ( int i = 0; i < iterations; ++i )
			checksum += match();
		sample = static_cast<double>( clock.getElapsedTime().asMicroseconds() ) * 1000 / iterations;
	}
	std::sort( samples.begin(), samples.end() );
	return samples[SampleCount / 2];
}

} // namespace

UTEST( Benchmark, CSSSelectorCombinators ) {
	ASSERT_TRUE(
		Engine::instance()->createWindow( WindowSettings(
			320, 240, "CSS selector combinators", WindowStyle::Default | WindowStyle::Hidden,
			WindowBackend::Default, 32, {}, 1, false, true ) ) != nullptr );
	auto* scene = UISceneNode::New();
	SceneManager::instance()->add( scene );
	const int iterations = selectorIterations();
	Uint64 checksum = 0;
	auto runCase = [&]( const char* label, const char* name, UIWidget* target, bool expected,
						bool applyPseudo = true, size_t expectedRelated = 0 ) {
		StyleSheetSelector selector( name );
		EXPECT_EQ( expected, selector.select( target, applyPseudo ) );
		EXPECT_EQ( expectedRelated, selector.getRelatedElements( target, applyPseudo ).size() );
		const double selectNs = medianNanoseconds(
			[&] { return selector.select( target, applyPseudo ); }, iterations, checksum );
		const double relatedNs = medianNanoseconds(
			[&] { return selector.getRelatedElements( target, applyPseudo ).size(); }, iterations,
			checksum );
		UTEST_PRINT_INFO( String::format( "%s: select %.1f ns/call; related %.1f ns/call; %s",
										  label, selectNs, relatedNs, name )
							  .c_str() );
	};

	// The nested table offers a nearer tr:first-child that cannot satisfy the outer child chain.
	auto* body = appendElement( scene->getRoot(), "body" );
	auto* center = appendElement( body, "center" );
	auto* table = appendElement( center, "table" );
	auto* tbody = appendElement( table, "tbody" );
	auto* row = appendElement( tbody, "tr" );
	auto* cell = appendElement( row, "td" );
	auto* innerTable = appendElement( cell, "table" );
	auto* innerBody = appendElement( innerTable, "tbody" );
	auto* innerRow = appendElement( innerBody, "tr" );
	auto* innerCell = appendElement( innerRow, "td" );
	auto* span = appendElement( innerCell, "span" );
	auto* link = appendElement( span, "a" );
	auto* shallowLink = appendElement( appendElement( cell, "span" ), "a" );
	auto* outside = appendElement( appendElement( appendElement( tbody, "tr" ), "td" ), "a" );
	constexpr const char* HNSelector = "body > center > table > tbody > tr:first-child * a:hover";
	runCase( "HN inactive hover", HNSelector, link, false );
	runCase( "HN structural lookup", HNSelector, link, true, false, 1 );
	scene->getEventDispatcher()->setMouseOverNode( link );
	link->pushState( UIState::StateHover );
	runCase( "HN hovered", HNSelector, link, true, true, 1 );
	scene->getEventDispatcher()->setMouseOverNode( shallowLink );
	shallowLink->pushState( UIState::StateHover );
	runCase( "HN without retry", HNSelector, shallowLink, true, true, 1 );
	runCase( "HN rightmost tag mismatch", HNSelector, cell, false );
	scene->getEventDispatcher()->setMouseOverNode( outside );
	outside->pushState( UIState::StateHover );
	runCase( "HN outside first row", HNSelector, outside, false );
	scene->getEventDispatcher()->setMouseOverNode( nullptr );

	for ( int depth : { 8, 24, 64 } ) {
		auto* root = appendElement( scene->getRoot(), "div" );
		root->setId( "scope" );
		auto* tail = root;
		for ( int i = 1; i < depth; ++i )
			tail = appendElement( tail, "div" );
		auto* target = appendElement( tail, "a" );
		const auto label = String::format( "depth %d", depth );
		runCase( label.c_str(), "a", target, true );
		runCase( label.c_str(), "div > div > a", target, true );
		runCase( label.c_str(), "div div div div div div a", target, true );
		runCase( label.c_str(), "#scope > * a", target, true );
		runCase( label.c_str(), "missing div div div div div div a", target, false );
		runCase( label.c_str(), "missing > div div div div div div a", target, false );
	}

	// Only the farthest .candidate has #start immediately before it.
	auto* siblings = appendElement( scene->getRoot(), "div" );
	appendElement( siblings, "span" )->setId( "start" );
	for ( int i = 0; i < 64; ++i ) {
		appendElement( siblings, "div" )->addClass( "candidate" );
		appendElement( siblings, "span" );
	}
	auto* target = appendElement( siblings, "a" );
	runCase( "64 sibling candidates", "#start + .candidate ~ a", target, true );
	runCase( "64 sibling candidates", "missing .candidate ~ a", target, false );

	// Checkpoint storage sizes: 16, 32 and 64 entries on the stack, then the heap. Each "* > div"
	// pair keeps one checkpoint and consumes two levels of the 160-level chain.
	auto* deepTail = appendElement( scene->getRoot(), "div" );
	deepTail->setId( "deep" );
	for ( int i = 0; i < 160; ++i )
		deepTail = appendElement( deepTail, "div" );
	auto* deepTarget = appendElement( deepTail, "a" );
	for ( int checkpoints : { 16, 17, 32, 33, 64, 65 } ) {
		std::string body;
		for ( int i = 0; i < checkpoints; ++i )
			body += "* > div ";
		const auto label = String::format( "%d checkpoints", checkpoints );
		const std::string match = "#deep > " + body + "a";
		const std::string reject = "#deep > " + body + "span";
		runCase( label.c_str(), match.c_str(), deepTarget, true );
		runCase( label.c_str(), reject.c_str(), deepTarget, false );
	}
	UTEST_PRINT_INFO( String::format( "%d iterations/sample; %zu samples; checksum %llu",
									  iterations, SampleCount,
									  static_cast<unsigned long long>( checksum ) )
						  .c_str() );
	EXPECT_TRUE( checksum > 0 );
	Engine::destroySingleton();
}

// Dependency collection is what UIStyle::subscribeNonCacheableStyles() runs for each subject, with
// pseudo-classes ignored. Cases cover the immediate exit, the shared-prefix fast path, and the
// union collector. The scene cases report the resulting subscriptions and the cost they add to
// style loading and to repeated hover changes.
UTEST( Benchmark, CSSStateDependencies ) {
	ASSERT_TRUE( Engine::instance()->createWindow( WindowSettings(
					 320, 240, "CSS state dependencies", WindowStyle::Default | WindowStyle::Hidden,
					 WindowBackend::Default, 32, {}, 1, false, true ) ) != nullptr );
	auto* scene = UISceneNode::New();
	SceneManager::instance()->add( scene );
	const int iterations = selectorIterations();
	Uint64 checksum = 0;
	auto runCase = [&]( const char* label, const char* name, UIWidget* target,
						size_t expectedRelated ) {
		StyleSheetSelector selector( name );
		EXPECT_TRUE( selector.select( target, false ) );
		EXPECT_EQ( expectedRelated, selector.getRelatedElements( target, false ).size() );
		const double selectNs = medianNanoseconds( [&] { return selector.select( target, false ); },
												   iterations, checksum );
		const double relatedNs =
			medianNanoseconds( [&] { return selector.getRelatedElements( target, false ).size(); },
							   iterations, checksum );
		UTEST_PRINT_INFO( String::format( "%s: select %.1f ns/call; related %.1f ns/call (%zu); %s",
										  label, selectNs, relatedNs, expectedRelated, name )
							  .c_str() );
	};

	for ( int depth : { 8, 24, 64 } ) {
		auto* root = appendElement( scene->getRoot(), "div" );
		root->setId( "scope" );
		auto* tail = root;
		for ( int i = 0; i < depth; ++i ) {
			tail = appendElement( tail, "div" );
			tail->addClass( "c" );
		}
		auto* target = appendElement( tail, "a" );
		const auto label = String::format( "depth %d", depth );
		const size_t all = static_cast<size_t>( depth );
		runCase( label.c_str(), "div div div a", target, 0 );
		runCase( label.c_str(), ".c:hover > .c > a", target, 1 );
		runCase( label.c_str(), "#scope > .c:hover a", target, 1 );
		runCase( label.c_str(), ".c:hover a", target, all );
		runCase( label.c_str(), "#scope .c:hover .c .c:hover a", target, all );
		runCase( label.c_str(), "#scope > .c:hover .c > .c:hover a", target, all - 1 );
		// The rest of the selector searches again from every candidate unless it is shared.
		runCase( label.c_str(), "[id=scope] .c:hover a", target, all );
		// A tracked rest repeats elements across candidates.
		runCase( label.c_str(), ".c:hover > .c:hover a", target, all );
	}

	auto* siblings = appendElement( scene->getRoot(), "div" );
	appendElement( siblings, "span" )->setId( "start" );
	for ( int i = 0; i < 64; ++i ) {
		appendElement( siblings, "div" )->addClass( "candidate" );
		appendElement( siblings, "span" );
	}
	auto* target = appendElement( siblings, "a" );
	runCase( "64 sibling candidates", "#start + .candidate:hover ~ a", target, 1 );
	runCase( "64 sibling candidates", ".candidate:hover ~ a", target, 64 );
	runCase( "64 sibling candidates", "div:hover > .candidate ~ a", target, 1 );
	runCase( "64 sibling candidates", "div .candidate:hover ~ a", target, 64 );

	// A shared rest that climbs above the candidates: costs must grow linearly with siblings.
	for ( int count : { 16, 64, 256 } ) {
		auto* list = appendElement( scene->getRoot(), "div" );
		list->addClass( "list" );
		for ( int i = 0; i < count; ++i )
			appendElement( list, "div" )->addClass( "candidate" );
		auto* last = appendElement( list, "a" );
		const auto label = String::format( "%d list siblings", count );
		runCase( label.c_str(), ".list > .candidate:hover ~ a", last, count );
		runCase( label.c_str(), ".list:hover > .candidate:hover ~ a", last, count + 1 );
	}

	// Scenes: style loading subscribes every subject, and hover changes restyle the subscribers.
	// Each load gets a fresh scene so its <style> rules are not combined with earlier copies.
	auto runScene = [&]( const char* label, const std::string& layout, const char* hoveredId ) {
		std::array<double, SampleCount> loads;
		UISceneNode* loaded = nullptr;
		for ( auto& load : loads ) {
			if ( loaded ) {
				SceneManager::instance()->remove( loaded );
				eeDelete( loaded );
			}
			loaded = UISceneNode::New();
			SceneManager::instance()->add( loaded );
			Clock clock;
			loaded->loadLayoutFromString( layout );
			load = clock.getElapsedTime().asMicroseconds();
		}
		std::sort( loads.begin(), loads.end() );
		size_t subscriptions = 0;
		size_t subscribers = 0;
		std::vector<Node*> stack{ loaded->getRoot() };
		while ( !stack.empty() ) {
			Node* node = stack.back();
			stack.pop_back();
			if ( node->isWidget() && node->asType<UIWidget>()->getUIStyle() ) {
				const size_t count =
					node->asType<UIWidget>()->getUIStyle()->getRelatedWidgets().size();
				subscriptions += count;
				subscribers += count != 0;
			}
			for ( Node* child = node->getFirstChild(); child; child = child->getNextNode() )
				stack.push_back( child );
		}
		auto* hovered = loaded->getRoot()->find( hoveredId )->asType<UIWidget>();
		const size_t restyled = hovered->getUIStyle()->getRelatedWidgets().size();
		// Hover only applies to the widget under the mouse, as with real pointer input.
		auto* dispatcher = loaded->getEventDispatcher();
		auto toggle = [&]( bool hover ) {
			dispatcher->setMouseOverNode( hover ? hovered : nullptr );
			if ( hover )
				hovered->pushState( UIState::StateHover );
			else
				hovered->popState( UIState::StateHover );
		};
		auto* subscriber = restyled ? *hovered->getUIStyle()->getRelatedWidgets().begin() : nullptr;
		toggle( true );
		EXPECT_TRUE( subscriber && subscriber->getAlpha() == 255.f );
		toggle( false );
		EXPECT_TRUE( subscriber && subscriber->getAlpha() == 0.f );
		const double toggleNs = medianNanoseconds(
			[&] {
				toggle( true );
				toggle( false );
				return hovered->getAlpha() == 255.f;
			},
			eemax( 1, iterations / 100 ), checksum );
		UTEST_PRINT_INFO(
			String::format(
				"%s: load %.1f us; %zu subscriptions on %zu watched widgets; hover toggle "
				"on #%s %.1f ns (%zu restyled)",
				label, loads[SampleCount / 2], subscriptions, subscribers, hoveredId, toggleNs,
				restyled )
				.c_str() );
		SceneManager::instance()->remove( loaded );
		eeDelete( loaded );
	};

	std::string nested = "<div id='scene'><style>span { opacity: 0; } "
						 ".c:hover span { opacity: 1; }</style>";
	for ( int i = 0; i < 24; ++i )
		nested += String::format( "<div id='c%d' class='c'>", i );
	for ( int i = 0; i < 32; ++i )
		nested += "<span />";
	for ( int i = 0; i < 24; ++i )
		nested += "</div>";
	nested += "</div>";
	runScene( "24 nested .c, 32 spans", nested, "c0" );
	runScene( "24 nested .c, 32 spans", nested, "c23" );

	std::string items = "<div id='scene'><style>.t { opacity: 0; } "
						".item:hover ~ .t { opacity: 1; }</style>";
	for ( int i = 0; i < 64; ++i )
		items += String::format( "<div id='i%d' class='item' />", i );
	for ( int i = 0; i < 16; ++i )
		items += "<div class='t' />";
	items += "</div>";
	runScene( "64 .item, 16 targets", items, "i0" );
	runScene( "64 .item, 16 targets", items, "i63" );

	UTEST_PRINT_INFO( String::format( "%d iterations/sample; %zu samples; checksum %llu",
									  iterations, SampleCount,
									  static_cast<unsigned long long>( checksum ) )
						  .c_str() );
	EXPECT_TRUE( checksum > 0 );
	Engine::destroySingleton();
}
