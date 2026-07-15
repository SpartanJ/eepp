#include "utest.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <eepp/core/lrucache.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/textlayout.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/resourceloader.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/variant.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::Window;

namespace {

struct LoaderGate {
	std::mutex mutex;
	std::condition_variable condition;
	bool started{ false };
	bool release{ false };

	void run() {
		std::unique_lock<std::mutex> lock( mutex );
		started = true;
		condition.notify_all();
		condition.wait( lock, [this] { return release; } );
	}

	void waitUntilStarted() {
		std::unique_lock<std::mutex> lock( mutex );
		condition.wait( lock, [this] { return started; } );
	}

	void finish() {
		std::unique_lock<std::mutex> lock( mutex );
		release = true;
		condition.notify_all();
	}
};

class TestTextureAtlas : public TextureAtlas {
  public:
	using TextureAtlas::setTextures;
};

class TestTextureAtlasLoader : public TextureAtlasLoader {
  public:
	void setTextureAtlas( TextureAtlas* textureAtlas ) { mTextureAtlas = textureAtlas; }

	void loadDelayed( const std::shared_ptr<LoaderGate>& gate,
					  const std::shared_ptr<std::atomic<bool>>& callbackCompleted ) {
		mRL.setThreaded( true );
		mRL.add( [gate] { gate->run(); } );
		mRL.add( [] {} );
		mRL.load( [this, callbackCompleted]( ResourceLoader* ) {
			mTempAtlass.emplace_back();
			*callbackCompleted = true;
		} );
	}
};

class AsyncDeliveryProducerScene : public UISceneNode {
  public:
	static AsyncDeliveryProducerScene* New( EE::Window::Window* window ) {
		return eeNew( AsyncDeliveryProducerScene, ( window ) );
	}

	~AsyncDeliveryProducerScene() override {
		mStop.store( true, std::memory_order_release );
		if ( mProducer.joinable() )
			mProducer.join();
	}

	void startProducing( std::shared_ptr<int> retained,
						 const std::shared_ptr<std::atomic<int>>& executed ) {
		auto resourceState = getAsyncResourceLoadState();
		const Uint64 generation = resourceState->generation.load( std::memory_order_acquire );
		mProducer = std::thread( [this, resourceState = std::move( resourceState ), generation,
								  retained = std::move( retained ), executed] {
			while ( !mStop.load( std::memory_order_acquire ) ) {
				UISceneNode::runAsyncResourceOnMainThread(
					resourceState, generation, [retained, executed]( UISceneNode* ) {
						(void)retained;
						executed->fetch_add( 1, std::memory_order_relaxed );
					} );
				if ( mSubmitted.fetch_add( 1, std::memory_order_release ) == 0 ) {
					std::lock_guard<std::mutex> lock( mSubmittedMutex );
					mSubmittedCondition.notify_all();
				}
				std::this_thread::yield();
			}
		} );
	}

	void waitUntilSubmitted() {
		std::unique_lock<std::mutex> lock( mSubmittedMutex );
		mSubmittedCondition.wait(
			lock, [this] { return mSubmitted.load( std::memory_order_acquire ) != 0; } );
	}

  protected:
	explicit AsyncDeliveryProducerScene( EE::Window::Window* window ) : UISceneNode( window ) {}

  private:
	std::atomic<bool> mStop{ false };
	std::atomic<Uint32> mSubmitted{ 0 };
	std::mutex mSubmittedMutex;
	std::condition_variable mSubmittedCondition;
	std::thread mProducer;
};

EE::Window::Window* createLifecycleTestWindow( const std::string& title ) {
	return Engine::instance()->createWindow(
		WindowSettings( 64, 64, title, WindowStyle::Default, WindowBackend::Default, 32, {}, 1,
						false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
}

} // namespace

static_assert( !std::is_copy_constructible<Texture>::value, "Texture must not be copyable" );
static_assert( !std::is_copy_assignable<Texture>::value, "Texture must not be copy-assignable" );

UTEST( ResourcePrerequisites, lruCacheEvictsOnlyLeastRecentlyUsedAndReleasesKeys ) {
	LRUCache<2, int, bool> recencyCache;
	recencyCache.put( 1, true );
	recencyCache.put( 2, false );
	ASSERT_TRUE( recencyCache.get( 1 ).has_value() );
	recencyCache.put( 3, true );

	EXPECT_TRUE( !recencyCache.get( 2 ).has_value() );
	EXPECT_TRUE( recencyCache.get( 1 ).has_value() );
	EXPECT_TRUE( recencyCache.get( 3 ).has_value() );

	auto ownedKey = std::make_shared<int>( 1 );
	std::weak_ptr<int> weakKey = ownedKey;
	LRUCache<2, std::shared_ptr<int>, bool> owningKeyCache;
	static_assert( decltype( owningKeyCache )::is_static() );
	owningKeyCache.put( ownedKey, true );
	ownedKey.reset();
	ASSERT_TRUE( !weakKey.expired() );

	owningKeyCache.clear();
	EXPECT_TRUE( weakKey.expired() );
}

UTEST( ResourcePrerequisites, textureAtlasLoaderAppliesFilterToEveryTexture ) {
	Engine::instance()->createWindow( WindowSettings( 64, 64, "TextureAtlasLoader filter test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	{
		Texture* first = TextureFactory::instance()->createEmptyTexture( 1, 1 );
		Texture* second = TextureFactory::instance()->createEmptyTexture( 1, 1 );
		ASSERT_TRUE( first != NULL );
		ASSERT_TRUE( second != NULL );

		TestTextureAtlas atlas;
		atlas.setTextures( { first, second } );

		TestTextureAtlasLoader loader;
		loader.setTextureAtlas( &atlas );
		loader.setTextureFilter( Texture::Filter::Nearest );

		EXPECT_EQ( first->getFilter(), Texture::Filter::Nearest );
		EXPECT_EQ( second->getFilter(), Texture::Filter::Nearest );
	}

	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, textureAtlasLoaderAcceptsFilterBeforeAtlasExists ) {
	TestTextureAtlasLoader loader;
	loader.setTextureFilter( Texture::Filter::Nearest );
	EXPECT_EQ( static_cast<Texture::Filter>( loader.getTextureAtlasHeader().TextureFilter ),
			   Texture::Filter::Nearest );
}

UTEST( ResourcePrerequisites, textureRegistryTracksStableIdentityAndMemoryWithoutOwning ) {
	createLifecycleTestWindow( "Texture registry identity test" );
	TextureFactory* factory = TextureFactory::instance();
	Texture* first = factory->createEmptyTexture( 2, 3, 4, Color::Transparent, false,
												  Texture::ClampMode::ClampToEdge, false, false,
												  "registry-first" );
	Texture* second = factory->createEmptyTexture( 1, 1, 4, Color::Transparent, false,
												   Texture::ClampMode::ClampToEdge, false, false,
												   "registry-second" );
	ASSERT_TRUE( first != nullptr );
	ASSERT_TRUE( second != nullptr );

	const ResourceId firstId = first->getTextureId();
	const ResourceId secondId = second->getTextureId();
	EXPECT_TRUE( static_cast<bool>( firstId ) );
	EXPECT_TRUE( static_cast<bool>( secondId ) );
	EXPECT_TRUE( firstId != secondId );

	first->setName( "registry-first-renamed" );
	TextureRegistrySnapshot snapshot = factory->snapshotTextures();
	auto firstRecord =
		std::find_if( snapshot.begin(), snapshot.end(),
					  [firstId]( const auto& record ) { return record.id == firstId; } );
	ASSERT_TRUE( firstRecord != snapshot.end() );
	EXPECT_TRUE( firstRecord->displayName == "registry-first-renamed" );
	ASSERT_TRUE( firstRecord->metrics != nullptr );
	EXPECT_EQ( firstRecord->metrics->getMemoryBytes(), static_cast<std::size_t>( 2 * 3 * 4 ) );
	EXPECT_EQ( factory->getTextureMemorySize(), 2u * 3u * 4u + 4u );
	first->resize( 4, 3 );
	EXPECT_EQ( firstRecord->metrics->getMemoryBytes(), static_cast<std::size_t>( 4 * 3 * 4 ) );
	EXPECT_EQ( factory->getTextureMemorySize(), 4u * 3u * 4u + 4u );

	TextureWeakPtr firstWeak = firstRecord->texture;
	TexturePtr retainedFirst = firstWeak.lock();
	ASSERT_TRUE( retainedFirst != nullptr );
	ASSERT_TRUE( factory->remove( first ) );
	EXPECT_FALSE( firstWeak.expired() );
	EXPECT_EQ( factory->getTextureMemorySize(), 4u * 3u * 4u + 4u );
	retainedFirst.reset();
	EXPECT_TRUE( firstWeak.expired() );

	snapshot = factory->snapshotTextures();
	EXPECT_TRUE( std::none_of( snapshot.begin(), snapshot.end(),
							   [firstId]( const auto& record ) { return record.id == firstId; } ) );
	EXPECT_EQ( factory->getTextureMemorySize(), 4u );

	Engine::destroySingleton();

	createLifecycleTestWindow( "Texture registry identity restart test" );
	Texture* afterRestart = TextureFactory::instance()->createEmptyTexture( 1, 1 );
	ASSERT_TRUE( afterRestart != nullptr );
	EXPECT_TRUE( secondId < afterRestart->getTextureId() );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, textureFinalReleaseWaitsForDisplayCollection ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Texture deferred release test" );
	TextureFactory* factory = TextureFactory::instance();
	Texture* texture = factory->createEmptyTexture( 2, 2 );
	ASSERT_TRUE( texture != nullptr );

	bool unloaded = false;
	texture->pushResourceChangeCallback(
		[&unloaded]( Uint32, DrawableResource::Event event, DrawableResource* ) {
			if ( event == DrawableResource::Event::Unload )
				unloaded = true;
		} );

	TextureRegistrySnapshot snapshot = factory->snapshotTextures();
	auto record = std::find_if( snapshot.begin(), snapshot.end(), [texture]( const auto& entry ) {
		return entry.id == texture->getTextureId();
	} );
	ASSERT_TRUE( record != snapshot.end() );
	TextureWeakPtr weakTexture = record->texture;
	TexturePtr retainedTexture = weakTexture.lock();
	ASSERT_TRUE( retainedTexture != nullptr );

	ASSERT_TRUE( factory->remove( texture ) );
	GlobalBatchRenderer::instance()->setTexture( texture );
	GlobalBatchRenderer::instance()->batchQuad( 0, 0, 2, 2 );
	retainedTexture.reset();

	EXPECT_TRUE( weakTexture.expired() );
	EXPECT_FALSE( unloaded );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 1 ) );

	window->display( false );

	EXPECT_TRUE( unloaded );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 0 ) );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, unsignedVariantPreservesTypeAndValue ) {
	const unsigned int value = std::numeric_limits<unsigned int>::max();
	Variant original( value );

	ASSERT_TRUE( original.is( Variant::Type::Uint ) );
	EXPECT_EQ( original.asUint(), value );
	EXPECT_TRUE( original.toString() == std::to_string( value ) );

	Variant copied( original );
	EXPECT_TRUE( copied.is( Variant::Type::Uint ) );
	EXPECT_EQ( copied.asUint(), value );

	Variant moved( std::move( copied ) );
	EXPECT_TRUE( moved.is( Variant::Type::Uint ) );
	EXPECT_EQ( moved.asUint(), value );
	EXPECT_TRUE( !copied.isValid() );

	Variant copyAssigned;
	copyAssigned = original;
	EXPECT_TRUE( copyAssigned.is( Variant::Type::Uint ) );
	EXPECT_EQ( copyAssigned.asUint(), value );

	Variant moveAssigned;
	moveAssigned = std::move( copyAssigned );
	EXPECT_TRUE( moveAssigned.is( Variant::Type::Uint ) );
	EXPECT_EQ( moveAssigned.asUint(), value );
	EXPECT_TRUE( !copyAssigned.isValid() );
}

UTEST( ResourcePrerequisites, emptyResourceLoaderHasDefinedProgress ) {
	ResourceLoader loader;
	loader.setThreaded( false );

	EXPECT_EQ( loader.getProgress(), 0.f );

	loader.load();

	EXPECT_TRUE( loader.isLoaded() );
	EXPECT_EQ( loader.getProgress(), 100.f );
}

UTEST( ResourcePrerequisites, resourceLoaderReportsPartialAndCompleteProgress ) {
	ResourceLoader loader;
	loader.setThreaded( false );
	Float progressDuringSecondTask = 0.f;

	loader.add( [] {} );
	loader.add( [&] { progressDuringSecondTask = loader.getProgress(); } );
	loader.load();

	EXPECT_EQ( progressDuringSecondTask, 50.f );
	EXPECT_EQ( loader.getProgress(), 100.f );
}

UTEST( ResourcePrerequisites, textureAtlasLoaderWaitsBeforeDestroyingCallbackState ) {
	auto gate = std::make_shared<LoaderGate>();
	auto callbackCompleted = std::make_shared<std::atomic<bool>>( false );
	auto loader = std::make_unique<TestTextureAtlasLoader>();
	loader->loadDelayed( gate, callbackCompleted );
	gate->waitUntilStarted();

	std::thread destroyThread( [loader = std::move( loader )]() mutable { loader.reset(); } );
	gate->finish();
	destroyThread.join();

	EXPECT_TRUE( *callbackCompleted );
}

UTEST( ResourcePrerequisites, asyncResourceDeliveriesArePurgedAcrossEngineRestarts ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Async delivery lifecycle test" );
	auto* scene = UISceneNode::New( window );
	SceneManager::instance()->add( scene );

	auto resourceState = scene->getAsyncResourceLoadState();
	const Uint64 generation = resourceState->generation.load( std::memory_order_acquire );
	std::atomic<int> executed{ 0 };
	auto immediateCapture = std::make_shared<int>( 1 );
	auto delayedCapture = std::make_shared<int>( 2 );
	std::weak_ptr<int> immediateCaptureWeak = immediateCapture;
	std::weak_ptr<int> delayedCaptureWeak = delayedCapture;

	std::thread producer( [resourceState, generation, immediateCapture, &executed] {
		UISceneNode::runAsyncResourceOnMainThread(
			resourceState, generation, [immediateCapture, &executed]( UISceneNode* ) {
				(void)immediateCapture;
				executed.fetch_add( 1, std::memory_order_relaxed );
			} );
	} );
	producer.join();

	UISceneNode::runAsyncResourceOnMainThread(
		resourceState, generation,
		[delayedCapture, &executed]( UISceneNode* ) {
			(void)delayedCapture;
			executed.fetch_add( 1, std::memory_order_relaxed );
		},
		Seconds( 60 ) );
	immediateCapture.reset();
	delayedCapture.reset();
	EXPECT_FALSE( immediateCaptureWeak.expired() );
	EXPECT_FALSE( delayedCaptureWeak.expired() );

	Engine::destroySingleton();

	EXPECT_TRUE( immediateCaptureWeak.expired() );
	EXPECT_TRUE( delayedCaptureWeak.expired() );
	EXPECT_EQ( executed.load( std::memory_order_relaxed ), 0 );

	window = createLifecycleTestWindow( "Async delivery lifecycle restart test" );
	auto* recreatedScene = UISceneNode::New( window );
	SceneManager::instance()->add( recreatedScene );
	recreatedScene->update( Time::Zero );

	EXPECT_EQ( executed.load( std::memory_order_relaxed ), 0 );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, asyncResourceQueueShutdownRacesProducerSafely ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Async delivery shutdown race test" );
	auto* scene = AsyncDeliveryProducerScene::New( window );
	SceneManager::instance()->add( scene );

	auto retained = std::make_shared<int>( 1 );
	std::weak_ptr<int> retainedWeak = retained;
	auto executed = std::make_shared<std::atomic<int>>( 0 );
	scene->startProducing( std::move( retained ), executed );
	scene->waitUntilSubmitted();

	Engine::destroySingleton();

	EXPECT_TRUE( retainedWeak.expired() );
	EXPECT_EQ( executed->load( std::memory_order_relaxed ), 0 );

	window = createLifecycleTestWindow( "Async delivery shutdown race restart test" );
	auto* recreatedScene = UISceneNode::New( window );
	SceneManager::instance()->add( recreatedScene );
	recreatedScene->update( Time::Zero );

	EXPECT_EQ( executed->load( std::memory_order_relaxed ), 0 );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, engineTeardownReleasesGraphicsBeforeContextsAcrossRestarts ) {
	for ( int cycle = 0; cycle < 2; ++cycle ) {
		createLifecycleTestWindow( "Engine teardown test" );

		Texture* texture = TextureFactory::instance()->createEmptyTexture( 4, 4 );
		ASSERT_TRUE( texture != nullptr );

		NinePatch* ninePatch = NinePatchManager::instance()->add(
			NinePatch::New( texture, 1, 1, 1, 1, 1, "engine-teardown-nine-patch" ) );
		ASSERT_TRUE( ninePatch != nullptr );

		auto* scene = UISceneNode::New();
		SceneManager::instance()->add( scene );
		scene->enableFrameBuffer();
		UIImage::New()->setDrawable( ninePatch )->setParent( scene->getRoot() );

		auto* font = FontTrueType::New( "engine-teardown-font" );
		ASSERT_TRUE(
			font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );
		auto layout = TextLayout::layout( String( "cached before Engine teardown" ), font, 14, 0 );
		std::weak_ptr<const TextLayout> layoutWeak = layout;
		layout.reset();

		auto* batch = GlobalBatchRenderer::instance();
		batch->setTexture( texture );
		batch->batchQuad( 0, 0, 4, 4 );

		Engine::destroySingleton();

		EXPECT_TRUE( layoutWeak.expired() );
		EXPECT_TRUE( Engine::existsSingleton() == nullptr );
		EXPECT_TRUE( SceneManager::existsSingleton() == nullptr );
		EXPECT_TRUE( GlobalBatchRenderer::existsSingleton() == nullptr );
		EXPECT_TRUE( NinePatchManager::existsSingleton() == nullptr );
		EXPECT_TRUE( FontManager::existsSingleton() == nullptr );
		EXPECT_TRUE( TextureAtlasManager::existsSingleton() == nullptr );
		EXPECT_TRUE( TextureFactory::existsSingleton() == nullptr );
		EXPECT_TRUE( ShaderProgramManager::existsSingleton() == nullptr );
		EXPECT_TRUE( Graphics::Private::FrameBufferManager::existsSingleton() == nullptr );
		EXPECT_TRUE( Graphics::Private::VertexBufferManager::existsSingleton() == nullptr );
		EXPECT_TRUE( Renderer::existsSingleton() == nullptr );
	}
}
