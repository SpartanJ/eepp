#include "utest.hpp"

#include <atomic>
#include <condition_variable>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/resourceloader.hpp>
#include <eepp/ui/models/variant.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>

using namespace EE;
using namespace EE::Graphics;
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

} // namespace

static_assert( !std::is_copy_constructible<Texture>::value, "Texture must not be copyable" );
static_assert( !std::is_copy_assignable<Texture>::value, "Texture must not be copy-assignable" );

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
