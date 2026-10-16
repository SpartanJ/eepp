#include "testexitcode.hpp"
#include <array>
#include <chrono>
#include <cstdlib>
#include <eepp/graphics/image.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <thread>
#include <unistd.h>

using namespace EE;
using namespace EE::Window;
using namespace EE::System;

namespace {
using TestClock = std::chrono::steady_clock;
const RGB sourceColor( 180, 30, 50 ), targetColor( 30, 80, 190 ), externalColor( 135, 45, 185 );
enum Scenario : size_t {
	Snapshot,
	Large,
	Rejected,
	Cancel,
	Escape,
	Destroy,
	External,
	Instant,
	StalledCancel,
	Shutdown
};
constexpr const char* scenarios[]{ "snapshot",		 "large",	"rejected", "cancel",
								   "escape",		 "destroy", "external", "instant",
								   "stalled-cancel", "shutdown" };

/** Weston controls placement. The isolated runner finds these solid colors on its X11 output;
 * no window positioning, pointer warping, or synthetic events occur in the normal manual demo. */
struct WaylandChecks {
	EE::Window::Window* source{};
	EE::Window::Window* target{};
	std::filesystem::path fixtureDirectory;
	DragOffer offer;
	SmallVector<std::string, 2> received;
	std::array<Uint8, 64 * 40 * 4> pixels;
	Graphics::Image image{ pixels.data(), 64, 40, 4 };
	Graphics::Image rgbImage{ pixels.data(), 64, 40, 3 };
	Vector2i press;
	TestClock::time_point deadline;
	size_t scenario{}, completions{};
	size_t clickPresses{}, clickReleases{};
	bool armed{}, done{}, reported{}, passed{ true }, cancelPending{}, destroyPending{},
		clickPending{}, exitingWithBackend{};

	WaylandChecks() {
		image.avoidFreeImage( true );
		rgbImage.avoidFreeImage( true );
		image.fillWithColor( Color( 40, 220, 100 ) );
	}

	~WaylandChecks() {
		Engine::destroySingleton();
		removeFixtures();
	}

	void removeFixtures() {
		if ( !fixtureDirectory.empty() ) {
			std::error_code error;
			for ( const char* name : { "sample #\xc3\xa9-one.txt", "sample two.txt" } )
				std::filesystem::remove( fixtureDirectory / name, error );
			std::filesystem::remove( fixtureDirectory, error );
			fixtureDirectory.clear();
		}
	}

	bool initialize() {
		auto candidate = std::filesystem::temp_directory_path() /
						 ( "eepp-wayland-dnd-" + std::to_string( Sys::getProcessID() ) + "-" +
						   std::to_string( TestClock::now().time_since_epoch().count() ) );
		if ( !std::filesystem::create_directory( candidate ) )
			return false;
		fixtureDirectory = std::move( candidate );
		for ( const char* name : { "sample #\xc3\xa9-one.txt", "sample two.txt" } ) {
			std::ofstream file( fixtureDirectory / name );
			file << "eepp native drag fixture\n";
			if ( !file )
				return false;
		}
		target = Engine::instance()->createWindow(
			{ 320, 220, "eepp Wayland test target", WindowStyle::Borderless } );
		if ( !target || !target->isOpen() )
			return false;
		// The post-drag click must reach the application even when it also activates the source.
		Engine::instance()->getDisplayManager()->enableMouseFocusClickThrough();
		target->setClearColor( targetColor );
		target->getInput()->pushCallback( [this]( InputEvent* event ) {
			if ( event->Type == InputEvent::FileDropped )
				received.emplace_back( event->file.file );
		} );
		return createSource();
	}

	bool createSource() {
		source = Engine::instance()->createWindow(
			{ 360, 240, "eepp Wayland test source", WindowStyle::Borderless } );
		if ( !source || !source->isOpen() )
			return false;
		source->setClearColor( sourceColor );
		source->getInput()->pushCallback( [this]( InputEvent* event ) {
			if ( reported && clickPending ) {
				if ( event->Type == InputEvent::MouseButtonDown &&
					 event->button.button == EE_BUTTON_LEFT )
					++clickPresses;
				if ( event->Type == InputEvent::MouseButtonUp &&
					 event->button.button == EE_BUTTON_LEFT )
					++clickReleases;
				return;
			}
			if ( done || reported )
				return;
			if ( event->Type == InputEvent::MouseButtonUp &&
				 event->button.button == EE_BUTTON_LEFT &&
				 DragAndDrop::forWindow( *source ).isDragging() ) {
				passed = false;
				std::cerr << "Native drag delivered a synthetic button-up to the application\n";
			}
			if ( event->Type == InputEvent::MouseButtonDown &&
				 event->button.button == EE_BUTTON_LEFT ) {
				press = { event->button.x, event->button.y };
				armed = true;
			} else if ( event->Type == InputEvent::MouseButtonUp ) {
				armed = false;
			} else if ( event->Type == InputEvent::MouseMotion && armed ) {
				const Vector2i delta = Vector2i( event->motion.x, event->motion.y ) - press;
				if ( delta.x * delta.x + delta.y * delta.y >= 64 ) {
					armed = false;
					start();
				}
			}
		} );
		return true;
	}

	void prepare() {
		if ( !source && !createSource() ) {
			passed = false;
			return;
		}
		offer.files.clear();
		const size_t count = scenario == Large ? 4096 : 2;
		for ( size_t i = 0; i < count; ++i ) {
			offer.files.push_back(
				{ ( fixtureDirectory / ( i % 2 ? "sample two.txt" : "sample #\xc3\xa9-one.txt" ) )
					  .string() } );
		}
		if ( scenario == External )
			rgbImage.fillWithColor( Color( 40, 220, 100 ) );
		offer.preview.image = scenario == Snapshot	 ? &image
							  : scenario == External ? &rgbImage
													 : nullptr;
		offer.preview.hotspot = { 8, 8 }; // The icon must never intercept target hit-testing.
		received.clear();
		completions = 0;
		armed = done = reported = false;
		deadline = TestClock::now() + std::chrono::seconds( 20 );
		if ( scenario == Snapshot ) {
			const auto* snapshot = offer.preview.image;
			std::array<Uint8, 2> monochrome{};
			for ( unsigned channels : { 1u, 2u } ) {
				Graphics::Image invalidImage( monochrome.data(), 1, 1, channels );
				invalidImage.avoidFreeImage( true );
				offer.preview.image = &invalidImage;
				passed &= DragAndDrop::forWindow( *source ).startDrag( offer ).status ==
						  DragStartStatus::InvalidOffer;
			}
			offer.preview.image = snapshot;
		}
		const auto invalid = DragAndDrop::forWindow( *source ).startDrag( offer );
		passed &= invalid.status == DragStartStatus::InvalidGesture;
		std::cout << "WAYLAND_DND_READY " << scenarios[scenario] << std::endl;
	}

	bool waitForRunner() {
		pollfd command{ STDIN_FILENO, POLLIN, 0 };
		char gate[16];
		return poll( &command, 1, 5000 ) > 0 && read( STDIN_FILENO, gate, sizeof( gate ) ) > 0;
	}

	void start() {
		if ( scenario == StalledCancel ) {
			std::cout << "WAYLAND_DND_BEFORE_START" << std::endl;
			passed &= waitForRunner();
		}
		auto drag = DragAndDrop::forWindow( *source );
		const auto result = drag.startDrag( offer, [this]( DragResult result ) {
			if ( exitingWithBackend )
				std::_Exit( 1 ); // Process teardown must never deliver the completion callback.
			++completions;
			const bool copied = scenario == Snapshot || scenario == Large || scenario == External;
			passed &=
				result.reason == ( copied ? DragEndReason::Dropped : DragEndReason::Cancelled );
			passed &= result.performedAction == ( copied ? DragAction::Copy : DragAction::None );
			if ( source ) {
				passed &= !source->getInput()->isMouseLeftPressed();
				passed &= !( source->getInput()->getClickTrigger() & EE_BUTTON_LMASK );
				passed &= !( source->getInput()->getDoubleClickTrigger() & EE_BUTTON_LMASK );
				passed &= !DragAndDrop::forWindow( *source ).isDragging();
			}
			done = true;
		} );
		passed &= result.status == DragStartStatus::Started;
		if ( result.status != DragStartStatus::Started ) {
			std::cerr << "Wayland start failed: " << result.error << '\n';
			done = true;
		}
		passed &= drag.startDrag( offer ).status == DragStartStatus::Busy;
		if ( scenario == Instant ) {
			drag.cancelDrag();
			passed &= completions == 0;
		}
		cancelPending = scenario == Cancel || scenario == StalledCancel;
		destroyPending = scenario == Destroy;
		std::cout << "WAYLAND_DND_STARTED " << scenarios[scenario] << std::endl;
		// Weston was stopped before startDrag(), so no setup acknowledgement can have arrived.
		// Avoid rendering or pumping SDL until the runner requests cancellation.
		if ( scenario == StalledCancel )
			passed &= waitForRunner();
	}

	void draw() {
		for ( auto* window : { source, target } ) {
			if ( window && window->isOpen() ) {
				auto current = Engine::instance()->makeWindowCurrent( window );
				window->setClearColor( window == source ? sourceColor : targetColor );
				window->clear();
				window->display();
			}
		}
	}

	int run( bool processExit = false ) {
		exitingWithBackend = processExit;
		if ( !initialize() )
			return 1;
		if ( !DragAndDrop::forWindow( *source ).getCapabilities().fileSource )
			return TestSkipped;
		if ( processExit )
			scenario = Shutdown;
		prepare();
		while ( scenario < std::size( scenarios ) && passed ) {
			Engine::instance()->updateInput();
			if ( source && DragAndDrop::forWindow( *source ).isDragging() ) {
				passed &= !( source->getInput()->getClickTrigger() & EE_BUTTON_LMASK );
				passed &= !( source->getInput()->getDoubleClickTrigger() & EE_BUTTON_LMASK );
			}
			if ( clickPending && clickReleases ) {
				passed &= clickPresses == 1 && clickReleases == 1;
				passed &= ( source->getInput()->getClickTrigger() & EE_BUTTON_LMASK ) != 0;
				clickPending = false;
				std::cout << "WAYLAND_DND_CLICK " << ( passed ? "PASS" : "FAIL" ) << std::endl;
			}
			if ( scenario == Shutdown && DragAndDrop::forWindow( *source ).isDragging() ) {
				if ( processExit ) {
					// std::exit deliberately skips stack destructors and leaves Engine/windows
					// alive. Its registered handlers must release the native session without a
					// completion.
					removeFixtures();
					std::cout << "WAYLAND_DND_PROCESS_EXIT" << std::endl;
					std::exit( passed ? 0 : 1 );
				}
				Engine::destroySingleton();
				source = target = nullptr;
				passed &= completions == 0;
				std::cout << "WAYLAND_DND_RESULT shutdown " << ( passed ? "PASS" : "FAIL" )
						  << " completions=" << completions << std::endl;
				return passed ? 0 : 1;
			}
			if ( cancelPending ) {
				cancelPending = false;
				DragAndDrop::forWindow( *source ).cancelDrag();
				if ( scenario == StalledCancel )
					std::cout << "WAYLAND_DND_CLEANUP_DONE" << std::endl;
			}
			if ( destroyPending ) {
				destroyPending = false;
				auto* destroyed = source;
				source = nullptr;
				Engine::instance()->destroyWindow( destroyed );
			}
			if ( done && !reported ) {
				passed &= completions == 1;
				const size_t expected =
					( scenario == Snapshot || scenario == Large ) ? offer.files.size() : 0;
				passed &= received.size() == expected;
				for ( size_t i = 0; i < std::min( received.size(), expected ); ++i )
					passed &= received[i] == offer.files[i].path;
				reported = true;
				std::cout << "WAYLAND_DND_RESULT " << scenarios[scenario] << ' '
						  << ( passed ? "PASS" : "FAIL" ) << " files=" << received.size()
						  << " completions=" << completions << std::endl;
			}
			pollfd input{ STDIN_FILENO, POLLIN, 0 };
			if ( reported && poll( &input, 1, 0 ) > 0 && ( input.revents & POLLIN ) ) {
				char command[16];
				if ( read( STDIN_FILENO, command, sizeof( command ) ) <= 0 )
					return 1;
				if ( command[0] == 'c' ) {
					clickPending = true;
					clickPresses = clickReleases = 0;
					std::cout << "WAYLAND_DND_CLICK_READY" << std::endl;
					continue;
				}
				++scenario;
				if ( scenario < std::size( scenarios ) )
					prepare();
			}
			if ( TestClock::now() > deadline ) {
				std::cerr << "Wayland scenario timed out\n";
				return 1;
			}
			draw();
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		}
		return passed ? 0 : 1;
	}
};
} // namespace

int runAutomatedWaylandDragTests( bool receiver, bool processExit ) {
	if ( !receiver ) {
		WaylandChecks checks;
		return checks.run( processExit );
	}
	auto* window = Engine::instance()->createWindow(
		{ 320, 220, "eepp Wayland external target", WindowStyle::Borderless } );
	if ( !window || !window->isOpen() )
		return 1;
	window->setClearColor( externalColor );
	SmallVector<std::string, 2> received;
	window->getInput()->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::FileDropped )
			received.emplace_back( FileSystem::fileNameFromPath( event->file.file ) );
	} );
	std::cout << "WAYLAND_DND_EXTERNAL_READY" << std::endl;
	const auto deadline = TestClock::now() + std::chrono::seconds( 20 );
	while ( received.size() < 2 && TestClock::now() < deadline ) {
		Engine::instance()->updateInput();
		window->clear();
		window->display();
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
	}
	const bool passed =
		received == SmallVector<std::string, 2>{ "sample #\xc3\xa9-one.txt", "sample two.txt" };
	Engine::destroySingleton();
	std::cout << "WAYLAND_DND_EXTERNAL " << ( passed ? "PASS" : "FAIL" ) << std::endl;
	return passed ? 0 : 1;
}
