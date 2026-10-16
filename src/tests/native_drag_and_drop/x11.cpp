#include "../../eepp/window/draganddrop/x11draganddrop.hpp"
#include "testexitcode.hpp"

#ifdef EE_HAS_X11_DRAG_SOURCE

// Xlib global typedefs conflict with eepp's namespace imports. Keep their ABI but name them
// locally.
#define Time X11Time
#define Window X11Window
#define Font X11Font
#define Cursor X11Cursor
#define Display X11Display
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef None
#undef Status
#undef Time
#undef Window
#undef Font
#undef Cursor
#undef Display

#include "../../eepp/window/draganddrop/dragaccess.hpp"
#include "../../eepp/window/draganddrop/dragmanager.hpp"
#include "../../eepp/window/draganddrop/x11library.hpp"
#include <chrono>
#include <dlfcn.h>
#include <eepp/window.hpp>
#include <iostream>
#include <thread>

using namespace EE;
using namespace EE::Window;

#define EE_X11_TEST_FUNCTIONS( F ) \
	F( XChangeProperty )           \
	F( XCloseDisplay )             \
	F( XConvertSelection )         \
	F( XCreateSimpleWindow )       \
	F( XDeleteProperty )           \
	F( XDestroyWindow )            \
	F( XFlush )                    \
	F( XFree )                     \
	F( XGetImage )                 \
	F( XGetWindowAttributes )      \
	F( XGetWindowProperty )        \
	F( XInternAtom )               \
	F( XMapRaised )                \
	F( XNextEvent )                \
	F( XOpenDisplay )              \
	F( XPending )                  \
	F( XQueryTree )                \
	F( XRaiseWindow )              \
	F( XSelectInput )              \
	F( XSendEvent )                \
	F( XSetInputFocus )            \
	F( XStoreName )                \
	F( XSync )                     \
	F( XUnmapWindow )              \
	F( XMapWindow )                \
	F( XWarpPointer )

namespace {
enum class Scenario : Uint8 {
	SnapshotCopy,
	IncrementalTransfer,
	Rejection,
	Cancellation,
	StalledIncrementalTransfer,
	DeferredBrowserAcceptance,
	ProxyOnlyAwareness,
	InvalidProxyFallback,
	RejectedPropertyWrite,
	MultipleSelectionMetadata,
	DeepWidgetTree,
	DesktopProxyFallback,
};

struct TestCase {
	const char* name;
	Scenario scenario;
};

constexpr TestCase testCases[]{
	{ "Snapshot copy", Scenario::SnapshotCopy },
	{ "Incremental transfer", Scenario::IncrementalTransfer },
	{ "Rejected drop", Scenario::Rejection },
	{ "Cancelled drag", Scenario::Cancellation },
	{ "Stalled incremental transfer", Scenario::StalledIncrementalTransfer },
	{ "Deferred browser acceptance", Scenario::DeferredBrowserAcceptance },
	{ "Proxy-only awareness", Scenario::ProxyOnlyAwareness },
	{ "Invalid proxy fallback", Scenario::InvalidProxyFallback },
	{ "Rejected property write", Scenario::RejectedPropertyWrite },
	{ "MULTIPLE selection metadata", Scenario::MultipleSelectionMetadata },
	{ "Deep widget tree", Scenario::DeepWidgetTree },
	{ "Desktop proxy fallback", Scenario::DesktopProxyFallback },
};

struct X11TestFunctions : Private::X11Library {
#define EE_DECLARE_X11_FUNCTION( name ) decltype( &::name ) name{ nullptr };
	EE_X11_TEST_FUNCTIONS( EE_DECLARE_X11_FUNCTION )
#undef EE_DECLARE_X11_FUNCTION

	bool load() {
		if ( !X11Library::load() )
			return false;
#define EE_LOAD_X11_FUNCTION( name )                           \
	if ( !( name = loadFunction<decltype( name )>( #name ) ) ) \
		return false;
		EE_X11_TEST_FUNCTIONS( EE_LOAD_X11_FUNCTION )
#undef EE_LOAD_X11_FUNCTION
		return true;
	}
};
} // namespace
#undef EE_X11_TEST_FUNCTIONS

/** Independent X11 target and XTest gesture. Its event loop owns a separate connection; eepp's
 * SDL-owned connection is serviced exclusively by the ordinary eepp input pump. */
int runAutomatedNativeDragTests() {
	X11TestFunctions x11;
	if ( !x11.load() )
		return TestSkipped;
	auto* library = dlopen( "libXtst.so.6", RTLD_NOW );
	if ( !library ) {
		std::cerr << "XTest is required for this native integration harness\n";
		return TestSkipped;
	}
	using FakeButton = int ( * )( ::X11Display*, unsigned, int, unsigned long );
	auto fakeButton = reinterpret_cast<FakeButton>( dlsym( library, "XTestFakeButtonEvent" ) );
	// Both SDL backends expose this public pointer query with the same C ABI.
	auto getCursor = reinterpret_cast<void* ( * )()>( dlsym( RTLD_DEFAULT, "SDL_GetCursor" ) );
	auto* peer = x11.XOpenDisplay( nullptr );
	if ( !peer || !fakeButton || !getCursor )
		return TestSkipped;
	auto* window = Engine::instance()->createWindow( { 320, 240, "eepp XDND integration source" } );
	if ( !window || !window->isOpen() )
		return 1;
	window->setPosition( 30, 60 );
	window->show();
	window->clear();
	window->display();
	std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	auto drag = DragAndDrop::forWindow( *window );
	if ( Private::NativeDragAccess::getNativeDragContext( *window ).kind !=
			 NativeWindowContext::Kind::X11 ||
		 !drag.getCapabilities().fileSource ) {
		Engine::destroySingleton();
		x11.XCloseDisplay( peer );
		dlclose( library );
		return TestSkipped;
	}
	auto target =
		x11.XCreateSimpleWindow( peer, DefaultRootWindow( peer ), 500, 400, 300, 200, 0, 0, 0 );
	x11.XStoreName( peer, target, "eepp XDND integration target" );
	x11.XSelectInput( peer, target, PropertyChangeMask );
	auto atom = [&]( const char* name ) { return x11.XInternAtom( peer, name, False ); };
	const auto aware = atom( "XdndAware" ), enter = atom( "XdndEnter" ),
			   position = atom( "XdndPosition" ), status = atom( "XdndStatus" ),
			   drop = atom( "XdndDrop" ), finished = atom( "XdndFinished" ),
			   selection = atom( "XdndSelection" ), uri = atom( "text/uri-list" ),
			   copy = atom( "XdndActionCopy" ), property = atom( "EEPP_TEST_DROP" ),
			   incr = atom( "INCR" );
	const auto proxyAtom = atom( "XdndProxy" );
	const auto multiple = atom( "MULTIPLE" ), timestamp = atom( "TIMESTAMP" ),
			   targets = atom( "TARGETS" ), atomPair = atom( "ATOM_PAIR" ),
			   metadata = atom( "EEPP_TEST_METADATA" ), timeResult = atom( "EEPP_TEST_TIMESTAMP" ),
			   targetsResult = atom( "EEPP_TEST_TARGETS" ), unknown = atom( "EEPP_TEST_UNKNOWN" );
	const auto proxyWindow =
		x11.XCreateSimpleWindow( peer, DefaultRootWindow( peer ), 0, 0, 1, 1, 0, 0, 0 );
	// SDL2's public bridge must remain at its caller-owned state when no drag is active.
	using EventState = Uint8 ( * )( Uint32, int );
	const auto eventState = reinterpret_cast<EventState>( dlsym( RTLD_DEFAULT, "SDL_EventState" ) );
	constexpr Uint32 syswmEvent = 0x201;
	const auto idleEventState = eventState ? eventState( syswmEvent, -1 ) : 0;
	unsigned long version = 5;
	x11.XChangeProperty( peer, target, aware, XA_ATOM, 32, PropModeReplace,
						 reinterpret_cast<unsigned char*>( &version ), 1 );
	x11.XMapRaised( peer, target );
	x11.XSync( peer, False );
	Sizei previewSize{ 112, 48 };
	auto previewWindow = [&]() {
		::X11Window root, parent, *children = nullptr;
		unsigned count = 0;
		::X11Window preview = 0;
		if ( x11.XQueryTree( peer, DefaultRootWindow( peer ), &root, &parent, &children,
							 &count ) ) {
			for ( unsigned i = 0; i < count; ++i ) {
				XWindowAttributes attributes{};
				if ( x11.XGetWindowAttributes( peer, children[i], &attributes ) &&
					 attributes.map_state == IsViewable && attributes.override_redirect &&
					 attributes.width == previewSize.getWidth() &&
					 attributes.height == previewSize.getHeight() )
					preview = children[i];
			}
		}
		if ( children )
			x11.XFree( children );
		return preview;
	};
	bool passed = true;
	for ( const auto& testCase : testCases ) {
		const auto scenario = testCase.scenario;
		// Proxy-only awareness uses a v4 proxy. An invalid proxy must fall back to the physical
		// v5 target when it does not point to itself.
		version = scenario == Scenario::ProxyOnlyAwareness ? 4 : 5;
		x11.XChangeProperty( peer, proxyWindow, aware, XA_ATOM, 32, PropModeReplace,
							 reinterpret_cast<unsigned char*>( &version ), 1 );
		const unsigned long proxySelf =
			scenario == Scenario::ProxyOnlyAwareness ? proxyWindow : target;
		x11.XChangeProperty( peer, proxyWindow, proxyAtom, XA_WINDOW, 32, PropModeReplace,
							 reinterpret_cast<const unsigned char*>( &proxySelf ), 1 );
		if ( scenario == Scenario::ProxyOnlyAwareness ) {
			x11.XDeleteProperty( peer, target, aware );
		} else {
			x11.XChangeProperty( peer, target, aware, XA_ATOM, 32, PropModeReplace,
								 reinterpret_cast<unsigned char*>( &version ), 1 );
		}
		if ( scenario == Scenario::ProxyOnlyAwareness ||
			 scenario == Scenario::InvalidProxyFallback ) {
			x11.XChangeProperty( peer, target, proxyAtom, XA_WINDOW, 32, PropModeReplace,
								 reinterpret_cast<const unsigned char*>( &proxyWindow ), 1 );
		} else {
			x11.XDeleteProperty( peer, target, proxyAtom );
		}
		x11.XSync( peer, False );
		::X11Window deepTree = 0;
		if ( scenario == Scenario::DeepWidgetTree ) {
			// The first aware client routes its deep widget tree. A stray descendant property
			// must not redirect messages away from it or cause per-motion cache churn.
			auto parent = target;
			for ( int depth = 0; depth < 16; ++depth ) {
				const auto child = x11.XCreateSimpleWindow( peer, parent, 0, 0, 300, 200, 0, 0, 0 );
				if ( !deepTree )
					deepTree = child;
				x11.XMapWindow( peer, child );
				parent = child;
			}
			x11.XChangeProperty( peer, parent, aware, XA_ATOM, 32, PropModeReplace,
								 reinterpret_cast<unsigned char*>( &version ), 1 );
		} else if ( scenario == Scenario::DesktopProxyFallback ) {
			// A desktop proxy is a fallback; it must not mask another aware client.
			x11.XChangeProperty( peer, proxyWindow, proxyAtom, XA_WINDOW, 32, PropModeReplace,
								 reinterpret_cast<const unsigned char*>( &proxyWindow ), 1 );
			x11.XChangeProperty( peer, DefaultRootWindow( peer ), proxyAtom, XA_WINDOW, 32,
								 PropModeReplace,
								 reinterpret_cast<const unsigned char*>( &proxyWindow ), 1 );
		}
		x11.XSync( peer, False );
		window->getCursorManager()->set( Cursor::SysArrow );
		previewSize = scenario == Scenario::SnapshotCopy ? Sizei( 48, 32 ) : Sizei( 112, 48 );
		DragOffer offer;
		const int files = scenario == Scenario::IncrementalTransfer ||
								  scenario == Scenario::StalledIncrementalTransfer
							  ? 5000
							  : 2;
		offer.files.reserve( files );
		for ( int i = 0; i < files; ++i )
			offer.files.push_back( { "/tmp/drag file #é-" + std::to_string( i ) + ".txt" } );
		Graphics::Image snapshot( 48, 32, 4, Color::Transparent, false );
		snapshot.fillWithColor( Color( 190, 20, 30, 255 ) );
		if ( scenario == Scenario::SnapshotCopy ) {
			offer.preview.image = &snapshot;
			// Cover the pointer deliberately: the tooltip must not intercept the target.
			offer.preview.hotspot = { 8, 8 };
		}
		std::string expected, received;
		DragAndDrop::encodeFileUriList( offer, expected );
		window->getClipboard()->setText( "eepp-dnd-clipboard" );
		const auto context = Private::NativeDragAccess::getNativeDragContext( *window );
		x11.XRaiseWindow( peer, context.handles.x11.window );
		x11.XSetInputFocus( peer, context.handles.x11.window, RevertToParent, CurrentTime );
		x11.XWarpPointer( peer, 0, context.handles.x11.window, 0, 0, 0, 0, 100, 100 );
		fakeButton( peer, 1, True, CurrentTime );
		x11.XSync( peer, False );
		for ( int pump = 0; pump < 10; ++pump ) {
			window->getInput()->update();
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
		int completions = 0;
		DragResult result;
		if ( scenario == Scenario::SnapshotCopy ) {
			Graphics::Image oversized( 2049, 1, 4 );
			offer.preview.image = &oversized;
			const auto invalid = drag.startDrag( offer, [&]( DragResult ) { ++completions; } );
			passed &= invalid.status == DragStartStatus::InvalidOffer && !completions &&
					  !Private::DragManager::hasSession() && !previewWindow();
			offer.preview.image = &snapshot;
		}
		auto started = drag.startDrag( offer, [&]( DragResult value ) {
			++completions;
			result = std::move( value );
		} );
		if ( started.status != DragStartStatus::Started ) {
			std::cerr << "Start failed: " << started.error << '\n';
			passed = false;
			break;
		}
		const auto preview = previewWindow();
		bool previewPassed = preview != 0 && drag.getCapabilities().dragImage;
		passed &= !eventState || eventState( syswmEvent, -1 ) == 1;
		const auto rejectedCursor = getCursor();
		window->getCursorManager()->set( Cursor::SysIBeam );
		const auto requestedCursor = getCursor();
		window->getInput()->update();
		previewPassed &= getCursor() == rejectedCursor;
		if ( scenario == Scenario::SnapshotCopy && preview ) {
			// The service must own the rendered pixels after start returns.
			snapshot.setPixel( 0, 0, Color::Blue );
			auto* image = x11.XGetImage( peer, preview, 0, 0, 1, 1, AllPlanes, ZPixmap );
			previewPassed &= image && XGetPixel( image, 0, 0 ) == 0xbe141e;
			if ( image )
				XDestroyImage( image );
		}
		if ( scenario != Scenario::SnapshotCopy && preview ) {
			auto* image = x11.XGetImage( peer, preview, 8, 6, 1, 1, AllPlanes, ZPixmap );
			previewPassed &= image && XGetPixel( image, 0, 0 ) == 0xdeeafb;
			if ( image )
				XDestroyImage( image );
		}
		// Concurrent starts must not replace a session or invoke the rejected callback.
		if ( drag.startDrag( { { { "/tmp/other" } } } ).status != DragStartStatus::Busy )
			passed = false;
		::X11Window source = 0;
		bool releaseSent = false, incremental = false, finishedSent = false, earlyRequest = false;
		bool badConversionSent = false, badConversionRejected = false;
		bool metadataSent = false, metadataPassed = false;
		auto reply = [&]( Atom message, long flags, Atom action ) {
			XEvent event{};
			event.xclient.type = ClientMessage;
			event.xclient.window = source;
			event.xclient.message_type = message;
			event.xclient.format = 32;
			event.xclient.data.l[0] = target;
			event.xclient.data.l[1] = flags;
			event.xclient.data.l[message == status ? 4 : 2] = action;
			x11.XSendEvent( peer, source, False, NoEventMask, &event );
			x11.XFlush( peer );
		};
		auto read = [&]( bool selectionEvent ) {
			Atom actual;
			int format;
			unsigned long count, remaining;
			unsigned char* bytes = nullptr;
			x11.XGetWindowProperty( peer, target, property, 0, 1024 * 1024, False, AnyPropertyType,
									&actual, &format, &count, &remaining, &bytes );
			if ( actual == incr ) {
				incremental = true;
				// The stalled-transfer case deliberately withholds the INCR acknowledgement. The
				// source must terminate the stalled stream while the gesture remains active.
				if ( scenario != Scenario::StalledIncrementalTransfer )
					x11.XDeleteProperty( peer, target, property );
			} else if ( actual == uri && format == 8 ) {
				received.append( reinterpret_cast<char*>( bytes ), count );
				x11.XDeleteProperty( peer, target, property );
				if ( ( ( !incremental && selectionEvent ) || ( incremental && count == 0 ) ) &&
					 !( scenario == Scenario::DeferredBrowserAcceptance && !releaseSent ) ) {
					if ( scenario == Scenario::StalledIncrementalTransfer )
						drag.cancelDrag();
					else
						reply( finished, 1, copy );
					finishedSent = true;
				}
			}
			if ( bytes )
				x11.XFree( bytes );
			x11.XFlush( peer );
		};
		x11.XWarpPointer( peer, 0, target, 0, 0, 0, 0, 120, 100 );
		x11.XFlush( peer );
		const auto deadline =
			std::chrono::steady_clock::now() +
			std::chrono::seconds( scenario == Scenario::StalledIncrementalTransfer ? 15 : 5 );
		while ( !completions && std::chrono::steady_clock::now() < deadline ) {
			window->getInput()->update();
			while ( x11.XPending( peer ) ) {
				XEvent event;
				x11.XNextEvent( peer, &event );
				if ( event.type == ClientMessage && event.xclient.message_type == enter ) {
					source = event.xclient.data.l[0];
					passed &=
						event.xclient.window == target &&
						static_cast<unsigned long>( event.xclient.data.l[1] ) >> 24 == version;
				}
				if ( event.type == ClientMessage && event.xclient.message_type == position ) {
					if ( !drag.isDragging() )
						continue;
					if ( !releaseSent && preview ) {
						XWindowAttributes attributes{};
						previewPassed &= x11.XGetWindowAttributes( peer, preview, &attributes ) &&
										 attributes.x == 620 - offer.preview.hotspot.x &&
										 attributes.y == 500 - offer.preview.hotspot.y;
					}
					const bool accept =
						scenario != Scenario::Rejection &&
						( scenario != Scenario::DeferredBrowserAcceptance || !received.empty() );
					reply( status, accept ? 1 : 0, copy );
					if ( !releaseSent ) {
						// Observe feedback while still held; pumping after release can complete
						// the drop and restore the normal cursor within this same update.
						// XFlush queues the peer's reply; it does not guarantee that SDL has
						// received it. Allow bounded delivery time before checking feedback.
						const auto feedbackMatches = [&] {
							return !accept ? getCursor() == rejectedCursor
										   : getCursor() != rejectedCursor &&
												 getCursor() != requestedCursor;
						};
						const auto feedbackDeadline =
							std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
						do {
							window->getInput()->update();
							if ( feedbackMatches() )
								break;
							std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
						} while ( std::chrono::steady_clock::now() < feedbackDeadline );
						previewPassed &= feedbackMatches();
						if ( scenario == Scenario::StalledIncrementalTransfer ) {
							x11.XConvertSelection( peer, selection, uri, property, target,
												   CurrentTime );
						} else if ( scenario == Scenario::DeferredBrowserAcceptance && !accept ) {
							// Emulate browser URI loading, then hold still until a position refresh
							// exposes acceptance. No second pointer movement initiates that
							// refresh.
							if ( !earlyRequest ) {
								x11.XConvertSelection( peer, selection, uri, property, target,
													   CurrentTime );
							}
							earlyRequest = true;
						} else if ( scenario == Scenario::RejectedPropertyWrite &&
									!badConversionSent ) {
							// A deliberately malformed peer request must receive failure, not a
							// successful SelectionNotify for a property the server did not store.
							XEvent request{};
							request.xselectionrequest.type = SelectionRequest;
							request.xselectionrequest.owner = source;
							request.xselectionrequest.requestor = target;
							request.xselectionrequest.selection = selection;
							request.xselectionrequest.target = uri;
							request.xselectionrequest.property = 0x7ffffff0;
							x11.XSendEvent( peer, source, False, NoEventMask, &request );
							badConversionSent = true;
						} else if ( scenario == Scenario::MultipleSelectionMetadata &&
									!metadataSent ) {
							Atom pairs[] = { timestamp,		timeResult, targets,
											 targetsResult, unknown,	property };
							x11.XChangeProperty(
								peer, target, metadata, atomPair, 32, PropModeReplace,
								reinterpret_cast<const unsigned char*>( pairs ), 6 );
							x11.XConvertSelection( peer, selection, multiple, metadata, target,
												   CurrentTime );
							metadataSent = true;
						} else {
							if ( scenario == Scenario::Cancellation )
								drag.cancelDrag();
							fakeButton( peer, 1, False, CurrentTime );
						}
						releaseSent =
							( scenario != Scenario::DeferredBrowserAcceptance || accept ) &&
							( scenario != Scenario::RejectedPropertyWrite ||
							  badConversionRejected ) &&
							( scenario != Scenario::MultipleSelectionMetadata || metadataPassed );
						x11.XFlush( peer );
					}
				}
				if ( event.type == ClientMessage && event.xclient.message_type == drop ) {
					passed &= event.xclient.data.l[2] != CurrentTime;
					if ( scenario == Scenario::DeferredBrowserAcceptance ) {
						reply( finished, 1, copy );
						finishedSent = true;
					} else {
						x11.XConvertSelection( peer, selection, uri, property, target,
											   CurrentTime );
					}
				}
				if ( event.type == SelectionNotify ) {
					if ( scenario == Scenario::MultipleSelectionMetadata &&
						 event.xselection.target == multiple ) {
						auto inspectProperty = [&]( Atom name, Atom type, unsigned long length,
													auto validate ) {
							Atom actual;
							int format;
							unsigned long count, remaining;
							unsigned char* bytes = nullptr;
							const bool valid =
								x11.XGetWindowProperty( peer, target, name, 0, length, True, type,
														&actual, &format, &count, &remaining,
														&bytes ) == Success &&
								actual == type && format == 32 && count == length && !remaining &&
								validate( reinterpret_cast<unsigned long*>( bytes ) );
							if ( bytes )
								x11.XFree( bytes );
							return valid;
						};
						metadataPassed =
							event.xselection.property == metadata &&
							inspectProperty( metadata, atomPair, 6,
											 [&]( const auto* pairs ) {
												 return pairs[0] == timestamp &&
														pairs[1] == timeResult &&
														pairs[2] == targets &&
														pairs[3] == targetsResult &&
														pairs[4] == unknown && pairs[5] == 0;
											 } ) &&
							inspectProperty( timeResult, XA_INTEGER, 1,
											 []( const auto* value ) { return value[0] != 0; } ) &&
							inspectProperty( targetsResult, XA_ATOM, 4, [&]( const auto* values ) {
								return values[0] == uri && values[1] == targets &&
									   values[2] == timestamp && values[3] == multiple;
							} );
						passed &= metadataPassed;
						fakeButton( peer, 1, False, CurrentTime );
						releaseSent = true;
						x11.XFlush( peer );
					} else if ( scenario == Scenario::RejectedPropertyWrite && !releaseSent ) {
						badConversionRejected = event.xselection.property == 0;
						passed &= badConversionRejected;
						fakeButton( peer, 1, False, CurrentTime );
						releaseSent = true;
						x11.XFlush( peer );
					} else {
						read( true );
					}
				}
				if ( event.type == PropertyNotify && event.xproperty.atom == property &&
					 event.xproperty.state == PropertyNewValue && incremental && !finishedSent )
					read( false );
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
		const bool success =
			completions == 1 &&
			( scenario == Scenario::Rejection || scenario == Scenario::Cancellation ||
					  scenario == Scenario::StalledIncrementalTransfer
				  ? result.reason == DragEndReason::Cancelled
				  : result.reason == DragEndReason::Dropped &&
						result.performedAction == DragAction::Copy && received == expected ) &&
			window->getClipboard()->getText() == "eepp-dnd-clipboard" &&
			( scenario != Scenario::StalledIncrementalTransfer ||
			  ( incremental && finishedSent && received.empty() ) ) &&
			( scenario != Scenario::RejectedPropertyWrite || badConversionRejected ) &&
			( scenario != Scenario::MultipleSelectionMetadata || metadataPassed ) &&
			previewPassed && !previewWindow();
		passed &= getCursor() == requestedCursor;
		passed &= !eventState || eventState( syswmEvent, -1 ) == idleEventState;
		std::cout << "Test " << testCase.name << " completions=" << completions
				  << " bytes=" << received.size() << " incremental=" << incremental
				  << " preview=" << previewPassed << " passed=" << success << '\n';
		passed &= success;
		if ( deepTree )
			x11.XDestroyWindow( peer, deepTree );
		if ( scenario == Scenario::DesktopProxyFallback )
			x11.XDeleteProperty( peer, DefaultRootWindow( peer ), proxyAtom );
		drag.cancelDrag();
		window->getInput()->update();
		fakeButton( peer, 1, False, CurrentTime );
		x11.XSync( peer, False );
	}
	x11.XDeleteProperty( peer, target, proxyAtom );
	x11.XUnmapWindow( peer, target );
	auto* destination =
		Engine::instance()->createWindow( { 300, 200, "eepp SDL legacy drop target" } );
	destination->setPosition( 500, 400 );
	destination->show();
	destination->clear();
	destination->display();
	const auto originContext = Private::NativeDragAccess::getNativeDragContext( *window );
	const auto destinationContext = Private::NativeDragAccess::getNativeDragContext( *destination );
	SmallVector<std::string, 2> legacyFiles;
	destination->getInput()->pushCallback( [&]( InputEvent* event ) {
		if ( event->Type == InputEvent::FileDropped ) {
			// SDL releases this transient memory after dispatch. Receiving owns a copy.
			legacyFiles.emplace_back( event->file.file );
		}
	} );
	x11.XRaiseWindow( peer, originContext.handles.x11.window );
	x11.XSetInputFocus( peer, originContext.handles.x11.window, RevertToParent, CurrentTime );
	x11.XWarpPointer( peer, 0, originContext.handles.x11.window, 0, 0, 0, 0, 100, 100 );
	fakeButton( peer, 1, True, CurrentTime );
	x11.XSync( peer, False );
	for ( int pump = 0; pump < 10; ++pump ) {
		Engine::instance()->updateInput();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	int legacyCompletions = 0;
	DragResult legacyResult;
	auto legacyStart = drag.startDrag(
		{ { { "/tmp/legacy #é-one.txt" }, { "/tmp/legacy-two.txt" } } }, [&]( DragResult result ) {
			++legacyCompletions;
			legacyResult = std::move( result );
		} );
	x11.XRaiseWindow( peer, destinationContext.handles.x11.window );
	x11.XWarpPointer( peer, 0, destinationContext.handles.x11.window, 0, 0, 0, 0, 120, 100 );
	x11.XFlush( peer );
	for ( int pump = 0; pump < 100; ++pump ) {
		Engine::instance()->updateInput();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	fakeButton( peer, 1, False, CurrentTime );
	x11.XFlush( peer );
	const auto legacyDeadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
	while ( !legacyCompletions && std::chrono::steady_clock::now() < legacyDeadline ) {
		Engine::instance()->updateInput();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	const bool legacyPassed =
		legacyStart.status == DragStartStatus::Started && legacyCompletions == 1 &&
		legacyResult.reason == DragEndReason::Dropped && legacyFiles.size() == 2 &&
		legacyFiles[0] == "/tmp/legacy #é-one.txt" && legacyFiles[1] == "/tmp/legacy-two.txt";
	std::cout << "SDL legacy target: completions=" << legacyCompletions
			  << " files=" << legacyFiles.size() << " passed=" << legacyPassed << '\n';
	passed &= legacyPassed;
	drag.cancelDrag();
	Engine::instance()->updateInput();
	Engine::instance()->destroyWindow( destination );
	// Destruction cancels the session before SDL closes its native resources. Completion remains
	// owned and is delivered once; recursive input polling from the callback is safe.
	x11.XRaiseWindow( peer, originContext.handles.x11.window );
	x11.XSetInputFocus( peer, originContext.handles.x11.window, RevertToParent, CurrentTime );
	x11.XWarpPointer( peer, 0, originContext.handles.x11.window, 0, 0, 0, 0, 100, 100 );
	fakeButton( peer, 1, True, CurrentTime );
	x11.XSync( peer, False );
	for ( int pump = 0; pump < 10; ++pump ) {
		Engine::instance()->updateInput();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	int destructionCompletions = 0;
	bool destructionPassed = false;
	auto destructionStart =
		drag.startDrag( { { { "/tmp/destruction.txt" } } }, [&]( DragResult result ) {
			++destructionCompletions;
			destructionPassed =
				result.reason == DragEndReason::Cancelled && !Private::DragManager::hasSession();
			Engine::instance()->updateInput();
		} );
	Engine::instance()->destroyWindow( window );
	Engine::instance()->updateInput();
	Engine::instance()->updateInput();
	fakeButton( peer, 1, False, CurrentTime );
	x11.XSync( peer, False );
	passed &= destructionStart.status == DragStartStatus::Started && destructionPassed &&
			  destructionCompletions == 1;
	std::cout << "Source destruction and nested callback: completions=" << destructionCompletions
			  << " passed=" << destructionPassed << '\n';
	// Engine shutdown must release an active source without calling observers after UI teardown.
	auto* shutdownWindow =
		Engine::instance()->createWindow( { 320, 240, "eepp XDND shutdown source" } );
	const auto shutdownContext = Private::NativeDragAccess::getNativeDragContext( *shutdownWindow );
	x11.XRaiseWindow( peer, shutdownContext.handles.x11.window );
	x11.XSetInputFocus( peer, shutdownContext.handles.x11.window, RevertToParent, CurrentTime );
	x11.XWarpPointer( peer, 0, shutdownContext.handles.x11.window, 0, 0, 0, 0, 100, 100 );
	fakeButton( peer, 1, True, CurrentTime );
	x11.XSync( peer, False );
	for ( int pump = 0; pump < 10; ++pump ) {
		Engine::instance()->updateInput();
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	int shutdownCompletions = 0;
	const auto shutdownStart = DragAndDrop::forWindow( *shutdownWindow )
								   .startDrag( { { { "/tmp/shutdown.txt" } } },
											   [&]( DragResult ) { ++shutdownCompletions; } );
	Engine::destroySingleton();
	const bool shutdownPassed = shutdownStart.status == DragStartStatus::Started &&
								shutdownCompletions == 0 && !Private::DragManager::hasSession() &&
								!previewWindow();
	passed &= shutdownPassed;
	std::cout << "Engine shutdown: completions=" << shutdownCompletions
			  << " passed=" << shutdownPassed << '\n';
	fakeButton( peer, 1, False, CurrentTime );
	x11.XDestroyWindow( peer, target );
	x11.XDestroyWindow( peer, proxyWindow );
	x11.XCloseDisplay( peer );
	dlclose( library );
	return passed ? 0 : 1;
}

#else

int runAutomatedNativeDragTests() {
	return TestSkipped;
}

#endif
