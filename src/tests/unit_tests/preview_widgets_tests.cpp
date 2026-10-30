#include "exif_test_utils.hpp"
#include "utest.hpp"
#include <atomic>
#include <cstring>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/sys.hpp>
#include <thread>

#include <eepp/audio/inputsoundfile.hpp>
#include <eepp/audio/music.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/tools/uiaudioplayer.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextview.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::Window;

namespace {

std::vector<Uint8> wav( Uint32 seconds ) {
	const Uint32 samples = 8000 * seconds;
	std::vector<Uint8> data( 44 + 2 * samples );
	const auto put = [&data]( size_t offset, Uint32 value, size_t bytes ) {
		for ( size_t i = 0; i < bytes; ++i )
			data[offset + i] = static_cast<Uint8>( value >> ( i * 8 ) );
	};
	std::memcpy( data.data(), "RIFF", 4 );
	put( 4, data.size() - 8, 4 );
	std::memcpy( data.data() + 8, "WAVEfmt ", 8 );
	put( 16, 16, 4 );
	put( 20, 1, 2 );
	put( 22, 1, 2 );
	put( 24, 8000, 4 );
	put( 28, 16000, 4 );
	put( 32, 2, 2 );
	put( 34, 16, 2 );
	std::memcpy( data.data() + 36, "data", 4 );
	put( 40, 2 * samples, 4 );
	return data;
}

std::unique_ptr<IOStream> wavStream( Uint32 seconds ) {
	const auto bytes = wav( seconds );
	auto stream = std::make_unique<IOStreamString>();
	stream->write( reinterpret_cast<const char*>( bytes.data() ), bytes.size() );
	stream->IOStreamString::seek( 0 );
	return stream;
}

struct StreamProbe {
	std::thread::id uiThread{ std::this_thread::get_id() };
	std::atomic<bool> track{ false }, uiIO{ false }, uiCleanup{ false }, destroyed{ false };
	std::atomic<int> workerSeeks{ 0 };
};

class ProbedStream : public IOStreamString {
  public:
	explicit ProbedStream( std::shared_ptr<StreamProbe> probe ) : probe( std::move( probe ) ) {}

	~ProbedStream() {
		probe->uiCleanup = std::this_thread::get_id() == probe->uiThread;
		probe->destroyed = true;
	}

	ios_size seek( ios_size position ) {
		if ( probe->track ) {
			if ( std::this_thread::get_id() == probe->uiThread )
				probe->uiIO = true;
			else
				++probe->workerSeeks;
		}
		return IOStreamString::seek( position );
	}

	std::shared_ptr<StreamProbe> probe;
};

bool finishImageWork( UIImageViewer* viewer ) {
	Clock clock;
	while ( viewer->isLoading() && clock.getElapsedTime() < Seconds( 5 ) )
		Sys::sleep( Milliseconds( 1 ) );
	return !viewer->isLoading();
}

} // namespace

UTEST( PreviewWidgets, ResetAndReplacementDiscardQueuedImageCompletions ) {
	UIApplication::Settings settings;
	settings.threadPool = ThreadPool::createShared( 1 );
	UIApplication app( WindowSettings{ 400, 300, "Preview widgets" }, settings );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( app.getUI() );
	viewer->setPixelsSize( 200, 200 );
	size_t completed = 0;
	size_t failed = 0;
	viewer->on( Event::OnResourceLoaded, [&completed]( const Event* ) { ++completed; } );
	viewer->on( Event::OnResourceLoadFailed, [&failed]( const Event* ) { ++failed; } );
	const std::string small = "<svg xmlns='http://www.w3.org/2000/svg' width='20' height='10'>"
							  "<rect width='20' height='10' fill='red'/></svg>";
	const std::string large = "<svg xmlns='http://www.w3.org/2000/svg' width='80' height='60'>"
							  "<rect width='80' height='60' fill='blue'/></svg>";
	viewer->loadImageAsync( small, true, false );
	ASSERT_TRUE( finishImageWork( viewer ) );
	viewer->reset();
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_FALSE( viewer->hasImage() );
	EXPECT_EQ( completed, size_t{ 0 } );
	EXPECT_FALSE( viewer->getLoader()->isVisible() );

	viewer->loadImageAsync( small, true, false );
	ASSERT_TRUE( finishImageWork( viewer ) );
	viewer->loadImageAsync( std::string( large ), true, false );
	ASSERT_TRUE( finishImageWork( viewer ) );
	app.getUI()->update( Milliseconds( 16 ) );
	ASSERT_TRUE( viewer->hasImage() );
	EXPECT_EQ( completed, size_t{ 1 } );
	const auto size = viewer->getImage()->getPixelsSize();
	EXPECT_NEAR( size.x / size.y, 80.f / 60.f, 0.01f );

	viewer->loadImageAsync( "invalid image", true, false );
	ASSERT_TRUE( finishImageWork( viewer ) );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_FALSE( viewer->hasImage() );
	EXPECT_FALSE( viewer->getLoader()->isVisible() );
	EXPECT_EQ( completed, size_t{ 1 } );
	EXPECT_EQ( failed, size_t{ 1 } );
}

UTEST( PreviewWidgets, AudioOwnsStreamStartsStoppedAndReleasesOnReset ) {
	UIApplication app( WindowSettings{ 400, 300, "Preview widgets" } );
	auto* player = UIAudioPlayer::New();
	player->setParent( app.getUI() );
	ASSERT_TRUE( player->loadFromStream( wavStream( 2 ), false ) );
	EXPECT_EQ( player->getAudio()->getStatus(), EE::Audio::SoundSource::Stopped );
	EXPECT_EQ( player->getAudio()->getDuration().asMilliseconds(), 2000 );
	player->scheduledUpdate( Time::Zero );
	EXPECT_EQ( player->getProgressBar()->getProgress(), 0.f );
	EXPECT_TRUE( player->getTimeView()->getText() == "0:00 / 0:02" );
	player->getAudio()->setVolume( 30.f );
	ASSERT_TRUE( player->loadFromStream( wavStream( 1 ), false ) );
	EXPECT_EQ( player->getAudio()->getDuration().asMilliseconds(), 1000 );
	player->reset();
	player->scheduledUpdate( Time::Zero );
	EXPECT_TRUE( player->getAudio()->getDuration() == Time::Zero );
	EXPECT_NEAR( player->getAudio()->getVolume(), 30.f, 0.01f );
	EXPECT_EQ( player->getProgressBar()->getProgress(), 0.f );
	EXPECT_TRUE( player->getTimeView()->getText() == "0:00 / 0:00" );
}

UTEST( PreviewWidgets, OwnedWebPStreamLoadsThroughTheImageViewer ) {
	UIApplication::Settings settings;
	settings.threadPool = ThreadPool::createShared( 1 );
	UIApplication app( WindowSettings{ 400, 300, "Preview streams" }, settings );
	std::string path = Sys::getTempPath();
	FileSystem::dirAddSlashAtEnd( path );
	path += "eepp-stream-image-" + String::toString( Sys::getProcessID() ) + ".webp";
	Image image( 16, 12, 4, Color::Red );
	ASSERT_TRUE( image.saveToFile( path, Image::SaveType::WEBP ) );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( app.getUI() );
	viewer->setPixelsSize( 200, 200 );
	viewer->loadImageAsync( std::make_unique<IOStreamFile>( path ) );
	ASSERT_TRUE( finishImageWork( viewer ) );
	app.getUI()->update( Milliseconds( 16 ) );
	ASSERT_TRUE( viewer->hasImage() );
	const auto size = viewer->getImage()->getPixelsSize();
	EXPECT_NEAR( size.x / size.y, 4.f / 3.f, 0.01f );
	FileSystem::fileRemove( path );
}

UTEST( PreviewWidgets, AudioSeeksAndCleanupStayOnTheWorkerAfterOwnershipTransfer ) {
	auto probe = std::make_shared<StreamProbe>();
	UIApplication::Settings settings;
	settings.threadPool = ThreadPool::createShared( 2 );
	UIApplication app( WindowSettings{ 600, 300, "Preview streams" }, settings );
	auto* player = UIAudioPlayer::New();
	player->setParent( app.getUI() );
	player->setPixelsSize( 500, 80 );
	auto stream = std::make_unique<ProbedStream>( probe );
	const auto data = wav( 2 );
	stream->write( reinterpret_cast<const char*>( data.data() ), data.size() );
	stream->seek( 0 );
	ASSERT_TRUE( player->loadFromStream( std::move( stream ), false ) );
	probe->track = true;
	player->play();
	Clock clock;
	while ( player->getAudio()->getStatus() != EE::Audio::SoundSource::Playing &&
			clock.getElapsedTime() < Seconds( 5 ) )
		Sys::sleep( Milliseconds( 1 ) );
	EXPECT_TRUE( player->getAudio()->getStatus() == EE::Audio::SoundSource::Playing );
	player->getProgressBar()->setPixelsSize( 200, 10 );
	const auto position = player->getProgressBar()->convertToWorldSpace( { 100, 5 } );
	app.getUI()->getEventDispatcher()->sendMouseClick( player->getProgressBar(), position.asInt(),
													   EE_BUTTON_LMASK );
	player->reset();
	clock.restart();
	while ( !probe->destroyed && clock.getElapsedTime() < Seconds( 5 ) )
		Sys::sleep( Milliseconds( 1 ) );
	EXPECT_TRUE( probe->destroyed );
	EXPECT_GT( probe->workerSeeks.load(), 0 );
	EXPECT_FALSE( probe->uiIO );
	EXPECT_FALSE( probe->uiCleanup );
}

UTEST( PreviewWidgets, FailedAudioRewindReleasesTheReaderAndOwnedStream ) {
	class FailedRewind : public IOStreamString {
	  public:
		explicit FailedRewind( bool& destroyed ) : destroyed( destroyed ) {}

		~FailedRewind() { destroyed = true; }

		ios_size seek( ios_size position ) {
			return position == 0 && ++rewinds == 2 ? -1 : IOStreamString::seek( position );
		}

		bool& destroyed;
		int rewinds = 0;
	};
	bool destroyed = false;
	auto stream = std::make_unique<FailedRewind>( destroyed );
	const auto data = wav( 1 );
	stream->write( reinterpret_cast<const char*>( data.data() ), data.size() );
	EE::Audio::InputSoundFile file;
	EXPECT_FALSE( file.openFromStream( std::move( stream ) ) );
	EXPECT_TRUE( destroyed );
	file.seek( Time::Zero ); // No half-open reader may retain the released stream.
	EE::Int16 sample;
	EXPECT_EQ( file.read( &sample, 1 ), EE::Uint64{ 0 } );
}

UTEST( PreviewWidgets, GalleryMetadataIsCommittedWithTheCurrentImageAndClearedForStreams ) {
	UIApplication::Settings settings;
	settings.threadPool = ThreadPool::createShared( 2 );
	UIApplication app( WindowSettings{ 400, 300, "Preview gallery" }, settings );
	std::string path = Sys::getTempPath();
	FileSystem::dirAddSlashAtEnd( path );
	path += "eepp-gallery-" + String::toString( Sys::getProcessID() ) + ".png";
	Image image( 16, 12, 4, Color::Red );
	ASSERT_TRUE( image.saveToFile( path, Image::SaveType::PNG ) );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( app.getUI() );
	viewer->setPixelsSize( 200, 200 );
	viewer->loadImageAsync( std::string_view( path ), false, false );
	ASSERT_TRUE( finishImageWork( viewer ) );
	EXPECT_TRUE( viewer->getImagePath().empty() );
	viewer->reset();
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_TRUE( viewer->getImagePath().empty() );

	viewer->loadImageAsync( std::string_view( path ), false, false );
	ASSERT_TRUE( finishImageWork( viewer ) );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_TRUE( viewer->getImagePath() == path );
	auto stream = std::make_unique<IOStreamString>( path );
	viewer->loadImageAsync( std::move( stream ) );
	ASSERT_TRUE( finishImageWork( viewer ) );
	EXPECT_TRUE( viewer->getImagePath() == path );
	app.getUI()->update( Milliseconds( 16 ) );
	EXPECT_TRUE( viewer->hasImage() );
	EXPECT_TRUE( viewer->getImageName().empty() );
	EXPECT_TRUE( viewer->getGalleryPath().empty() );
	FileSystem::fileRemove( path );
}

UTEST( PreviewWidgets, EXIFOrientationUsesDrawTransformsAndKeepsTexturePixelsUnchanged ) {
	UIApplication::Settings settings;
	settings.threadPool = ThreadPool::createShared( 1 );
	UIApplication app( WindowSettings{ 400, 300, "EXIF rendering" }, settings );
	std::string path = Sys::getTempPath();
	FileSystem::dirAddSlashAtEnd( path );
	path += "eepp-exif-viewer-" + String::toString( Sys::getProcessID() ) + ".jpg";
	Image source( 64, 32, 3, Color::Red );
	ASSERT_TRUE( source.saveToFile( path, Image::SaveType::JPG ) );
	std::string encoded;
	ASSERT_TRUE( FileSystem::fileGet( path, encoded ) );
	FileSystem::fileRemove( path );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( app.getUI() );
	viewer->setPixelsSize( 100, 40 );
	const int rotations[] = { 0, 0, 180, 0, 90, 90, 90, 270 };
	const RenderMode modes[] = { RENDER_NORMAL,	 RENDER_MIRROR, RENDER_NORMAL, RENDER_FLIPPED,
								 RENDER_FLIPPED, RENDER_NORMAL, RENDER_MIRROR, RENDER_NORMAL };
	for ( int orientation = 1; orientation <= 8; ++orientation ) {
		viewer->loadImageAsync( exiftest::jpeg( encoded, exiftest::tiff( orientation ) ), true,
								false );
		ASSERT_TRUE( finishImageWork( viewer ) );
		app.getUI()->update( Milliseconds( 16 ) );
		ASSERT_TRUE( viewer->hasImage() );
		auto* sprite = static_cast<Sprite*>( viewer->getImage()->getDrawable().get() );
		EXPECT_EQ( sprite->getCurrentTextureRegion()->getTexture()->getPixelsSize().x, 64.f );
		EXPECT_EQ( sprite->getCurrentTextureRegion()->getTexture()->getPixelsSize().y, 32.f );
		EXPECT_EQ( sprite->getRenderMode(), modes[orientation - 1] );
		EXPECT_EQ( viewer->getImage()->getRotation(), float( rotations[orientation - 1] ) );
		EXPECT_EQ( viewer->getOrientedImageSize().x,
				   orientation >= 5 ? sprite->getPixelsSize().y : sprite->getPixelsSize().x );
		viewer->resetImageView();
		EXPECT_EQ( viewer->getImage()->getRotation(), float( rotations[orientation - 1] ) );
		const auto bounds = viewer->getImage()->getWorldBounds();
		const auto parent = viewer->getWorldBounds();
		EXPECT_GE( bounds.Left, parent.Left - 1.f );
		EXPECT_LE( bounds.Right, parent.Right + 1.f );
		EXPECT_GE( bounds.Top, parent.Top - 1.f );
		EXPECT_LE( bounds.Bottom, parent.Bottom + 1.f );
	}
}

UTEST( PreviewWidgets, ImageViewerKeysAreRebindableCommands ) {
	UIApplication app( WindowSettings{ 400, 300, "Image viewer commands" } );
	auto* viewer = UIImageViewer::New();
	viewer->setParent( app.getUI() );
	viewer->setFocus();
	auto* dispatcher = app.getUI()->getEventDispatcher();
	auto press = [&]( Keycode key ) {
		dispatcher->sendKeyDown( key, SCANCODE_UNKNOWN, 0, KEYMOD_NONE );
		dispatcher->sendKeyUp( key, SCANCODE_UNKNOWN, 0, KEYMOD_NONE );
	};
	press( KEY_R );
	EXPECT_NEAR( 90.f, viewer->getImage()->getRotation(), 0.001f );

	// Applications replace the defaults with configured shortcuts.
	viewer->getKeyBindings().reset();
	viewer->getKeyBindings().addKeybind( { KEY_O, KEYMOD_NONE }, "image-rotate" );
	press( KEY_R );
	EXPECT_NEAR( 90.f, viewer->getImage()->getRotation(), 0.001f );
	press( KEY_O );
	EXPECT_NEAR( 180.f, viewer->getImage()->getRotation(), 0.001f );
}
