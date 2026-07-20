#include "utest.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <eepp/core/lrucache.hpp>
#include <eepp/graphics/drawablegroup.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/rectangledrawable.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/resourcecatalog.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/scrollparallax.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/statelistdrawable.hpp>
#include <eepp/graphics/textlayout.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/resourceloader.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/models/variant.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextureregion.hpp>
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

class TestDrawableResource : public DrawableResource {
  public:
	TestDrawableResource() : DrawableResource( Drawable::CUSTOM ) {}

	Sizef getSize() { return {}; }
	Sizef getPixelsSize() { return {}; }
	void draw() {}
	void draw( const Vector2f& ) {}
	void draw( const Vector2f&, const Sizef& ) {}
	bool isStateful() { return false; }

	void notifyChange() { onResourceChange(); }
};

class CountingDrawable : public Drawable {
  public:
	CountingDrawable( std::shared_ptr<int> instanceCount ) :
		Drawable( Drawable::CUSTOM ), mInstanceCount( std::move( instanceCount ) ) {}

	Sizef getSize() { return { 16.f, 16.f }; }
	Sizef getPixelsSize() { return getSize(); }
	void draw() {}
	void draw( const Vector2f& ) {}
	void draw( const Vector2f&, const Sizef& ) {}
	bool isStateful() { return false; }

	DrawablePtr createInstance() const {
		++*mInstanceCount;
		auto instance = makeResource<CountingDrawable>( mInstanceCount );
		instance->setColor( mColor );
		instance->setPosition( mPosition );
		return instance;
	}

  private:
	std::shared_ptr<int> mInstanceCount;
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
		TexturePtr first = TextureFactory::instance()->createEmptyTexture( 1, 1 );
		TexturePtr second = TextureFactory::instance()->createEmptyTexture( 1, 1 );
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
	Uint8 firstPixels[2 * 3 * 4]{};
	TextureLoader firstLoader( firstPixels, 2, 3, 4, false, Texture::ClampMode::ClampToEdge, false,
							   false, "registry-first" );
	firstLoader.load();
	TexturePtr first = firstLoader.getTexture();
	TexturePtr second = factory->createEmptyTexture( 1, 1, 4, Color::Transparent, false,
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
	EXPECT_EQ( firstRecord->memoryBytes, static_cast<std::size_t>( 2 * 3 * 4 ) );
	EXPECT_EQ( factory->getTextureMemorySize(), 2u * 3u * 4u + 4u );
	first->resize( 4, 3 );
	snapshot = factory->snapshotTextures();
	firstRecord = std::find_if( snapshot.begin(), snapshot.end(),
								[firstId]( const auto& record ) { return record.id == firstId; } );
	ASSERT_TRUE( firstRecord != snapshot.end() );
	EXPECT_EQ( firstRecord->memoryBytes, static_cast<std::size_t>( 4 * 3 * 4 ) );
	EXPECT_EQ( factory->getTextureMemorySize(), 4u * 3u * 4u + 4u );

	TextureWeakPtr firstWeak = firstRecord->texture;
	TexturePtr retainedFirst = firstWeak.lock();
	ASSERT_TRUE( retainedFirst != nullptr );
	firstLoader.reset();
	first.reset();
	EXPECT_FALSE( firstWeak.expired() );
	EXPECT_EQ( factory->getTextureMemorySize(), 4u * 3u * 4u + 4u );
	retainedFirst.reset();
	EXPECT_TRUE( firstWeak.expired() );
	EXPECT_TRUE( factory->getTexture( firstId ) == nullptr );

	snapshot = factory->snapshotTextures();
	EXPECT_TRUE( std::none_of( snapshot.begin(), snapshot.end(),
							   [firstId]( const auto& record ) { return record.id == firstId; } ) );
	EXPECT_EQ( factory->getTextureMemorySize(), 4u );

	second.reset();
	Engine::destroySingleton();

	createLifecycleTestWindow( "Texture registry identity restart test" );
	TexturePtr afterRestart = TextureFactory::instance()->createEmptyTexture( 1, 1 );
	ASSERT_TRUE( afterRestart != nullptr );
	EXPECT_TRUE( secondId < afterRestart->getTextureId() );
	afterRestart.reset();
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, resourceCatalogOwnsPublishedTextures ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Resource catalog ownership test" );
	TextureFactory* factory = TextureFactory::instance();
	ResourceCatalogPtr catalog = ResourceCatalog::New();
	TexturePtr texture = factory->createEmptyTexture( 1, 1 );
	ASSERT_TRUE( texture != nullptr );
	TextureWeakPtr weakTexture = texture;

	catalog->publish( "catalog-texture", texture );
	catalog->publish( "catalog-texture-alias", texture );
	texture.reset();
	EXPECT_FALSE( weakTexture.expired() );
	EXPECT_TRUE( catalog->findTexture( "catalog-texture" ) != nullptr );

	EXPECT_TRUE( catalog->erase( "catalog-texture" ) );
	EXPECT_FALSE( weakTexture.expired() );
	EXPECT_TRUE( catalog->erase( "catalog-texture-alias" ) );
	EXPECT_TRUE( weakTexture.expired() );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 1 ) );
	window->display( false );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 0 ) );

	catalog.reset();
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, resourceScopesResolveOnlyLocalAndExplicitlyImportedCatalogs ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Resource scope isolation test" );
	TextureFactory* factory = TextureFactory::instance();
	ResourceScopePtr firstScope = ResourceScope::New();
	ResourceScopePtr secondScope = ResourceScope::New();
	ResourceCatalogPtr sharedCatalog = ResourceCatalog::New();
	ResourceCatalogPtr laterCatalog = ResourceCatalog::New();
	TexturePtr first = factory->createEmptyTexture( 1, 1 );
	TexturePtr second = factory->createEmptyTexture( 1, 1 );
	TexturePtr shared = factory->createEmptyTexture( 1, 1 );
	TexturePtr later = factory->createEmptyTexture( 1, 1 );
	TexturePtr observedOnly = factory->createEmptyTexture( 1, 1, 4, Color::Transparent, false,
														   Texture::ClampMode::ClampToEdge, false,
														   false, "observed-only" );
	ASSERT_TRUE( first != nullptr );
	ASSERT_TRUE( second != nullptr );
	ASSERT_TRUE( shared != nullptr );
	ASSERT_TRUE( later != nullptr );
	ASSERT_TRUE( observedOnly != nullptr );

	firstScope->publishLocal( "same-name", first );
	secondScope->publishLocal( "same-name", second );
	sharedCatalog->publish( "shared-name", shared );
	laterCatalog->publish( "shared-name", later );
	firstScope->importCatalog( sharedCatalog );
	firstScope->importCatalog( laterCatalog );

	EXPECT_EQ( first.get(), firstScope->findTexture( "same-name" ).get() );
	EXPECT_EQ( second.get(), secondScope->findTexture( "same-name" ).get() );
	EXPECT_EQ( shared.get(), firstScope->findTexture( "shared-name" ).get() );
	EXPECT_TRUE( secondScope->findTexture( "shared-name" ) == nullptr );
	EXPECT_TRUE( firstScope->findTexture( "observed-only" ) == nullptr );
	EXPECT_TRUE( secondScope->findTexture( "observed-only" ) == nullptr );
	EXPECT_TRUE( firstScope->removeCatalog( sharedCatalog ) );
	EXPECT_EQ( later.get(), firstScope->findTexture( "shared-name" ).get() );
	EXPECT_FALSE( firstScope->removeCatalog( sharedCatalog ) );

	TextureWeakPtr externallyRetainedWeak = first;
	TexturePtr externallyRetained = first;
	firstScope.reset();
	EXPECT_FALSE( externallyRetainedWeak.expired() );

	first.reset();
	second.reset();
	shared.reset();
	later.reset();
	observedOnly.reset();
	externallyRetained.reset();
	secondScope.reset();
	sharedCatalog.reset();
	laterCatalog.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, defaultResourceScopeImportsGlobalCatalog ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Default resource scope test" );
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 1, 1 );
	ASSERT_TRUE( texture != nullptr );
	globalResourceCatalog().publish( "global-texture", texture );

	EXPECT_EQ( texture.get(), defaultResourceScope().findTexture( "global-texture" ).get() );
	ResourceScopePtr isolatedScope = ResourceScope::New();
	EXPECT_TRUE( isolatedScope->findTexture( "global-texture" ) == nullptr );

	texture.reset();
	isolatedScope.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, uiScenesOwnIsolatedScopesThatCanBeSharedExplicitly ) {
	EE::Window::Window* window = createLifecycleTestWindow( "UI scene resource scope test" );
	UISceneNode* firstScene = UISceneNode::New( window );
	UISceneNode* secondScene = UISceneNode::New( window );
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 1, 1 );
	ASSERT_TRUE( texture != nullptr );

	firstScene->getResourceScope()->publishLocal( "scene-texture", texture );
	EXPECT_TRUE( secondScene->getResourceScope()->findTexture( "scene-texture" ) == nullptr );
	secondScene->setResourceScope( firstScene->getResourceScope() );
	EXPECT_EQ( texture.get(),
			   secondScene->getResourceScope()->findTexture( "scene-texture" ).get() );

	texture.reset();
	eeDelete( secondScene );
	eeDelete( firstScene );
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, pendingBatchRetainsTextureUntilDisplayCollection ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Texture deferred release test" );
	TextureFactory* factory = TextureFactory::instance();
	Uint8 pixels[2 * 2 * 4]{};
	TextureLoader loader( pixels, 2, 2, 4 );
	loader.load();
	TexturePtr texture = loader.getTexture();
	ASSERT_TRUE( texture != nullptr );

	TextureRegistrySnapshot snapshot = factory->snapshotTextures();
	auto record = std::find_if( snapshot.begin(), snapshot.end(), [&texture]( const auto& entry ) {
		return entry.id == texture->getTextureId();
	} );
	ASSERT_TRUE( record != snapshot.end() );
	TextureWeakPtr weakTexture = record->texture;
	TexturePtr retainedTexture = weakTexture.lock();
	ASSERT_TRUE( retainedTexture != nullptr );

	loader.reset();
	GlobalBatchRenderer::instance()->setTexture( texture );
	GlobalBatchRenderer::instance()->batchQuad( 0, 0, 2, 2 );
	texture.reset();
	retainedTexture.reset();

	EXPECT_FALSE( weakTexture.expired() );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 0 ) );

	window->display( false );

	EXPECT_TRUE( weakTexture.expired() );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 0 ) );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, workerFinalReleaseDefersDestructionUntilDisplay ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Worker texture release test" );
	TextureFactory* factory = TextureFactory::instance();
	TexturePtr texture = factory->createEmptyTexture( 1, 1 );
	ASSERT_TRUE( texture != nullptr );
	TextureWeakPtr weakTexture = texture;

	std::thread worker( [texture = std::move( texture )]() mutable { texture.reset(); } );
	worker.join();

	EXPECT_TRUE( weakTexture.expired() );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 1 ) );
	window->display( false );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 0 ) );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, workerFileTextureLoadReleasesDecoderPixelsCorrectly ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Worker file texture load test" );
	TexturePtr texture;
	std::thread worker( [&texture] {
		texture = TextureFactory::instance()->loadFromFile( Sys::getProcessPath() +
															"../assets/atlases/bnb/007.png" );
	} );
	worker.join();
	ASSERT_TRUE( texture != nullptr );

	texture.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, textureRegionRetainsItsTexture ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Texture region ownership test" );
	TextureFactory* factory = TextureFactory::instance();
	Uint8 pixels[2 * 2 * 4]{};
	TextureLoader loader( pixels, 2, 2, 4 );
	loader.load();
	TexturePtr texture = loader.getTexture();
	ASSERT_TRUE( texture != nullptr );
	TextureWeakPtr weakTexture = texture;

	{
		TextureRegion region( texture );
		loader.reset();
		texture.reset();
		EXPECT_FALSE( weakTexture.expired() );
	}

	EXPECT_TRUE( weakTexture.expired() );
	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 1 ) );

	window->display( false );

	EXPECT_EQ( factory->getPendingReleaseCount(), static_cast<std::size_t>( 0 ) );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, drawableResourceConnectionsDisconnectWithTheirLifetime ) {
	auto resource = makeResource<TestDrawableResource>();
	int notifications = 0;
	{
		DrawableResourceConnection connection = resource->connectResourceChange(
			[&notifications]( DrawableResource& ) { ++notifications; } );
		EXPECT_TRUE( static_cast<bool>( connection ) );
		resource->notifyChange();
		EXPECT_EQ( notifications, 1 );
	}

	resource->notifyChange();
	EXPECT_EQ( notifications, 1 );

	DrawableResourceConnection expiredConnection =
		resource->connectResourceChange( []( DrawableResource& ) {} );
	resource.reset();
	EXPECT_FALSE( static_cast<bool>( expiredConnection ) );
	expiredConnection.disconnect();
}

UTEST( ResourcePrerequisites, textureCreatesIndependentDrawableInstances ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Texture drawable instance test" );
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 2, 2 );
	ASSERT_TRUE( texture != nullptr );

	DrawablePtr firstDrawable = texture->createInstance();
	DrawablePtr secondDrawable = texture->createInstance();
	ASSERT_TRUE( firstDrawable != nullptr );
	ASSERT_TRUE( secondDrawable != nullptr );
	ASSERT_EQ( firstDrawable->getDrawableType(), Drawable::TEXTUREDRAWABLE );
	ASSERT_EQ( secondDrawable->getDrawableType(), Drawable::TEXTUREDRAWABLE );
	auto first = std::static_pointer_cast<TextureDrawable>( firstDrawable );
	auto second = std::static_pointer_cast<TextureDrawable>( secondDrawable );
	ASSERT_TRUE( first != nullptr );
	ASSERT_TRUE( second != nullptr );
	EXPECT_NE( first.get(), second.get() );
	EXPECT_EQ( first->getTexture().get(), texture.get() );
	EXPECT_EQ( second->getTexture().get(), texture.get() );

	first->setColor( Color::Red );
	second->setColor( Color::Blue );
	first->setPosition( { 3.f, 4.f } );
	EXPECT_TRUE( first->getColor() == Color::Red );
	EXPECT_TRUE( second->getColor() == Color::Blue );
	EXPECT_TRUE( second->getPosition() == Vector2f::Zero );

	firstDrawable.reset();
	secondDrawable.reset();
	first.reset();
	second.reset();
	texture.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, uiIconSeparatesSourceLookupFromInstanceCreation ) {
	auto instanceCount = std::make_shared<int>( 0 );
	DrawablePtr source = makeResource<CountingDrawable>( instanceCount );
	UIIcon* icon = UIIcon::New( "counting-icon" );
	icon->setSource( 16, source );

	const DrawablePtr& exactSource = icon->getSource( 16 );
	const DrawablePtr& closestSource = icon->getSource( 14 );
	EXPECT_EQ( exactSource.get(), source.get() );
	EXPECT_EQ( closestSource.get(), source.get() );
	EXPECT_EQ( *instanceCount, 0 );

	DrawablePtr first = icon->createDrawable( 16 );
	DrawablePtr second = icon->createDrawable( 14 );
	ASSERT_TRUE( first != nullptr );
	ASSERT_TRUE( second != nullptr );
	EXPECT_EQ( *instanceCount, 2 );
	EXPECT_NE( first.get(), second.get() );
	EXPECT_NE( first.get(), source.get() );

	first->setColor( Color::Red );
	EXPECT_TRUE( source->getColor() == Color::White );
	EXPECT_TRUE( second->getColor() == Color::White );

	eeDelete( icon );
}

UTEST( ResourcePrerequisites, stateListsCloneStateAndChildrenIndependently ) {
	EE::Window::Window* window = createLifecycleTestWindow( "State list instance test" );
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 2, 2 );
	ASSERT_TRUE( texture != nullptr );
	auto firstRegion = makeResource<TextureRegion>( texture );
	auto secondRegion = makeResource<TextureRegion>( texture );
	auto source = ResourcePtr<StateListDrawable>( StateListDrawable::New(),
												  ResourceDeleter<StateListDrawable>() );
	source->setStateDrawable( 1, firstRegion );
	source->setStateDrawable( 2, secondRegion );
	source->setState( 1 );

	DrawablePtr drawableInstance = source->createInstance();
	ASSERT_TRUE( drawableInstance != nullptr );
	ASSERT_EQ( drawableInstance->getDrawableType(), Drawable::STATELIST );
	auto instance = std::static_pointer_cast<StateListDrawable>( drawableInstance );
	ASSERT_TRUE( instance != nullptr );
	EXPECT_EQ( source->getState(), 1u );
	EXPECT_EQ( instance->getState(), 1u );
	EXPECT_NE( source->getStateDrawable( 1 ), instance->getStateDrawable( 1 ) );

	instance->setState( 2 );
	instance->setStateColor( 1, Color::Red );
	EXPECT_EQ( source->getState(), 1u );
	EXPECT_EQ( instance->getState(), 2u );
	EXPECT_TRUE( source->getStateDrawable( 1 )->getColor() == Color::White );
	EXPECT_TRUE( instance->getStateDrawable( 1 )->getColor() == Color::Red );
	EXPECT_FALSE( source->hasDrawableState( 99 ) );
	source->setState( 99 );
	EXPECT_FALSE( source->hasDrawableState( 99 ) );

	instance.reset();
	drawableInstance.reset();
	source.reset();
	firstRegion.reset();
	secondRegion.reset();
	texture.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, spritesCloneFramesAndAnimationStateIndependently ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Sprite instance test" );
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 2, 2 );
	ASSERT_TRUE( texture != nullptr );
	SpritePtr source = Sprite::New();
	source->createAnimation();
	source->addFrame( texture, Sizef( 2.f, 2.f ) );
	source->addFrame( texture, Sizef( 2.f, 2.f ) );
	source->setCurrentFrame( 0 );

	SpritePtr instance = source->clone();
	ASSERT_TRUE( instance != nullptr );
	TextureRegion* sourceFrame = source->getTextureRegion( 0 );
	TextureRegion* instanceFrame = instance->getTextureRegion( 0 );
	ASSERT_TRUE( sourceFrame != nullptr );
	ASSERT_TRUE( instanceFrame != nullptr );
	EXPECT_NE( sourceFrame, instanceFrame );
	EXPECT_EQ( sourceFrame->getTexture().get(), instanceFrame->getTexture().get() );

	instance->setCurrentFrame( 2 );
	instanceFrame->setDestSize( Sizef( 8.f, 9.f ) );
	EXPECT_EQ( source->getCurrentFrame(), 0u );
	EXPECT_EQ( instance->getCurrentFrame(), 1u );
	EXPECT_TRUE( sourceFrame->getDestSize() == Sizef( 2.f, 2.f ) );
	EXPECT_TRUE( instanceFrame->getDestSize() == Sizef( 8.f, 9.f ) );

	instance.reset();
	source.reset();
	texture.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, regionConsumersDoNotMutateSharedSourceGeometryWhileDrawing ) {
	EE::Window::Window* window = createLifecycleTestWindow( "Texture region draw isolation test" );
	TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 8, 8 );
	ASSERT_TRUE( texture != nullptr );
	auto source = makeResource<TextureRegion>( texture, Rect( 1, 2, 7, 8 ), Sizef( 6.f, 6.f ),
											   Vector2i( 2, 3 ) );
	const Rect originalRect = source->getSrcRect();
	const Sizef originalSize = source->getDestSize();
	const Vector2i originalOffset = source->getOffset();

	UITextureRegion* widget = UITextureRegion::New();
	widget->setTextureRegion( source.get() );
	widget->setSize( Sizef( 20.f, 20.f ) );
	widget->setScaleType( UIScaleType::Expand );
	widget->draw();
	EXPECT_TRUE( source->getSrcRect() == originalRect );
	EXPECT_TRUE( source->getDestSize() == originalSize );
	EXPECT_TRUE( source->getOffset() == originalOffset );
	eeDelete( widget );

	{
		ScrollParallax parallax( source.get(), Vector2f::Zero, Sizef( 20.f, 20.f ) );
		ASSERT_TRUE( parallax.getTextureRegion() != nullptr );
		EXPECT_NE( parallax.getTextureRegion(), source.get() );
		EXPECT_EQ( parallax.getTextureRegion()->getTexture().get(), texture.get() );
		parallax.draw();
		EXPECT_TRUE( source->getSrcRect() == originalRect );
		EXPECT_TRUE( source->getDestSize() == originalSize );
		EXPECT_TRUE( source->getOffset() == originalOffset );
	}

	source.reset();
	texture.reset();
	window->display( false );
	Engine::destroySingleton();
}

UTEST( ResourcePrerequisites, drawableGroupsAndVariantsHoldSafeIndependentHandles ) {
	auto source = DrawableGroup::New();
	auto rectangle = makeResource<RectangleDrawable>( Vector2f( 1.f, 2.f ), Sizef( 3.f, 4.f ) );
	source->addDrawable( rectangle );

	DrawablePtr drawableInstance = source->createInstance();
	ASSERT_TRUE( drawableInstance != nullptr );
	ASSERT_EQ( drawableInstance->getDrawableType(), Drawable::GROUP );
	auto instance = std::static_pointer_cast<DrawableGroup>( drawableInstance );
	ASSERT_TRUE( instance != nullptr );
	ASSERT_EQ( instance->getDrawableCount(), 1u );
	EXPECT_NE( source->getGroup()[0].get(), instance->getGroup()[0].get() );
	instance->getGroup()[0]->setColor( Color::Blue );
	EXPECT_TRUE( source->getGroup()[0]->getColor() == Color::White );
	EXPECT_TRUE( instance->getGroup()[0]->getColor() == Color::Blue );

	Variant original( instance );
	Variant copied( original );
	instance.reset();
	EXPECT_TRUE( original.asDrawable() != nullptr );
	EXPECT_EQ( original.asDrawable().get(), copied.asDrawable().get() );
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

		TexturePtr texture = TextureFactory::instance()->createEmptyTexture( 4, 4 );
		ASSERT_TRUE( texture != nullptr );

		NinePatch* ninePatch = NinePatchManager::instance()->add(
			NinePatch::New( texture, 1, 1, 1, 1, 1, "engine-teardown-nine-patch" ) );
		ASSERT_TRUE( ninePatch != nullptr );

		auto* scene = UISceneNode::New();
		SceneManager::instance()->add( scene );
		scene->enableFrameBuffer();
		UIImage::New()->setDrawable( ninePatch->createInstance() )->setParent( scene->getRoot() );

		auto* font = FontTrueType::New( "engine-teardown-font" );
		ASSERT_TRUE(
			font->loadFromFile( Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf" ) );
		auto layout = TextLayout::layout( String( "cached before Engine teardown" ), font, 14, 0 );
		std::weak_ptr<const TextLayout> layoutWeak = layout;
		layout.reset();

		auto* batch = GlobalBatchRenderer::instance();
		batch->setTexture( texture );
		batch->batchQuad( 0, 0, 4, 4 );
		texture.reset();

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
