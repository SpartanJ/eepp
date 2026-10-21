#include "utest.hpp"

#include <array>
#include <chrono>
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/cursor.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/runtime.hpp>
#include <memory>
#include <thread>
#include <type_traits>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;

UTEST( FontPreferences, updatesExistingAndFutureTextWithoutOverridingStyles ) {
	UIApplication app(
		WindowSettings{ 400, 300, "UI Font Preferences" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* scene = app.getUI();
	auto* layout = scene->loadLayoutFromString( R"xml(
		<vbox>
			<TextView id="default_text" text="Default font" />
			<TextView id="styled_text" text="Styled font" />
		</vbox>
	)xml" );
	ASSERT_NE( nullptr, layout );
	scene->combineStyleSheet( "#styled_text { font-size: 19dp; font-family: monospace; }" );
	scene->flushDirtyStyleAndLayout();
	auto* defaultText = layout->find<UITextView>( "default_text" );
	auto* styledText = layout->find<UITextView>( "styled_text" );
	Font* styledFont = styledText->getFont();
	const auto styledSize = styledText->getFontSize();
	EXPECT_EQ( 19u, styledSize );
	const std::string fontPath = Sys::getProcessPath() + ".." + FileSystem::getOSSlash() +
								 "assets" + FileSystem::getOSSlash() + "fonts" +
								 FileSystem::getOSSlash() + "DejaVuSansMono.ttf";
	auto font = FontTrueType::New( "preferences-ui-font", *scene->getResourceScope() );
	ASSERT_TRUE( font->loadFromFile( fontPath ) );
	EXPECT_STDSTREQ( FileSystem::getRealPath( fontPath ), FontTrueType::filePathOf( font.get() ) );

	scene->applyDefaultFont( font.get(), 18 );
	EXPECT_EQ( font.get(), defaultText->getFont() );
	EXPECT_EQ( 18u, defaultText->getFontSize() );
	EXPECT_EQ( styledFont, styledText->getFont() );
	EXPECT_EQ( styledSize, styledText->getFontSize() );
	EXPECT_EQ( font.get(), scene->getUIThemeManager()->getDefaultTheme()->getDefaultFont() );
	EXPECT_EQ( 18.f, scene->getUIThemeManager()->getDefaultTheme()->getDefaultFontSize() );

	auto* newText = UITextView::New();
	newText->setParent( scene->getRoot() );
	EXPECT_EQ( font.get(), newText->getFont() );
	EXPECT_EQ( 18u, newText->getFontSize() );
	scene->applyDefaultFont( nullptr, 22 );
	EXPECT_EQ( font.get(), newText->getFont() );
	EXPECT_EQ( 22u, newText->getFontSize() );
	EXPECT_EQ( 22u, defaultText->getFontSize() );
	EXPECT_EQ( styledSize, styledText->getFontSize() );
}

UTEST( FontPreferences, renderingPolicyReachesBaseAndSceneFonts ) {
	UIApplication app(
		WindowSettings{ 400, 300, "Font Rendering Preferences" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* scene = app.getUI();
	auto* baseFont = static_cast<FontTrueType*>( scene->getUIThemeManager()->getDefaultFont() );
	auto localFont = FontTrueType::New( "preferences-local-font", *scene->getResourceScope() );
	ASSERT_TRUE( localFont->loadFromFile( FontTrueType::filePathOf( baseFont ) ) );

	scene->setFontRendering( FontHinting::None, FontAntialiasing::None );
	for ( auto* font : { baseFont, localFont.get() } ) {
		EXPECT_EQ( Font::fontHintingFromEnvironment( FontHinting::None ), font->getHinting() );
		EXPECT_EQ( Font::fontAntialiasingFromEnvironment( FontAntialiasing::None ),
				   font->getAntialiasing() );
	}
	scene->setFontRendering( FontHinting::Full, FontAntialiasing::Grayscale );
	for ( auto* font : { baseFont, localFont.get() } ) {
		EXPECT_EQ( Font::fontHintingFromEnvironment( FontHinting::Full ), font->getHinting() );
		EXPECT_EQ( Font::fontAntialiasingFromEnvironment( FontAntialiasing::Grayscale ),
				   font->getAntialiasing() );
	}
}

UTEST( FontPreferences, rejectedFontCanBeDestroyedOrLoadedAgain ) {
	UIApplication app(
		WindowSettings{ 400, 300, "Rejected Font Preferences" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	const std::string fontPath = Sys::getProcessPath() + ".." + FileSystem::getOSSlash() +
								 "assets" + FileSystem::getOSSlash() + "fonts" +
								 FileSystem::getOSSlash() + "DejaVuSansMono.ttf";
	std::string data;
	ASSERT_TRUE( FileSystem::fileGet( fontPath, data ) );
	ASSERT_GE( data.size(), 12u );
	const size_t tableCount =
		( static_cast<Uint8>( data[4] ) << 8 ) | static_cast<Uint8>( data[5] );
	ASSERT_GE( data.size(), 12u + tableCount * 16u );
	// FreeType must reject Unicode selection without a character map or glyph names to synthesize
	// one.
	for ( size_t index = 0; index < tableCount; ++index ) {
		const size_t offset = 12 + index * 16;
		if ( data.compare( offset, 4, "cmap" ) == 0 || data.compare( offset, 4, "post" ) == 0 )
			data.replace( offset, 4, "test" );
	}
	{
		auto font =
			FontTrueType::New( "rejected-preference-font", *app.getUI()->getResourceScope() );
		EXPECT_FALSE( font->loadFromMemory( data.data(), data.size() ) );
		EXPECT_FALSE( font->loaded() );
		EXPECT_TRUE( font->loadFromFile( fontPath ) );
	}
	auto font = FontTrueType::New( "rejected-font-destruction", *app.getUI()->getResourceScope() );
	EXPECT_FALSE( font->loadFromMemory( data.data(), data.size() ) );
	EXPECT_FALSE( font->loaded() );
}

UTEST( UISceneNode, CssPointerCursorUsesHandCursor ) {
	EXPECT_EQ( Cursor::fromName( "pointer" ), Cursor::Hand );
	EXPECT_EQ( Cursor::fromName( "POINTER" ), Cursor::Hand );
	EXPECT_STREQ( Cursor::toName( Cursor::Hand ), "hand" );
	EXPECT_STREQ( Cursor::toName( Cursor::Arrow ), "arrow" );
}

UTEST( UISceneNode, MouseOverAncestryUsesCommittedOverNode ) {
	UIApplication app(
		WindowSettings{ 320, 240, "Mouse Over Ancestry" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* scene = app.getUI();
	auto* parent = UIWidget::New();
	parent->setPixelsSize( 200, 100 );
	parent->setParent( scene->getRoot() );
	auto* firstChild = UIWidget::New();
	firstChild->setPixelsSize( 80, 80 );
	firstChild->setParent( parent );
	auto* secondChild = UIWidget::New();
	secondChild->setPixelsPosition( 100, 0 );
	secondChild->setPixelsSize( 80, 80 );
	secondChild->setParent( parent );
	scene->flushDirtyStyleAndLayout();

	Input* input = app.getWindow()->getInput();
	input->setMousePos( firstChild->convertToWorldSpace( { 20.f, 20.f } ).asInt() );
	SceneManager::instance()->update();

	EXPECT_EQ( scene->getEventDispatcher()->getMouseOverNode(), firstChild );
	EXPECT_TRUE( firstChild->isMouseOverMeOrChildren() );
	EXPECT_TRUE( firstChild->isMouseOver() );
	EXPECT_TRUE( parent->isMouseOverMeOrChildren() );
	EXPECT_TRUE( parent->isMouseOver() );
	EXPECT_FALSE( secondChild->isMouseOverMeOrChildren() );
	EXPECT_FALSE( secondChild->isMouseOver() );

	const Vector2i secondChildPosition = secondChild->convertToWorldSpace( { 20.f, 20.f } ).asInt();
	EXPECT_EQ( scene->overFind( secondChildPosition.asFloat() ), secondChild );
	EXPECT_EQ( scene->getEventDispatcher()->getMouseOverNode(), firstChild );
	EXPECT_TRUE( firstChild->isMouseOverMeOrChildren() );
	EXPECT_TRUE( firstChild->isMouseOver() );
	EXPECT_FALSE( secondChild->isMouseOverMeOrChildren() );
	EXPECT_FALSE( secondChild->isMouseOver() );

	input->setMousePos( secondChildPosition );
	SceneManager::instance()->update();

	EXPECT_EQ( scene->getEventDispatcher()->getMouseOverNode(), secondChild );
	EXPECT_FALSE( firstChild->isMouseOverMeOrChildren() );
	EXPECT_FALSE( firstChild->isMouseOver() );
	EXPECT_TRUE( secondChild->isMouseOverMeOrChildren() );
	EXPECT_TRUE( secondChild->isMouseOver() );
	EXPECT_TRUE( parent->isMouseOverMeOrChildren() );
	EXPECT_TRUE( parent->isMouseOver() );

	input->setMousePos( { 280, 180 } );
	SceneManager::instance()->update();

	EXPECT_FALSE( firstChild->isMouseOverMeOrChildren() );
	EXPECT_FALSE( firstChild->isMouseOver() );
	EXPECT_FALSE( secondChild->isMouseOverMeOrChildren() );
	EXPECT_FALSE( secondChild->isMouseOver() );
	EXPECT_FALSE( parent->isMouseOverMeOrChildren() );
	EXPECT_FALSE( parent->isMouseOver() );
}

UTEST( UISceneNode, ScopedContextBindsNodesAndRestoresNestedScene ) {
	auto* engine = Engine::instance();
	auto* window = engine->getCurrentWindow();
	if ( !window ) {
		window =
			engine->createWindow( WindowSettings( 320, 240, "UI Context Test", WindowStyle::Default,
												  WindowBackend::Default, 32, {}, 1, false, true ),
								  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	}

	auto* sceneA = UISceneNode::New( window );
	auto* sceneB = UISceneNode::New( window );
	auto* sceneManager = SceneManager::instance();
	sceneManager->add( sceneA );
	sceneManager->add( sceneB );
	sceneManager->setCurrentSceneNode( sceneA );

	{
		auto contextA = sceneA->makeCurrent();
		EXPECT_EQ( sceneManager->getUISceneNode(), sceneA );
		auto* nodeA = UIWidget::New();
		EXPECT_EQ( nodeA->getUISceneNode(), sceneA );

		{
			auto contextB = sceneB->makeCurrent();
			EXPECT_EQ( sceneManager->getUISceneNode(), sceneB );
			auto* nodeB = UIWidget::New();
			EXPECT_EQ( nodeB->getUISceneNode(), sceneB );
		}

		EXPECT_EQ( sceneManager->getUISceneNode(), sceneA );
	}

	EXPECT_EQ( sceneManager->getUISceneNode(), sceneA );
	sceneManager->remove( sceneB );
	sceneManager->remove( sceneA );
	eeDelete( sceneB );
	eeDelete( sceneA );
}

UTEST( SceneNode, ScopedContextSupportsMixedScenesAndMovedGuards ) {
	static_assert( std::is_same_v<SceneNode::Context, UISceneNode::Context> );
	UIApplication app(
		WindowSettings( 320, 240, "Mixed Scene Context", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* manager = SceneManager::instance();
	auto* engine = Engine::instance();
	SceneNode plainScene( app.getWindow() );
	EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );

	{
		// Bind a UI scene through the base API.
		SceneNode* baseScene = app.getUI();
		auto uiContext = baseScene->makeCurrent();
		EXPECT_EQ( manager->getCurrentSceneNode(), baseScene );
		{
			auto plainContext = plainScene.makeCurrent();
			EXPECT_EQ( manager->getCurrentSceneNode(), &plainScene );
			EXPECT_TRUE( manager->getUISceneNode() == nullptr );
			EXPECT_EQ( manager->getUISceneNode( app.getWindow() ), app.getUI() );
			auto orphanWidget = std::unique_ptr<UIWidget>( UIWidget::New() );
			EXPECT_TRUE( orphanWidget->getUISceneNode() == nullptr );
			EXPECT_TRUE( orphanWidget->getParent() == nullptr );

			{
				auto movedContext = std::move( plainContext );
				auto nestedScene =
					std::unique_ptr<UISceneNode>( UISceneNode::New( app.getWindow() ) );
				EXPECT_EQ( nestedScene->getRoot()->getUISceneNode(), nestedScene.get() );
				EXPECT_EQ( manager->getCurrentSceneNode(), &plainScene );
				{
					// Explicit binding also works before registration with the manager.
					auto nestedContext = nestedScene->makeCurrent();
					EXPECT_EQ( manager->getCurrentSceneNode(), nestedScene.get() );
					EXPECT_EQ( manager->getUISceneNode(), nestedScene.get() );
					auto* nestedWidget = UIWidget::New();
					EXPECT_EQ( nestedWidget->getUISceneNode(), nestedScene.get() );
				}
				EXPECT_EQ( manager->getCurrentSceneNode(), &plainScene );
			}
			EXPECT_EQ( manager->getCurrentSceneNode(), baseScene );
		}
		// The moved-from guard must not restore the scope a second time.
		EXPECT_EQ( manager->getCurrentSceneNode(), baseScene );
	}
	EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
	EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );
}

class ContextTestSceneNode : public SceneNode {
  public:
	explicit ContextTestSceneNode( EE::Window::Window* window ) : SceneNode( window ) {
		setEventDispatcher( EventDispatcher::New( this ) );
	}

	void draw() {
		drawBound = hasCurrentContext();
		++drawCount;
	}

	void update( const Time& ) { updateBound = hasCurrentContext(); }

	Uint32 onTextInput( const TextInputEvent& ) {
		inputBound = hasCurrentContext();
		return 1;
	}

	Uint32 onTextEditing( const TextEditingEvent& ) {
		editingBound = hasCurrentContext();
		return 1;
	}

	bool drawBound{ false };
	bool updateBound{ false };
	bool inputBound{ false };
	bool editingBound{ false };
	int drawCount{ 0 };

  private:
	bool hasCurrentContext() {
		return SceneManager::instance()->getCurrentSceneNode() == this &&
			   Engine::instance()->getCurrentWindow() == getWindow();
	}
};

UTEST( SceneManager, PersistentBindingDoesNotFallBackAfterRemoval ) {
	UIApplication app(
		WindowSettings( 320, 240, "Persistent Scene Binding", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* manager = SceneManager::instance();
	EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
	EXPECT_EQ( manager->setCurrentSceneNode( nullptr ), app.getUI() );
	EXPECT_TRUE( manager->getUISceneNode() == nullptr );

	SceneNode plainScene( app.getWindow() );
	manager->add( &plainScene );
	EXPECT_EQ( manager->getCurrentSceneNode(), &plainScene );
	EXPECT_TRUE( manager->getUISceneNode() == nullptr );
	EXPECT_EQ( manager->getUISceneNode( app.getWindow() ), app.getUI() );
	ASSERT_TRUE( manager->remove( &plainScene ) );
	EXPECT_TRUE( manager->getCurrentSceneNode() == nullptr );
	EXPECT_TRUE( manager->getUISceneNode() == nullptr );
	manager->setCurrentSceneNode( app.getUI() );
}

UTEST( SceneNode, ManagerAndInputBindPlainSceneWindowAndRestoreAmbientContext ) {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		UTEST_SKIP( "Terminal runtime supports only one native window" );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UTEST_SKIP( "Emscripten supports only one native window" );
#endif
	UIApplication app(
		WindowSettings( 320, 240, "Scene Context Routing", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* secondaryUI =
		app.createWindow( WindowSettings( 240, 180, "Secondary Scene Context", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondaryUI != nullptr );
	auto* manager = SceneManager::instance();
	auto* engine = Engine::instance();
	ContextTestSceneNode plainScene( secondaryUI->getWindow() );
	manager->add( &plainScene );
	{
		auto ambientContext = app.getUI()->makeCurrent();
		manager->update( Time::Zero );
		EXPECT_TRUE( plainScene.updateBound );
		EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
		EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );
		manager->draw( app.getWindow() );
		EXPECT_EQ( plainScene.drawCount, 0 );
		manager->draw( secondaryUI->getWindow() );
		EXPECT_TRUE( plainScene.drawBound );
		EXPECT_EQ( plainScene.drawCount, 1 );
		manager->draw();
		EXPECT_EQ( plainScene.drawCount, 2 );
		EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
		EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );

		InputEvent event{};
		event.Type = InputEvent::TextInput;
		event.WinID = secondaryUI->getWindow()->getWindowID();
		event.text.text = 'x';
		app.getWindow()->getInput()->processEventForWindow( &event );
		EXPECT_TRUE( plainScene.inputBound );
		EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
		EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );

		event.Type = InputEvent::TextEditing;
		event.textediting.text = "x";
		event.textediting.start = 0;
		event.textediting.length = 1;
		app.getWindow()->getInput()->processEventForWindow( &event );
		EXPECT_TRUE( plainScene.editingBound );
		EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
		EXPECT_EQ( engine->getCurrentWindow(), app.getWindow() );
	}
	manager->remove( &plainScene );
	EXPECT_EQ( manager->getCurrentSceneNode(), app.getUI() );
}

UTEST( UISceneNode, AsyncResourceCallbacksBindOwnerSceneAndRestorePreviousScene ) {
	auto* engine = Engine::instance();
	auto* window = engine->getCurrentWindow();
	if ( !window ) {
		window = engine->createWindow( WindowSettings( 320, 240, "Async Resource Context Test",
													   WindowStyle::Default, WindowBackend::Default,
													   32, {}, 1, false, true ),
									   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	}

	auto* sceneA = UISceneNode::New( window );
	auto* sceneB = UISceneNode::New( window );
	auto* sceneManager = SceneManager::instance();
	sceneManager->add( sceneA );
	sceneManager->add( sceneB );
	sceneManager->setCurrentSceneNode( sceneA );

	auto state = sceneB->getAsyncResourceLoadState();
	const Uint64 generation = state->generation.load( std::memory_order_acquire );
	bool immediateBound = false;
	bool queuedBound = false;
	{
		auto context = sceneA->makeCurrent();
		UISceneNode::runAsyncResourceOnMainThread( state, generation, [&]( UISceneNode* owner ) {
			immediateBound = owner == sceneB && sceneManager->getUISceneNode() == sceneB;
		} );
		EXPECT_EQ( sceneManager->getUISceneNode(), sceneA );

		std::thread producer( [&] {
			UISceneNode::runAsyncResourceOnMainThread(
				state, generation, [&]( UISceneNode* owner ) {
					queuedBound = owner == sceneB && sceneManager->getUISceneNode() == sceneB;
				} );
		} );
		producer.join();
		sceneA->update( Time::Zero );
		EXPECT_EQ( sceneManager->getUISceneNode(), sceneA );
	}

	EXPECT_TRUE( immediateBound );
	EXPECT_TRUE( queuedBound );
	sceneManager->remove( sceneB );
	sceneManager->remove( sceneA );
	eeDelete( sceneB );
	eeDelete( sceneA );
}

UTEST( UISceneNode, AsyncResourceEnqueueDuringDrainSurvivesTheNextIdleCheck ) {
	UIApplication app( WindowSettings( 320, 240, "Async Resource Drain", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ) );
	auto* scene = app.getUI();
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto state = scene->getAsyncResourceLoadState();
	const auto generation = state->generation.load( std::memory_order_acquire );
	int delivered = 0;
	scene->update( Time::Zero );

	std::thread producer( [&] {
		UISceneNode::runAsyncResourceOnMainThread( state, generation, [&]( UISceneNode* ) {
			++delivered;
			// Queue from a worker after the drain swapped out its current batch.
			std::thread nested( [&] {
				UISceneNode::runAsyncResourceOnMainThread( state, generation,
														   [&]( UISceneNode* ) { ++delivered; } );
			} );
			nested.join();
		} );
	} );
	producer.join();
	scene->update( Time::Zero );
	EXPECT_EQ( delivered, 1 );
	scene->update( Time::Zero );
	EXPECT_EQ( delivered, 2 );
	scene->update( Time::Zero );
	EXPECT_EQ( delivered, 2 );
}

UTEST( UISceneNode, AsyncResourceConcurrentProducersDeliverExactlyOnce ) {
	UIApplication app( WindowSettings( 320, 240, "Async Resource Producers", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ) );
	auto* scene = app.getUI();
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto state = scene->getAsyncResourceLoadState();
	const auto generation = state->generation.load( std::memory_order_acquire );
	constexpr size_t producerCount = 4;
	constexpr size_t deliveriesPerProducer = 128;
	std::array<int, producerCount * deliveriesPerProducer> delivered{};
	std::atomic<size_t> completed{};
	std::array<std::thread, producerCount> producers;
	for ( size_t producer = 0; producer < producerCount; ++producer ) {
		producers[producer] = std::thread( [&, producer] {
			for ( size_t i = 0; i < deliveriesPerProducer; ++i ) {
				const size_t index = producer * deliveriesPerProducer + i;
				UISceneNode::runAsyncResourceOnMainThread(
					state, generation, [&, index]( UISceneNode* ) { ++delivered[index]; } );
				std::this_thread::yield();
			}
			completed.fetch_add( 1, std::memory_order_release );
		} );
	}
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	while ( completed.load( std::memory_order_acquire ) != producerCount &&
			std::chrono::steady_clock::now() < deadline ) {
		scene->update( Time::Zero );
		std::this_thread::yield();
	}
	for ( auto& producer : producers )
		producer.join();
	scene->update( Time::Zero );
	scene->update( Time::Zero );
	for ( const auto count : delivered )
		EXPECT_EQ( count, 1 );
}

UTEST( UISceneNode, AsyncResourceDelayedWorkRemainsPendingAndInvalidationReleasesIt ) {
	UIApplication app( WindowSettings( 320, 240, "Async Resource Delays", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ) );
	auto* scene = app.getUI();
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto state = scene->getAsyncResourceLoadState();
	const auto generation = state->generation.load( std::memory_order_acquire );
	int delivered = 0;
	auto retained = std::make_shared<int>( 1 );
	std::weak_ptr<int> weak = retained;
	UISceneNode::runAsyncResourceOnMainThread(
		state, generation,
		[retained = std::move( retained ), &delivered]( UISceneNode* ) { ++delivered; },
		Seconds( 60 ) );
	scene->update( Time::Zero );
	scene->update( Time::Zero );
	EXPECT_EQ( delivered, 0 );
	EXPECT_FALSE( weak.expired() );
	scene->invalidateAsyncResourceLoads();
	scene->update( Time::Zero );
	EXPECT_TRUE( weak.expired() );
	EXPECT_EQ( delivered, 0 );

	const auto currentGeneration = state->generation.load( std::memory_order_acquire );
	UISceneNode::runAsyncResourceOnMainThread(
		state, currentGeneration, [&]( UISceneNode* ) { ++delivered; }, Milliseconds( 20 ) );
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	while ( !delivered && std::chrono::steady_clock::now() < deadline ) {
		scene->update( Time::Zero );
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	EXPECT_EQ( delivered, 1 );
	scene->update( Time::Zero );
	EXPECT_EQ( delivered, 1 );
}

UTEST( UISceneNode, WidgetDeletionUsesDraggingStateAfterCloseCallbacks ) {
	UIApplication app( WindowSettings( 320, 240, "Widget Dragging Teardown", WindowStyle::Default,
									   WindowBackend::Default, 32, {}, 1, false, true ) );
	auto* scene = app.getUI();
	scene->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto* dispatcher = scene->getEventDispatcher();
	auto* widget = UIWidget::New();
	dispatcher->setNodeDragging( widget );
	eeDelete( widget );
	EXPECT_TRUE( dispatcher->getNodeDragging() == nullptr );

	auto* survivor = UIWidget::New();
	widget = UIWidget::New();
	dispatcher->setNodeDragging( widget );
	widget->addEventListener( Event::OnClose, [dispatcher, survivor]( const Event* ) {
		dispatcher->setNodeDragging( nullptr );
		dispatcher->setNodeDragging( survivor );
	} );
	eeDelete( widget );
	EXPECT_EQ( dispatcher->getNodeDragging(), survivor );
	eeDelete( survivor );
	EXPECT_TRUE( dispatcher->getNodeDragging() == nullptr );
}

class TestUIApplication : public UIApplication {
  public:
	using UIApplication::UIApplication;

	void tickOnce() { tick(); }

	void markRunning() { mRunning = true; }
};

UTEST( UIApplication, NativeQuitStopsLoopWithoutClosingWindowsAndCanBeOverridden ) {
	TestUIApplication app(
		WindowSettings( 320, 240, "Native Quit", WindowStyle::Default, WindowBackend::Default, 32,
						{}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f,
								 false ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* window = app.getWindow();
	ASSERT_TRUE( window != nullptr );
	app.setQuitPolicy( UIApplication::QuitPolicy::Explicit );
	app.markRunning();
	InputEvent event( InputEvent::Quit );
	event.WinID = window->getWindowID();
	window->getInput()->pushEvent( event );
	EXPECT_FALSE( app.isRunning() );
	EXPECT_TRUE( window->isOpen() );
	EXPECT_EQ( app.getWindowCount(), 1u );
	EXPECT_TRUE( app.getUI() != nullptr );

	bool handled = false;
	window->setQuitCallback( [&]( EE::Window::Window* target ) {
		handled = true;
		EXPECT_EQ( target, window );
	} );
	app.markRunning();
	window->getInput()->pushEvent( event );
	EXPECT_TRUE( handled );
	EXPECT_TRUE( app.isRunning() );
}

UTEST( UIApplication, NativeQuitFromSecondaryWindowStopsApplication ) {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		UTEST_SKIP( "Terminal runtime supports only one native window" );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UTEST_SKIP( "Emscripten supports only one native window" );
#endif
	TestUIApplication app(
		WindowSettings( 320, 240, "Primary Quit", WindowStyle::Default, WindowBackend::Default, 32,
						{}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f,
								 false ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* secondary =
		app.createWindow( WindowSettings( 240, 180, "Secondary Quit", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondary != nullptr );
	app.setQuitPolicy( UIApplication::QuitPolicy::OnLastWindowClosed );
	app.markRunning();
	InputEvent event( InputEvent::Quit );
	event.WinID = secondary->getWindow()->getWindowID();
	secondary->getWindow()->getInput()->pushEvent( event );
	EXPECT_FALSE( app.isRunning() );
	EXPECT_TRUE( app.getWindow()->isOpen() );
	EXPECT_TRUE( secondary->getWindow()->isOpen() );
	EXPECT_EQ( app.getWindowCount(), 2u );
}

UTEST( UIApplication, ClosingCurrentWindowSelectsSurvivingScene ) {
	if ( Runtime::mode() == RuntimeMode::Terminal )
		UTEST_SKIP( "Terminal runtime supports only one native window" );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	UTEST_SKIP( "Emscripten supports only one native window" );
#endif
	TestUIApplication app(
		WindowSettings( 320, 240, "Closing Current Scene", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* secondaryUI =
		app.createWindow( WindowSettings( 240, 180, "Surviving Scene", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondaryUI != nullptr );
	app.setQuitPolicy( UIApplication::QuitPolicy::OnLastWindowClosed );
	app.closeWindow( app.getWindow() );
	app.tickOnce();
	EXPECT_EQ( app.getWindowCount(), 1u );
	EXPECT_EQ( SceneManager::instance()->getCurrentSceneNode(), secondaryUI );
	EXPECT_EQ( SceneManager::instance()->getUISceneNode(), secondaryUI );
	EXPECT_EQ( Engine::instance()->getCurrentWindow(), secondaryUI->getWindow() );
	auto* widget = UIWidget::New();
	EXPECT_EQ( widget->getUISceneNode(), secondaryUI );
	app.closeWindow( secondaryUI->getWindow() );
	app.tickOnce();
	EXPECT_EQ( app.getWindowCount(), 0u );
	EXPECT_TRUE( SceneManager::instance()->getCurrentSceneNode() == nullptr );
	EXPECT_TRUE( SceneManager::instance()->getUISceneNode() == nullptr );
}

UTEST( UIApplication, ClosesWindowWithVisibleModalDialog ) {
	TestUIApplication app(
		WindowSettings( 320, 240, "Modal shutdown", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getUI() != nullptr );
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* dialog = UIMessageBox::New( UIMessageBox::OK_CANCEL, "Close?" );
	dialog->show();
	app.getUI()->update( Milliseconds( 16 ) );
	ASSERT_TRUE( dialog->getModalWidget() != nullptr );
	app.getWindow()->close();
	app.tickOnce();
	EXPECT_EQ( app.getWindowCount(), 0u );
	EXPECT_TRUE( app.getUI() == nullptr );
}

UTEST( UIApplication, RoutesNativeMouseWheelExactlyOnceToTargetWindow ) {
	TestUIApplication app( WindowSettings( 320, 240, "Primary Wheel Routing", WindowStyle::Default,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f, true ),
						   ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	auto* secondaryUI =
		app.createWindow( WindowSettings( 240, 180, "Secondary Wheel Routing", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondaryUI != nullptr );

	auto* primaryInput = app.getWindow()->getInput();
	auto* secondaryInput = secondaryUI->getWindow()->getInput();
	const Uint32 targetWindowId = secondaryUI->getWindow()->getWindowID();
	int primaryWheelEvents = 0;
	int secondaryWheelEvents = 0;
	Vector2f secondaryWheelOffset;
	Uint32 receivedWindowId = 0;
	const Uint32 primaryCallback = primaryInput->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::MouseWheel )
			++primaryWheelEvents;
	} );
	const Uint32 secondaryCallback = secondaryInput->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::MouseWheel ) {
			++secondaryWheelEvents;
			secondaryWheelOffset = { event->wheel.x, event->wheel.y };
			receivedWindowId = event->WinID;
		}
	} );

	InputEvent event{};
	event.Type = InputEvent::MouseWheel;
	event.WinID = targetWindowId;
	event.wheel.x = 0.f;
	event.wheel.y = -1.f;
	event.wheel.direction = InputEvent::WheelEvent::Normal;
	ASSERT_TRUE( primaryInput->pushEvent( event ) );
	Engine::instance()->updateInput();

	EXPECT_EQ( primaryWheelEvents, 0 );
	EXPECT_EQ( secondaryWheelEvents, 1 );
	EXPECT_EQ( receivedWindowId, targetWindowId );
	EXPECT_EQ( secondaryWheelOffset.x, 0.f );
	EXPECT_EQ( secondaryWheelOffset.y, -1.f );
	primaryInput->popCallback( primaryCallback );
	secondaryInput->popCallback( secondaryCallback );
}

UTEST( UIApplication, CreatesSecondaryWindowWithoutChangingAmbientScene ) {
	auto threadPool = System::ThreadPool::createShared( 1 );
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.5f,
									  true );
	settings.threadPool = threadPool;
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	TestUIApplication app( WindowSettings( 320, 240, "Primary UI Context", WindowStyle::Default,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   settings, ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( app.getWindow() != nullptr );
	ASSERT_TRUE( app.getUI() != nullptr );
	EXPECT_TRUE( app.getUI()->getThreadPool() == threadPool );
	EXPECT_EQ( app.getUI()->getAccessibilityPolicy(), AccessibilityPolicy::Disabled );
	app.tickOnce();
	EXPECT_TRUE( static_cast<const UISceneNode*>( app.getUI() )->getAccessibilityManager() ==
				 nullptr );
	app.getUI()->getAccessibilityManager()->update();
	EXPECT_FALSE( app.getUI()->getAccessibilityManager()->isBackendAvailable() );
	EXPECT_TRUE( app.getUI()->getAccessibilityManager()->isBackendInitializationComplete() );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.5f );
	ASSERT_TRUE( app.getUI()->getUIIconThemeManager()->getCurrentTheme() != nullptr );
	EXPECT_TRUE( app.getUI()->getUIIconThemeManager()->findIcon( "go-up" ) != nullptr );
	EXPECT_EQ( app.getQuitPolicy(), UIApplication::QuitPolicy::OnPrimaryWindowClosed );
	if ( Runtime::mode() == RuntimeMode::Terminal ) {
		auto* inApplicationWindow = UIWindow::NewInApplicationWindow(
			app, WindowSettings( 240, 180, "Terminal In-Application Window" ),
			UIWindow::RELATIVE_LAYOUT, UIWindow::StyleConfig(), ContextSettings(), true );
		ASSERT_TRUE( inApplicationWindow != nullptr );
		EXPECT_EQ( app.getWindowCount(), 1u );
		EXPECT_EQ( inApplicationWindow->getUISceneNode(), app.getUI() );
		EXPECT_TRUE( inApplicationWindow->isModal() );
		EXPECT_FALSE( inApplicationWindow->getWinFlags() & UI_WIN_NO_DECORATION );
		return;
	}

	auto* primaryWindow = app.getWindow();
	auto* secondaryUI =
		app.createWindow( WindowSettings( 240, 180, "Secondary UI Context", WindowStyle::Default,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( secondaryUI != nullptr );
	EXPECT_TRUE( secondaryUI->getThreadPool() == threadPool );
	EXPECT_EQ( secondaryUI->getAccessibilityPolicy(), AccessibilityPolicy::Disabled );
	EXPECT_EQ( PixelDensity::getPixelDensity(), 1.5f );
	EXPECT_EQ( app.getWindowCount(), 2u );
	EXPECT_EQ( app.getUI( secondaryUI->getWindow() ), secondaryUI );
	EXPECT_EQ( secondaryUI->getUIIconThemeManager()->getCurrentTheme(),
			   app.getUI()->getUIIconThemeManager()->getCurrentTheme() );
	EXPECT_EQ( Engine::instance()->getCurrentWindow(), primaryWindow );
	EXPECT_EQ( SceneManager::instance()->getUISceneNode(), app.getUI() );
	TextureFactory::instance()->setCurrentTexture( 123, 0 );
	Engine::instance()->setCurrentWindow( secondaryUI->getWindow() );
	EXPECT_EQ( TextureFactory::instance()->getCurrentTexture( 0 ), -1 );
	Engine::instance()->setCurrentWindow( primaryWindow );

	{
		auto context = secondaryUI->makeCurrent();
		auto* widget = UIWidget::New();
		EXPECT_EQ( widget->getUISceneNode(), secondaryUI );

		auto* textInput = UITextInput::New();
		textInput->setParent( secondaryUI->getRoot() );
		textInput->setFocus();
		InputEvent textEvent;
		textEvent.Type = InputEvent::TextInput;
		textEvent.WinID = secondaryUI->getWindow()->getWindowID();
		textEvent.text.text = 'x';
		textEvent.text.timestamp = 1;
		secondaryUI->getWindow()->getInput()->beginInputFrame();
		primaryWindow->getInput()->processEventForWindow( &textEvent );
		secondaryUI->getWindow()->getInput()->endInputFrame();
		EXPECT_STDSTREQ( textInput->getText().toUtf8(), "x" );
	}

	EXPECT_EQ( SceneManager::instance()->getUISceneNode(), app.getUI() );

	const Vector2i primaryPosition = primaryWindow->getPosition();
	const Sizei primaryScreenSize = primaryWindow->getSizeInScreenCoordinates();
	auto* fileDialog = UIFileDialog::NewInApplicationWindow(
		app,
		WindowSettings( 640, 400, "File Dialog UI Context", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIFileDialog::DefaultFlags, "*", FileSystem::getCurrentWorkingDirectory(),
		ContextSettings( false, 0, 0, GLv_default, true, false ), true,
		UIWindow::ApplicationWindowPosition::CenteredOnPrimary );
	ASSERT_TRUE( fileDialog != nullptr );
	EXPECT_FALSE( fileDialog->getUISceneNode()->getWindow()->isVisible() );
	EXPECT_EQ( app.getWindowCount(), 3u );
	EXPECT_NE( fileDialog->getUISceneNode(), app.getUI() );
	EXPECT_NE( fileDialog->getUISceneNode(), secondaryUI );
	EXPECT_TRUE( fileDialog->getWinFlags() & UI_WIN_NO_DECORATION );
	EXPECT_EQ( fileDialog->getLayoutWidthPolicy(), SizePolicy::MatchParent );
	EXPECT_EQ( fileDialog->getLayoutHeightPolicy(), SizePolicy::MatchParent );
	EXPECT_EQ( Engine::instance()->getCurrentWindow(), primaryWindow );
	EXPECT_EQ( SceneManager::instance()->getUISceneNode(), app.getUI() );
	EXPECT_TRUE( fileDialog->getParent()->isType( UI_TYPE_RELATIVE_LAYOUT ) );
	const Sizei dialogWindowSize =
		fileDialog->getUISceneNode()->getWindow()->getSizeInScreenCoordinates();
	const Sizef dialogMinimumSizePx = PixelDensity::dpToPx( fileDialog->getMinWindowSize() );
	const Float dialogWindowScale = fileDialog->getUISceneNode()->getWindow()->getScale();
	EXPECT_TRUE( dialogWindowSize.getWidth() >=
				 eeceil( dialogMinimumSizePx.getWidth() / dialogWindowScale ) );
	EXPECT_TRUE( dialogWindowSize.getHeight() >=
				 eeceil( dialogMinimumSizePx.getHeight() / dialogWindowScale ) );
	const Vector2i dialogPosition = fileDialog->getUISceneNode()->getWindow()->getPosition();
	const Vector2i primaryCenter( primaryPosition.x + primaryScreenSize.getWidth() / 2,
								  primaryPosition.y + primaryScreenSize.getHeight() / 2 );
	auto* displayManager = Engine::instance()->getDisplayManager();
	for ( int i = 0; i < displayManager->getDisplayCount(); ++i ) {
		auto* display = displayManager->getDisplayIndex( i );
		if ( nullptr == display || !display->getBounds().contains( primaryCenter ) )
			continue;
		const Rect usableBounds = display->getUsableBounds();
		const Rect border = fileDialog->getUISceneNode()->getWindow()->getBorderSize();
		if ( dialogWindowSize.getWidth() + border.Left + border.Right <= usableBounds.getWidth() ) {
			EXPECT_TRUE( dialogPosition.x - border.Left >= usableBounds.Left );
			EXPECT_TRUE( dialogPosition.x + dialogWindowSize.getWidth() + border.Right <=
						 usableBounds.Right );
		}
		if ( dialogWindowSize.getHeight() + border.Top + border.Bottom <=
			 usableBounds.getHeight() ) {
			EXPECT_TRUE( dialogPosition.y - border.Top >= usableBounds.Top );
			EXPECT_TRUE( dialogPosition.y + dialogWindowSize.getHeight() + border.Bottom <=
						 usableBounds.Bottom );
		}
		break;
	}

	auto* primaryFont = app.getUI()->getUIThemeManager()->getDefaultFont();
	ASSERT_TRUE( primaryFont != nullptr );
	fileDialog->getUISceneNode()->getWindow()->close();
	app.tickOnce();
	EXPECT_EQ( app.getWindowCount(), 2u );
	EXPECT_EQ( app.getUI()->getUIThemeManager()->getDefaultFont(), primaryFont );
	auto* genericWindow = UIWindow::NewInApplicationWindow(
		app,
		WindowSettings( 400, 240, "Generic UI Window", WindowStyle::Default, WindowBackend::Default,
						32, {}, 1, false, true ),
		UIWindow::RELATIVE_LAYOUT, UIWindow::StyleConfig(),
		ContextSettings( false, 0, 0, GLv_default, true, false ), true );
	ASSERT_TRUE( genericWindow != nullptr );
	EXPECT_EQ( app.getWindowCount(), 3u );
	genericWindow->getUISceneNode()->getWindow()->close();
	app.tickOnce();
	EXPECT_EQ( app.getWindowCount(), 2u );
	auto* messageBox = UIMessageBox::NewInApplicationWindow(
		app,
		WindowSettings( 400, 180, "Message Box UI Context", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIMessageBox::OK, "Native modal message", UI_MESSAGE_BOX_DEFAULT_FLAGS,
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( messageBox != nullptr );
	EXPECT_EQ( app.getWindowCount(), 3u );
	EXPECT_FALSE( messageBox->isModal() );
	EXPECT_TRUE( messageBox->getModalWidget() == nullptr );
	const Sizei messageBoxWindowSize =
		messageBox->getUISceneNode()->getWindow()->getSizeInScreenCoordinates();
	const Uint32 messageBoxWindowId = messageBox->getUISceneNode()->getWindow()->getWindowID();
	EXPECT_EQ( messageBoxWindowSize.getWidth(), 400 );
	EXPECT_EQ( messageBoxWindowSize.getHeight(), 180 );
	bool messageBoxConfirmed = false;
	messageBox->on( Event::OnConfirm,
					[&messageBoxConfirmed]( const Event* ) { messageBoxConfirmed = true; } );
	messageBox->getUISceneNode()->getUIThemeManager()->setDefaultEffectsEnabled( true );
	messageBox->getEventDispatcher()->sendMsg( messageBox->getButtonOK(), NodeMessage::MouseClick,
											   EE_BUTTON_LMASK );
	EXPECT_TRUE( messageBoxConfirmed );
	EXPECT_FALSE( messageBox->getUISceneNode()->getWindow()->isVisible() );
	for ( int i = 0; i < 20 && app.getWindowCount() == 3u; ++i ) {
		Sys::sleep( Milliseconds( 5 ) );
		app.tickOnce();
	}
	EXPECT_EQ( app.getWindowCount(), 2u );
	EXPECT_TRUE( Engine::instance()->getWindowID( messageBoxWindowId ) == nullptr );

	auto* reopenedDialog = UIFileDialog::NewInApplicationWindow(
		app,
		WindowSettings( 640, 400, "Reopened File Dialog", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		UIFileDialog::DefaultFlags, "*", FileSystem::getCurrentWorkingDirectory(),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	ASSERT_TRUE( reopenedDialog != nullptr );
	EXPECT_EQ( app.getWindowCount(), 3u );
	EXPECT_EQ( reopenedDialog->getUISceneNode()->getUIThemeManager()->getDefaultFont(),
			   primaryFont );

	primaryWindow->close();
	app.tickOnce();
	EXPECT_EQ( app.getWindowCount(), 0u );
}

UTEST( Node, DescendantWorldBoundsRefreshAfterDirtyAncestorMoves ) {
	Node* parent = Node::New();
	Node* child = Node::New();
	Node* descendant = Node::New();
	child->setParent( parent );
	descendant->setParent( child );
	child->setPosition( 10.f, 0.f );
	descendant->setPosition( 5.f, 0.f );
	descendant->setSize( 10.f, 10.f );

	EXPECT_EQ( 15.f, descendant->getWorldBounds().Left );
	child->setPosition( 20.f, 0.f );
	EXPECT_EQ( 25.f, descendant->getScreenRect().Left );
	EXPECT_EQ( 25.f, descendant->getWorldBounds().Left );

	eeDelete( parent );
}

static void init_test_scene_node( UISceneNode* sceneNode ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );
	FontTrueType* monoFont = FontTrueType::New( "monospace" ).get();
	monoFont->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentSceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
}

static UISceneNode* init_test_scene_node() {
	UISceneNode* sceneNode = UISceneNode::New();
	init_test_scene_node( sceneNode );
	return sceneNode;
}

class InvalidationTestSceneNode : public UISceneNode {
  public:
	static InvalidationTestSceneNode* New() { return eeNew( InvalidationTestSceneNode, () ); }

	size_t pendingStyleCount() const { return mDirtyStyle.size(); }

	size_t pendingStyleStateCount() const { return mDirtyStyleState.size(); }

	size_t pendingStyleStateAnimationCount() const { return mDirtyStyleStateCSSAnimations.size(); }

	size_t pendingLayoutCount() const { return mDirtyLayouts.size(); }

	bool hasPendingLayout( UILayout* layout ) const { return mDirtyLayouts.count( layout ) != 0; }

	bool hasPendingStyle( UIWidget* widget ) const { return mDirtyStyle.count( widget ) != 0; }

	bool hasPendingStyleState( UIWidget* widget ) const {
		return mDirtyStyleState.count( widget ) != 0;
	}

	bool hasPendingStyleStateAnimations( UIWidget* widget ) const {
		return mDirtyStyleStateCSSAnimations.count( widget ) != 0;
	}

	size_t processedStyleRootCount() const { return mDirtyStylesSnapshot.size(); }

	UIWidget* processedStyleRoot( size_t index ) const { return mDirtyStylesSnapshot[index].first; }

	bool processedStyleRootDisablesAnimations( size_t index ) const {
		return mDirtyStylesSnapshot[index].second;
	}

  protected:
	InvalidationTestSceneNode() : UISceneNode() {}
};

UTEST( UISceneNode, ViewportMetricsAreIndependentFromSceneExtent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Scene Viewport Metrics Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	sceneNode->setPixelsSize( 800, 3000 );

	EXPECT_EQ( sceneNode->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getPixelsSize().getHeight(), 3000 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 3000 );
	EXPECT_EQ( sceneNode->getMediaFeatures().height, 3000 );

	UIWidget* widget = UIWidget::New();
	widget->setId( "viewport-metric-target" );
	widget->setParent( sceneNode->getRoot() );
	sceneNode->setStyleSheet( R"css(
#viewport-metric-target { background-color: #ff0000; height: 100vh; }
@media screen and (max-height: 700px) {
	#viewport-metric-target { background-color: #00ff00; }
}
)css" );
	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_NEAR( widget->getPixelsSize().getHeight(), 3000.f, 0.01f );

	sceneNode->setViewportPixelsSize( Sizef( 800, 600 ) );

	EXPECT_EQ( sceneNode->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getPixelsSize().getHeight(), 3000 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 600 );
	EXPECT_EQ( sceneNode->getMediaFeatures().width, 800 );
	EXPECT_EQ( sceneNode->getMediaFeatures().height, 600 );
	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#00ff00" ) );
	EXPECT_NEAR( widget->getPixelsSize().getHeight(), 600.f, 0.01f );
	EXPECT_NEAR( widget->convertLength( StyleSheetLength( "100vh" ), 0 ), 600.f, 0.01f );
	EXPECT_NEAR( widget->convertLength( StyleSheetLength( "50vw" ), 0 ), 400.f, 0.01f );

	sceneNode->clearViewportPixelsSize();
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 3000 );
	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_NEAR( widget->getPixelsSize().getHeight(), 3000.f, 0.01f );
	EXPECT_NEAR( widget->convertLength( StyleSheetLength( "100vh" ), 0 ), 3000.f, 0.01f );

	Engine::destroySingleton();
}

UTEST( UISceneNode, MediaQueriesUseCssViewportPixelsAtPixelDensity2 ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Scene Media CSS Pixels Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 2.f, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	sceneNode->setViewportPixelsSize( Sizef( 800, 600 ) );

	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 600 );
	EXPECT_EQ( sceneNode->getMediaFeatures().width, 400 );
	EXPECT_EQ( sceneNode->getMediaFeatures().height, 300 );

	UIWidget* widget = UIWidget::New();
	widget->setId( "media-css-pixels-target" );
	widget->setParent( sceneNode->getRoot() );
	sceneNode->setStyleSheet( R"css(
#media-css-pixels-target { background-color: #ff0000; }
@media only screen and (min-width: 300px) and (max-width: 500px) {
	#media-css-pixels-target { background-color: #00ff00; }
}
)css" );

	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#00ff00" ) );

	Engine::destroySingleton();
	EE::Graphics::PixelDensity::setPixelDensity( 1.0f );
}

UTEST( UISceneNode, EmbeddedSceneCanKeepContentDrivenExtent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Embedded Scene Extent Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* host = UIWidget::New();
	host->setPixelsSize( 800, 600 );
	host->setParent( hostScene->getRoot() );

	UISceneNode* embeddedScene = UISceneNode::New();
	embeddedScene->setFollowParentSize( false );
	embeddedScene->setPixelsSize( 800, 3000 );
	embeddedScene->setParent( host );

	EXPECT_FALSE( embeddedScene->followsParentSize() );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 3000 );

	host->setPixelsSize( 700, 500 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 3000 );

	embeddedScene->setFollowParentSize( true );
	EXPECT_TRUE( embeddedScene->followsParentSize() );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 700 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 500 );

	host->setPixelsSize( 640, 480 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 640 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 480 );

	Engine::destroySingleton();
}

UTEST( UISceneNode, LayoutViewportRootTraversesDocumentExtentForHitTesting ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Embedded Scene Hit Test Bounds",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* host = UIWidget::New();
	host->setPixelsSize( 800, 600 );
	host->setParent( hostScene->getRoot() );

	UISceneNode* embeddedScene = UISceneNode::New();
	embeddedScene->setFollowParentSize( false );
	embeddedScene->setParent( host );
	embeddedScene->setPixelsSize( 800, 1000 );
	embeddedScene->setLayoutViewportPixelsSize( Sizef( 800, 300 ) );

	UIWidget* target = UIWidget::New();
	target->setId( "below-layout-viewport" );
	target->setPixelsSize( 100, 50 );
	target->setPosition( 10, 900 );
	target->setParent( embeddedScene->getRoot() );

	UIRoot* embeddedRoot = embeddedScene->getRoot()->asType<UIRoot>();
	ASSERT_TRUE( embeddedRoot != nullptr );
	ASSERT_TRUE( embeddedRoot->hasChildHitTestTraversalPixelsSize() );
	EXPECT_NEAR( embeddedScene->getRoot()->getPixelsSize().getHeight(), 300.f, 0.5f );
	EXPECT_NEAR( embeddedRoot->getChildHitTestTraversalPixelsSize().getHeight(), 1000.f, 0.5f );

	Node* hit = embeddedScene->overFind( Vector2f( 20, 920 ) );
	EXPECT_EQ( hit, target );

	embeddedScene->clearLayoutViewportPixelsSize();
	EXPECT_FALSE( embeddedRoot->hasChildHitTestTraversalPixelsSize() );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneOwnsDescendantSceneAndStyles ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Nested Scene Isolation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostScene->getRoot() );

	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setParent( hostWidget );
	AccessibilityManager* hostAccessibility = hostScene->getAccessibilityManager();
	EXPECT_TRUE( nestedScene->getAccessibilityManager() == hostAccessibility );
	UIWidget* nestedWidget = UIWidget::New();
	nestedWidget->setClass( "shared-name" );
	nestedWidget->setParent( nestedScene->getRoot() );
	UIWidget* hostStyledWidget = UIWidget::New();
	hostStyledWidget->setClass( "shared-name" );
	hostStyledWidget->setParent( hostScene->getRoot() );

	EXPECT_TRUE( nestedWidget->getUISceneNode() == nestedScene );
	EXPECT_TRUE( nestedWidget->getSceneNode() == nestedScene );

	hostScene->setStyleSheet( ".shared-name { background-color: #ff0000; }" );
	nestedScene->setStyleSheet( ".shared-name { background-color: #0000ff; }" );

	EXPECT_TRUE( hostStyledWidget->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_TRUE( nestedWidget->getBackgroundColor() == Color( "#0000ff" ) );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneRebindsHostServicesOnSceneChange ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Nested Scene Rebind Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostSceneA = init_test_scene_node();
	UISceneNode* hostSceneB = UISceneNode::New();
	SceneManager::instance()->add( hostSceneB );
	hostSceneA->setColorSchemePreference( ColorSchemePreference::Light );
	hostSceneB->setColorSchemePreference( ColorSchemePreference::Dark );
	hostSceneB->getUIThemeManager()->setDefaultFontSize( 42.f );

	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostSceneA->getRoot() );
	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setParent( hostWidget );
	UIWidget* nestedHostWidget = UIWidget::New();
	nestedHostWidget->setParent( nestedScene->getRoot() );
	UISceneNode* nestedChildScene = UISceneNode::New();
	nestedChildScene->setParent( nestedHostWidget );

	EXPECT_TRUE( nestedScene->getEventDispatcher() == hostSceneA->getEventDispatcher() );
	EXPECT_EQ( nestedScene->getColorSchemePreference(), ColorSchemePreference::Light );
	EXPECT_EQ( hostSceneA->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( hostSceneA->getChildUISceneNodes()[0] == nestedScene );
	EXPECT_EQ( nestedScene->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( nestedScene->getChildUISceneNodes()[0] == nestedChildScene );

	hostSceneA->setHighlightOverRecursive( true );
	hostSceneA->setHighlightFocusRecursive( true );
	hostSceneA->setDrawBoxesRecursive( true );
	hostSceneA->setDrawDebugDataRecursive( true );

	EXPECT_TRUE( hostSceneA->getHighlightOver() );
	EXPECT_TRUE( hostSceneA->getHighlightFocus() );
	EXPECT_TRUE( hostSceneA->getDrawBoxes() );
	EXPECT_TRUE( hostSceneA->getDrawDebugData() );
	EXPECT_TRUE( nestedScene->getHighlightOver() );
	EXPECT_TRUE( nestedScene->getHighlightFocus() );
	EXPECT_TRUE( nestedScene->getDrawBoxes() );
	EXPECT_TRUE( nestedScene->getDrawDebugData() );
	EXPECT_TRUE( nestedChildScene->getHighlightOver() );
	EXPECT_TRUE( nestedChildScene->getHighlightFocus() );
	EXPECT_TRUE( nestedChildScene->getDrawBoxes() );
	EXPECT_TRUE( nestedChildScene->getDrawDebugData() );

	nestedScene->setHighlightOverRecursive( false );
	nestedScene->setHighlightFocusRecursive( false );
	nestedScene->setDrawBoxesRecursive( false );
	nestedScene->setDrawDebugDataRecursive( false );

	EXPECT_TRUE( hostSceneA->getHighlightOver() );
	EXPECT_TRUE( hostSceneA->getHighlightFocus() );
	EXPECT_TRUE( hostSceneA->getDrawBoxes() );
	EXPECT_TRUE( hostSceneA->getDrawDebugData() );
	EXPECT_FALSE( nestedScene->getHighlightOver() );
	EXPECT_FALSE( nestedScene->getHighlightFocus() );
	EXPECT_FALSE( nestedScene->getDrawBoxes() );
	EXPECT_FALSE( nestedScene->getDrawDebugData() );
	EXPECT_FALSE( nestedChildScene->getHighlightOver() );
	EXPECT_FALSE( nestedChildScene->getHighlightFocus() );
	EXPECT_FALSE( nestedChildScene->getDrawBoxes() );
	EXPECT_FALSE( nestedChildScene->getDrawDebugData() );

	hostWidget->setParent( hostSceneB->getRoot() );

	EXPECT_TRUE( nestedScene->getEventDispatcher() == hostSceneB->getEventDispatcher() );
	EXPECT_EQ( nestedScene->getColorSchemePreference(), ColorSchemePreference::Dark );
	EXPECT_EQ( nestedScene->getUIThemeManager()->getDefaultFontSize(), 42.f );
	EXPECT_TRUE( nestedScene->getRoot()->getUISceneNode() == nestedScene );
	EXPECT_TRUE( hostSceneA->getChildUISceneNodes().empty() );
	EXPECT_EQ( hostSceneB->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( hostSceneB->getChildUISceneNodes()[0] == nestedScene );
	EXPECT_EQ( nestedScene->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( nestedScene->getChildUISceneNodes()[0] == nestedChildScene );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneMediaUsesExplicitViewport ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Nested Scene Media Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostScene->getRoot() );

	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setFollowParentSize( false );
	nestedScene->setPixelsSize( 800, 3000 );
	nestedScene->setViewportPixelsSize( Sizef( 800, 600 ) );
	nestedScene->setParent( hostWidget );

	UIWidget* nestedWidget = UIWidget::New();
	nestedWidget->setId( "media-target" );
	nestedWidget->setParent( nestedScene->getRoot() );
	nestedScene->setStyleSheet( R"css(
#media-target { background-color: #ff0000; }
@media screen and (max-height: 700px) {
	#media-target { background-color: #00ff00; }
}
)css" );

	EXPECT_EQ( nestedScene->getMediaFeatures().height, 600 );
	EXPECT_TRUE( nestedWidget->getBackgroundColor() == Color( "#00ff00" ) );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneInvalidationPropagatesToHostScene ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "Nested Scene Invalidation Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	hostScene->enableDrawInvalidation();

	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostScene->getRoot() );
	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setParent( hostWidget );
	UIWidget* nestedWidget = UIWidget::New();
	nestedWidget->setParent( nestedScene->getRoot() );

	win->getInput()->update();
	SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
	win->clear();
	SceneManager::instance()->draw();
	win->display();

	ASSERT_FALSE( hostScene->invalidated() );
	ASSERT_FALSE( nestedScene->invalidated() );

	nestedWidget->invalidateDraw();

	EXPECT_TRUE( nestedScene->invalidated() );
	EXPECT_TRUE( hostScene->invalidated() );

	Engine::destroySingleton();
}

UTEST( UISceneNode, StyleStateUpdateAllowsWidgetCreation ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Style State Widget Creation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	UIFileDialog* dialog = UIFileDialog::New();
	dialog->setParent( sceneNode->getRoot() );

	// Applying the select-button state creates its UIImage icon. The new widget invalidates style
	// state while the current dirty-state pass is still being processed.
	sceneNode->flushDirtyStyleAndLayout();

	EXPECT_TRUE( dialog->getButtonOpen() != nullptr );

	Engine::destroySingleton();
}

class InvalidationTestLayout : public UILayout {
  public:
	InvalidationTestLayout() : UILayout( "invalidation-test" ) {}

	void updateLayout() override {
		lastReasons = mCurrentLayoutReasons;
		mDirtyLayout = false;
	}

	LayoutInvalidationFlags lastReasons{ 0 };
};

UTEST( UISceneNode, WebViewDeletionDoesNotLeaveHostLayoutWork ) {
	Engine::instance()->createWindow( WindowSettings( 320, 240, "WebView Dirty Queue Ownership",
													  WindowStyle::Default | WindowStyle::Hidden,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* host = InvalidationTestSceneNode::New();
	init_test_scene_node( host );
	host->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	host->flushDirtyStyleAndLayout();

	auto* webView = UIWebView::New();
	auto* document = webView->getDocumentSceneNode()->getRoot()->findByTag( "webview::doc" );
	ASSERT_TRUE( document != nullptr );
	ASSERT_TRUE( document->isLayout() );
	auto* layout = document->asType<UILayout>();
	EXPECT_EQ( layout->getUISceneNode(), webView->getDocumentSceneNode() );
	EXPECT_FALSE( host->hasPendingLayout( layout ) );
	EXPECT_FALSE( host->hasPendingStyle( layout ) );
	EXPECT_FALSE( host->hasPendingStyleState( layout ) );
	EXPECT_FALSE( host->hasPendingStyleStateAnimations( layout ) );

	eeDelete( webView );
	// Compare queue membership only: the document pointer is no longer dereferenceable.
	const bool staleLayout = host->hasPendingLayout( layout );
	EXPECT_FALSE( staleLayout );
	EXPECT_FALSE( host->hasPendingStyle( layout ) );
	EXPECT_FALSE( host->hasPendingStyleState( layout ) );
	EXPECT_FALSE( host->hasPendingStyleStateAnimations( layout ) );
	// Keep the ownership failure readable when deliberately testing without the fix.
	if ( !staleLayout ) {
		auto* input = UIHTMLInput::New();
		EXPECT_EQ( input->getUISceneNode(), host );
		EXPECT_TRUE( input->getChildWidget() != nullptr );
		host->flushDirtyStyleAndLayout();
	}

	Engine::destroySingleton();
}

UTEST( UISceneNode, DirtyLayoutAndStyleWorkFollowsSceneRebinding ) {
	Engine::instance()->createWindow( WindowSettings( 320, 240, "Dirty Queue Scene Rebinding",
													  WindowStyle::Default | WindowStyle::Hidden,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* host = InvalidationTestSceneNode::New();
	init_test_scene_node( host );
	host->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
	auto* embedded = InvalidationTestSceneNode::New();
	embedded->setParent( host->getRoot() );
	host->setStyleSheet( "* { opacity: 1; }" );
	auto* layout = eeNew( InvalidationTestLayout, () );
	auto* child = UIWidget::New();
	child->setParent( layout );
	host->flushDirtyStyleAndLayout();
	embedded->flushDirtyStyleAndLayout();

	layout->setLayoutDirty( LayoutInvalidation::TextFormatting );
	host->invalidateStyle( layout );
	host->invalidateStyleState( layout, true );
	host->invalidateStyle( child );
	host->invalidateStyleState( child, true );
	EXPECT_TRUE( host->hasPendingLayout( layout ) );
	layout->setParent( embedded->getRoot() );
	EXPECT_EQ( layout->getUISceneNode(), embedded );
	EXPECT_FALSE( host->hasPendingLayout( layout ) );
	EXPECT_FALSE( host->hasPendingStyle( layout ) );
	EXPECT_FALSE( host->hasPendingStyleState( layout ) );
	EXPECT_FALSE( host->hasPendingStyleStateAnimations( layout ) );
	EXPECT_FALSE( host->hasPendingStyle( child ) );
	EXPECT_FALSE( host->hasPendingStyleState( child ) );
	EXPECT_FALSE( host->hasPendingStyleStateAnimations( child ) );
	EXPECT_TRUE( embedded->hasPendingLayout( layout ) );
	EXPECT_TRUE( embedded->hasPendingStyle( layout ) );
	EXPECT_TRUE( embedded->hasPendingStyleState( layout ) );
	EXPECT_TRUE( embedded->hasPendingStyleStateAnimations( layout ) );

	embedded->flushDirtyStyleAndLayout();
	EXPECT_TRUE( layout->lastReasons & LayoutInvalidation::TextFormatting );
	EXPECT_FALSE( layout->isLayoutDirty() );
	layout->setLayoutDirty( LayoutInvalidation::TextFormatting );
	layout->setParent( host->getRoot() );
	EXPECT_FALSE( embedded->hasPendingLayout( layout ) );
	EXPECT_FALSE( embedded->hasPendingStyle( layout ) );
	EXPECT_FALSE( embedded->hasPendingStyleState( layout ) );
	EXPECT_TRUE( host->hasPendingLayout( layout ) );
	host->updateDirtyLayouts();
	EXPECT_TRUE( layout->lastReasons & LayoutInvalidation::TextFormatting );
	EXPECT_FALSE( layout->isLayoutDirty() );

	// Coalescing into a dirty parent must not strand a child moved into another scene.
	auto* parent = eeNew( InvalidationTestLayout, () );
	layout->setParent( parent );
	host->flushDirtyStyleAndLayout();
	layout->setLayoutDirty( LayoutInvalidation::TextFormatting );
	parent->setLayoutDirty( LayoutInvalidation::Self );
	EXPECT_FALSE( host->hasPendingLayout( layout ) );
	EXPECT_TRUE( layout->isLayoutDirty() );
	layout->setParent( embedded->getRoot() );
	EXPECT_TRUE( embedded->hasPendingLayout( layout ) );
	embedded->updateDirtyLayouts();
	EXPECT_TRUE( layout->lastReasons & LayoutInvalidation::TextFormatting );
	EXPECT_FALSE( layout->isLayoutDirty() );
	eeDelete( layout );
	host->flushDirtyStyleAndLayout();
	embedded->flushDirtyStyleAndLayout();

	Engine::destroySingleton();
}

UTEST( UISceneNode, LeafLayoutInvalidationPreservesSiblingRootsAndDescendantReasons ) {
	Engine::instance()->createWindow( WindowSettings( 320, 240, "Leaf Layout Invalidation",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	auto* sceneNode = InvalidationTestSceneNode::New();
	init_test_scene_node( sceneNode );
	auto* first = eeNew( InvalidationTestLayout, () );
	first->setParent( sceneNode->getRoot() );
	auto* second = eeNew( InvalidationTestLayout, () );
	second->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();

	sceneNode->invalidateLayout( first, LayoutInvalidation::Self );
	sceneNode->invalidateLayout( second, LayoutInvalidation::TextFormatting );
	EXPECT_EQ( size_t{ 2 }, sceneNode->pendingLayoutCount() );
	sceneNode->updateDirtyLayouts();
	EXPECT_EQ( LayoutInvalidation::Self, first->lastReasons );
	EXPECT_EQ( LayoutInvalidation::TextFormatting, second->lastReasons );

	// Once a leaf gains a child, ancestor coalescing must still retain the child's reasons.
	auto* child = eeNew( InvalidationTestLayout, () );
	child->setParent( first );
	sceneNode->flushDirtyStyleAndLayout();
	sceneNode->invalidateLayout( child, LayoutInvalidation::TextFormatting );
	sceneNode->invalidateLayout( second, LayoutInvalidation::Self );
	sceneNode->invalidateLayout( first, LayoutInvalidation::Document );
	EXPECT_EQ( size_t{ 2 }, sceneNode->pendingLayoutCount() );
	sceneNode->updateDirtyLayouts();
	EXPECT_EQ( LayoutInvalidation::TextFormatting | LayoutInvalidation::Document,
			   first->lastReasons );
	EXPECT_EQ( LayoutInvalidation::Self, second->lastReasons );

	Engine::destroySingleton();
}

UTEST( UISceneNode, StyleInvalidationCoalescesAtProcessingTime ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Deferred Style Invalidation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	auto* sceneNode = InvalidationTestSceneNode::New();
	init_test_scene_node( sceneNode );
	sceneNode->flushDirtyStyleAndLayout();

	UIWidget* parent = UIWidget::New();
	parent->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();

	constexpr size_t childCount = 2048;
	UIWidget* firstChild = nullptr;
	for ( size_t i = 0; i < childCount; ++i ) {
		UIWidget* child = UIWidget::New();
		child->setParent( parent );
		if ( !firstChild )
			firstChild = child;
	}

	EXPECT_EQ( childCount, sceneNode->pendingStyleCount() );
	EXPECT_EQ( childCount, sceneNode->pendingStyleStateCount() );

	sceneNode->invalidateStyle( parent );
	sceneNode->invalidateStyleState( parent, true );
	EXPECT_EQ( childCount + 1, sceneNode->pendingStyleCount() );
	EXPECT_EQ( childCount + 1, sceneNode->pendingStyleStateCount() );

	sceneNode->updateDirtyStyles();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleCount() );

	sceneNode->updateDirtyStyleStates();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_TRUE( sceneNode->processedStyleRootDisablesAnimations( 0 ) );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleStateCount() );

	// The existing ancestor fast path still avoids queueing descendants when the parent arrives
	// first, and the parent's animation policy governs the recursive state update.
	sceneNode->invalidateStyle( parent );
	sceneNode->invalidateStyle( firstChild );
	sceneNode->invalidateStyleState( parent, false );
	sceneNode->invalidateStyleState( firstChild, true );
	EXPECT_EQ( size_t{ 1 }, sceneNode->pendingStyleCount() );
	EXPECT_EQ( size_t{ 1 }, sceneNode->pendingStyleStateCount() );
	sceneNode->updateDirtyStyles();
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	sceneNode->updateDirtyStyleStates();
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_FALSE( sceneNode->processedStyleRootDisablesAnimations( 0 ) );

	// Reparenting an already-dirty widget under a dirty parent can leave both entries queued. The
	// current tree must decide which root gets processed.
	UIWidget* reparentTarget = UIWidget::New();
	reparentTarget->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();
	sceneNode->invalidateStyle( firstChild );
	sceneNode->invalidateStyleState( firstChild );
	sceneNode->invalidateStyle( reparentTarget );
	sceneNode->invalidateStyleState( reparentTarget, true );
	firstChild->setParent( reparentTarget );
	sceneNode->updateDirtyStyles();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( reparentTarget, sceneNode->processedStyleRoot( 0 ) );
	sceneNode->updateDirtyStyleStates();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( reparentTarget, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_TRUE( sceneNode->processedStyleRootDisablesAnimations( 0 ) );

	UIWidget* deletedWidget = UIWidget::New();
	deletedWidget->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();
	sceneNode->invalidateStyleState( deletedWidget, true );
	EXPECT_EQ( size_t{ 1 }, sceneNode->pendingStyleStateAnimationCount() );
	eeDelete( deletedWidget );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleStateCount() );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleStateAnimationCount() );

	Engine::destroySingleton();
}

UTEST( UIWindow, ShadowSurvivesRepeatedTranslucentDraws ) {
	for ( Float density : { 1.f, 1.5f } ) {
		UIApplication app( WindowSettings( 320, 240, "Window shadow opacity", WindowStyle::Default,
										   WindowBackend::Default, 32, {}, 1, false, true ),
						   UIApplication::Settings(
							   Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), density ) );
		auto* scene = app.getUI();
		scene->getUIThemeManager()->setDefaultEffectsEnabled( false );
		scene->loadLayoutFromString( R"xml(
			<window id="shadow-test" x="30dp" y="30dp" width="60dp" height="40dp"
				window-flags="borderless|shadow" background-color="white"
				window-shadow-color="#00000080" window-shadow-size="8dp"
				window-shadow-offset="0dp 0dp" />
		)xml" );
		scene->flushDirtyStyleAndLayout();
		scene->getRoot()->setBackgroundColor( Color::White );
		auto* window = scene->find( "shadow-test" )->asType<UIWindow>();
		window->setVisible( true );
		const auto* shadowColor = CSS::StyleSheetSpecification::instance()->getProperty(
			CSS::PropertyId::WindowShadowColor );
		// Exercise fade-in and fade-out frames without timer or inspector-driven redraws.
		for ( Uint8 alpha : { 16, 64, 128, 192, 255, 128, 255 } ) {
			window->setAlpha( alpha );
			app.getWindow()->clear();
			scene->draw();
			EXPECT_STDSTREQ( "#00000080", window->getPropertyString( shadowColor ) );
		}
		auto pixels = app.getWindow()->getFrontBufferImage();
		const auto bounds = window->getWorldBounds();
		const auto y = static_cast<unsigned int>( bounds.getCenter().y );
		const auto nearX = static_cast<unsigned int>( bounds.Left - PixelDensity::dpToPx( 4.f ) );
		const auto farX = static_cast<unsigned int>( bounds.Left - PixelDensity::dpToPx( 16.f ) );
		EXPECT_LT( pixels.getPixel( nearX, y ).r, pixels.getPixel( farX, y ).r );
	}
}

UTEST( UIWindow, ModalWindowStopsKeyBindingsFromReachingScene ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Modal Key Binding Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	int executedCommands = 0;
	sceneNode->setKeyBindingCommand( "global-command",
									 [&executedCommands] { executedCommands++; } );
	sceneNode->addKeyBinding( { KEY_F, KEYMOD_LMETA | KEYMOD_LSHIFT }, "global-command" );

	UIWindow* window = UIWindow::New();
	window->setParent( sceneNode->getRoot() );
	sceneNode->getEventDispatcher()->setFocusNode( window->getContainer() );

	sceneNode->getEventDispatcher()->sendKeyDown( KEY_F, SCANCODE_UNKNOWN, 0,
												  KEYMOD_LMETA | KEYMOD_LSHIFT );
	EXPECT_EQ( executedCommands, 1 );

	window->setWindowFlags( window->getWinFlags() | UI_WIN_MODAL );
	sceneNode->getEventDispatcher()->sendKeyDown( KEY_F, SCANCODE_UNKNOWN, 0,
												  KEYMOD_LMETA | KEYMOD_LSHIFT );
	EXPECT_EQ( executedCommands, 1 );

	Engine::destroySingleton();
}

UTEST( UISceneNode, CookieJarIsPrivateByDefaultAndReplaceable ) {
	UIApplication app(
		WindowSettings{ 320, 240, "Shared cookie jars" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1.f ) );
	auto* first = app.getUI();
	auto* second = UISceneNode::New();
	ASSERT_TRUE( &first->getCookieManager() != &second->getCookieManager() );
	first->getCookieManager().storeCookiesFromHeader( "example.com", "private=value" );
	EXPECT_TRUE( second->getCookieManager().getCookieHeader( "example.com" ).empty() );

	auto shared = std::make_shared<CookieManager>();
	first->setCookieManager( shared );
	second->setCookieManager( shared );
	EXPECT_TRUE( &first->getCookieManager() == shared.get() );
	EXPECT_TRUE( &second->getCookieManager() == shared.get() );
	first->getCookieManager().storeCookiesFromHeader( "example.com", "shared=value" );
	EXPECT_TRUE( second->getCookieManager().getCookieHeader( "example.com" ) == "shared=value" );
	first->setCookieManager( nullptr );
	EXPECT_TRUE( &first->getCookieManager() != shared.get() );
	EXPECT_TRUE( first->getCookieManager().getCookieHeader( "example.com" ).empty() );
	EXPECT_TRUE( second->getCookieManager().getCookieHeader( "example.com" ) == "shared=value" );
	eeDelete( second );
}
