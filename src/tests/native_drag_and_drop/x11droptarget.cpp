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
#undef None
#undef Status
#undef Time
#undef Window
#undef Font
#undef Cursor
#undef Display

#include "../../eepp/window/draganddrop/dragaccess.hpp"
#include "../../eepp/window/draganddrop/x11library.hpp"
#include <chrono>
#include <eepp/window.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>

using namespace EE;
using namespace EE::Window;

#define EE_X11_TEST_FUNCTIONS( F ) \
	F( XChangeProperty )           \
	F( XCloseDisplay )             \
	F( XCreateSimpleWindow )       \
	F( XDestroyWindow )            \
	F( XFlush )                    \
	F( XFree )                     \
	F( XGetWindowProperty )        \
	F( XInternAtom )               \
	F( XNextEvent )                \
	F( XOpenDisplay )              \
	F( XPending )                  \
	F( XSelectInput )              \
	F( XSendEvent )                \
	F( XSetSelectionOwner )        \
	F( XSync )

namespace {

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
#undef EE_X11_TEST_FUNCTIONS

/** An XDND drag source on its own connection, talking to eepp's window directly (no pointer).
 * Selection requests are answered by onRequest; replies can be held back to answer later. */
class TestDragSource {
  public:
	TestDragSource( X11TestFunctions& x11, ::X11Display* peer ) : x11( x11 ), peer( peer ) {
		window = x11.XCreateSimpleWindow( peer, DefaultRootWindow( peer ), 0, 0, 1, 1, 0, 0, 0 );
		x11.XSelectInput( peer, window, PropertyChangeMask );
	}

	~TestDragSource() { x11.XDestroyWindow( peer, window ); }

	Atom atom( const char* name ) { return x11.XInternAtom( peer, name, False ); }

	/** A current server timestamp, from a property change: selection requests are refused for
	 * times earlier than the selection's ownership or later than the server's clock. */
	unsigned long serverTime() {
		const Atom marker = atom( "EEPP_TEST_TIME" );
		x11.XChangeProperty( peer, window, marker, XA_STRING, 8, PropModeReplace, nullptr, 0 );
		x11.XSync( peer, False );
		for ( ;; ) {
			XEvent event;
			x11.XNextEvent( peer, &event );
			if ( event.type == PropertyNotify && event.xproperty.atom == marker )
				return event.xproperty.time;
			handle( event );
		}
	}

	void own() {
		x11.XSetSelectionOwner( peer, atom( "XdndSelection" ), window, serverTime() );
		x11.XSync( peer, False );
	}

	void setProperty( const char* name, const char* type, const std::string& value ) {
		x11.XChangeProperty( peer, window, atom( name ), atom( type ), 8, PropModeReplace,
							 reinterpret_cast<const unsigned char*>( value.data() ),
							 static_cast<int>( value.size() ) );
	}

	std::string property( ::X11Window owner, Atom name ) {
		Atom type = 0;
		int format = 0;
		unsigned long count = 0, after = 0;
		unsigned char* data = nullptr;
		x11.XGetWindowProperty( peer, owner, name, 0, 1 << 16, False, AnyPropertyType, &type,
								&format, &count, &after, &data );
		std::string value( data ? reinterpret_cast<char*>( data ) : "", data ? count : 0 );
		if ( data )
			x11.XFree( data );
		return value;
	}

	void message( ::X11Window target, const char* type, std::array<long, 5> data ) {
		XEvent event{};
		event.xclient.type = ClientMessage;
		event.xclient.window = target;
		event.xclient.message_type = atom( type );
		event.xclient.format = 32;
		std::copy( data.begin(), data.end(), event.xclient.data.l );
		x11.XSendEvent( peer, target, False, NoEventMask, &event );
		x11.XFlush( peer );
	}

	/** XdndEnter offering types (up to three), then a position over the window. */
	void enter( ::X11Window target, const std::vector<Atom>& types, int rootX, int rootY ) {
		std::array<long, 5> data{ static_cast<long>( window ), 5L << 24, 0, 0, 0 };
		for ( size_t i = 0; i < types.size() && i < 3; ++i )
			data[2 + i] = static_cast<long>( types[i] );
		message( target, "XdndEnter", data );
		message( target, "XdndPosition",
				 { static_cast<long>( window ), 0, ( rootX << 16 ) | rootY, CurrentTime,
				   static_cast<long>( atom( "XdndActionCopy" ) ) } );
	}

	void drop( ::X11Window target, unsigned long time ) {
		message( target, "XdndDrop",
				 { static_cast<long>( window ), 0, static_cast<long>( time ), 0, 0 } );
	}

	/** Stores value in the requested property and notifies the requestor. */
	void reply( const XSelectionRequestEvent& request, const std::string& value,
				const char* type = "text/plain" ) {
		x11.XChangeProperty( peer, request.requestor, request.property, atom( type ),
							 type == std::string( "INCR" ) ? 32 : 8, PropModeReplace,
							 reinterpret_cast<const unsigned char*>( value.data() ),
							 static_cast<int>( type == std::string( "INCR" ) ? 1 : value.size() ) );
		XEvent event{};
		event.xselection.type = SelectionNotify;
		event.xselection.requestor = request.requestor;
		event.xselection.selection = request.selection;
		event.xselection.target = request.target;
		event.xselection.property = request.property;
		event.xselection.time = request.time;
		x11.XSendEvent( peer, request.requestor, False, NoEventMask, &event );
		x11.XFlush( peer );
	}

	void handle( const XEvent& event ) {
		if ( event.type == SelectionRequest ) {
			requests.push_back( event.xselectionrequest );
			if ( onRequest )
				onRequest( event.xselectionrequest );
		} else if ( event.type == ClientMessage ) {
			if ( event.xclient.message_type == atom( "XdndStatus" ) )
				accepted = event.xclient.data.l[1] & 1;
			if ( event.xclient.message_type == atom( "XdndFinished" ) )
				finished.push_back( event.xclient.data.l[1] & 1 );
		}
	}

	void pump() {
		while ( x11.XPending( peer ) ) {
			XEvent event;
			x11.XNextEvent( peer, &event );
			handle( event );
		}
	}

	void clearRecords() {
		requests.clear();
		finished.clear();
		accepted = false;
		onRequest = {};
	}

	X11TestFunctions& x11;
	::X11Display* peer;
	::X11Window window{ 0 };
	std::vector<XSelectionRequestEvent> requests;
	std::vector<bool> finished;
	std::function<void( const XSelectionRequestEvent& )> onRequest;
	bool accepted{ false };
};

} // namespace

/** The receiving side: drags without files reaching setDataDropHandler() and
 * setFileSaveDropHandler(), from a scripted source. */
int runAutomatedNativeDropTests() {
	X11TestFunctions x11;
	if ( !x11.load() )
		return TestSkipped;
	auto* peer = x11.XOpenDisplay( nullptr );
	if ( !peer )
		return TestSkipped;
	auto* window = Engine::instance()->createWindow( { 320, 240, "eepp XDND drop target" } );
	auto* other = Engine::instance()->createWindow( { 160, 120, "eepp XDND second target" } );
	if ( !window || !window->isOpen() || !other || !other->isOpen() )
		return 1;
	window->setPosition( 40, 60 );
	window->show();
	auto dnd = DragAndDrop::forWindow( *window );
	if ( Private::NativeDragAccess::getNativeDragContext( *window ).kind !=
			 NativeWindowContext::Kind::X11 ||
		 !dnd.getCapabilities().dataTarget ) {
		Engine::destroySingleton();
		x11.XCloseDisplay( peer );
		return TestSkipped;
	}
	const auto context = Private::NativeDragAccess::getNativeDragContext( *window );
	const auto target = static_cast<::X11Window>( context.handles.x11.window );
	auto* eeppDisplay = static_cast<::X11Display*>( context.handles.x11.display );
	// The source uses the peer connection: it must be destroyed before the connection closes.
	auto source = std::make_unique<TestDragSource>( x11, peer );
	source->own();
	const Atom typeA = source->atom( "application/x-eepp-test-a" );
	const Atom typeB = source->atom( "application/x-eepp-test-b" );
	const Atom directSave = source->atom( "XdndDirectSave0" );
	const Atom uriList = source->atom( "text/uri-list" );
	const int rootX = window->getPosition().x + 20, rootY = window->getPosition().y + 20;

	auto pumpUntil = [&]( const std::function<bool()>& done ) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
		while ( !done() && std::chrono::steady_clock::now() < deadline ) {
			Engine::instance()->updateInput();
			source->pump();
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
		return done();
	};
	// Lets anything still in flight arrive, such as a delivery that must not happen.
	auto settle = [&] {
		for ( int i = 0; i < 200; ++i ) {
			Engine::instance()->updateInput();
			source->pump();
			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
	};
	auto report = [&]( const char* name, bool success ) {
		std::cout << "Drop target: " << name << " passed=" << success << '\n';
		return success;
	};
	bool passed = true;

	std::vector<DataDrop> drops;
	dnd.setDataDropHandler( { "application/x-eepp-test-a", "application/x-eepp-test-b" },
							[&]( DataDrop drop ) { drops.push_back( std::move( drop ) ); } );
	int otherDrops = 0;
	DragAndDrop::forWindow( *other ).setDataDropHandler( { "application/x-eepp-test-a" },
														 [&]( DataDrop ) { ++otherDrops; } );

	// Both registered types are fetched, in registration order, whatever the offer's order.
	{
		source->clearRecords();
		source->onRequest = [&]( const XSelectionRequestEvent& request ) {
			source->reply( request, request.target == typeA ? "alpha" : "beta" );
		};
		source->enter( target, { typeB, typeA, XA_STRING }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		const bool delivered = pumpUntil( [&] { return !drops.empty(); } );
		const DataDrop* drop = delivered ? &drops.front() : nullptr;
		passed &= report( "data drop", drop && drops.size() == 1 && drop->items.size() == 2 &&
										   drop->items[0].first == "application/x-eepp-test-a" &&
										   *drop->find( "application/x-eepp-test-a" ) == "alpha" &&
										   *drop->find( "application/x-eepp-test-b" ) == "beta" &&
										   drop->position.x >= 0 && drop->position.y >= 0 &&
										   source->finished == std::vector{ true } );
	}

	// An incremental (INCR) reply is not supported: the drop fails, no empty item arrives.
	{
		drops.clear();
		source->clearRecords();
		source->onRequest = [&]( const XSelectionRequestEvent& request ) {
			source->reply( request, std::string( 4, '\0' ), "INCR" );
		};
		source->enter( target, { typeA }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		pumpUntil( [&] { return !source->finished.empty(); } );
		passed &= report( "incremental reply refused",
						  drops.empty() && source->finished == std::vector{ false } );
	}

	// A late reply to an abandoned drop cannot complete the drop that replaced it.
	{
		drops.clear();
		source->clearRecords();
		source->enter( target, { typeA }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		pumpUntil( [&] { return source->requests.size() == 1; } );
		const auto abandoned = source->requests.front();
		// A new drag replaces the unanswered drop.
		source->accepted = false;
		source->enter( target, { typeA }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		pumpUntil( [&] { return source->requests.size() == 2; } );
		const auto current = source->requests.back();
		source->reply( abandoned, "old" );
		source->reply( current, "new" );
		const bool delivered = pumpUntil( [&] { return !drops.empty(); } );
		settle();
		const bool lateDiscarded =
			source->property( target, abandoned.property ).empty() || !abandoned.property;
		passed &= report( "late reply ignored",
						  delivered && drops.size() == 1 &&
							  *drops.front().find( "application/x-eepp-test-a" ) == "new" &&
							  abandoned.property != current.property && lateDiscarded );
	}

	// XDS is preferred over an offered file list: the source saves into the handler's folder.
	{
		std::vector<FileSaveDrop> saves;
		std::optional<std::string> folder = std::string( "/tmp/eepp xds" );
		dnd.setFileSaveDropHandler( [&]( const FileSaveDrop& drop ) {
			saves.push_back( drop );
			return folder;
		} );
		std::string uri;
		source->clearRecords();
		source->setProperty( "XdndDirectSave0", "text/plain", "xds.txt" );
		source->onRequest = [&]( const XSelectionRequestEvent& request ) {
			uri = source->property( source->window, directSave );
			source->reply( request, request.target == directSave ? "S" : "" );
		};
		source->enter( target, { directSave, uriList }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		pumpUntil( [&] { return !source->finished.empty(); } );
		passed &=
			report( "file save drop", saves.size() == 1 && saves.front().fileName == "xds.txt" &&
										  uri == "file:///tmp/eepp%20xds/xds.txt" &&
										  source->requests.size() == 1 &&
										  source->requests[0].target == directSave &&
										  source->finished == std::vector{ true } );

		// Refused by the handler: the source is told, and never asked to save. Sources name
		// the file again for each drag (the previous drop replaced it with its URI).
		saves.clear();
		folder.reset();
		source->clearRecords();
		source->setProperty( "XdndDirectSave0", "text/plain", "xds.txt" );
		source->enter( target, { directSave }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		pumpUntil( [&] { return !source->finished.empty(); } );
		passed &= report( "file save refused", saves.size() == 1 && source->requests.empty() &&
												   source->finished == std::vector{ false } );
		dnd.setFileSaveDropHandler( {} );
	}

	// A drop completed, but not yet delivered, when its data handler goes away. The close request
	// is read in the same event pump as the drop's last reply, and an input callback changes the
	// handlers (onCloseRequest) before deliveries run. Returns whether the source was told the
	// drop succeeded.
	auto dropThenCloseRequest = [&]( const std::function<void()>& onCloseRequest ) {
		drops.clear();
		source->clearRecords();
		const auto closeId = window->getInput()->pushCallback( [&]( InputEvent* event ) {
			if ( event->Type == InputEvent::Window &&
				 event->window.type == InputEvent::WindowClose )
				onCloseRequest();
		} );
		window->setCloseRequestCallback( []( EE::Window::Window* ) { return false; } );
		source->enter( target, { typeA }, rootX, rootY );
		pumpUntil( [&] { return source->accepted; } );
		source->drop( target, source->serverTime() );
		pumpUntil( [&] { return source->requests.size() == 1; } );
		source->reply( source->requests.front(), "queued" );
		source->message(
			target, "WM_PROTOCOLS",
			{ static_cast<long>( source->atom( "WM_DELETE_WINDOW" ) ), CurrentTime, 0, 0, 0 } );
		x11.XSync( peer, False );
		// Both events now wait on eepp's connection: one pump reads them together.
		x11.XSync( eeppDisplay, False );
		pumpUntil( [&] { return !source->finished.empty(); } );
		settle();
		window->getInput()->popCallback( closeId );
		return source->finished == std::vector{ true };
	};

	// Removed: its queued drop is never delivered.
	{
		const bool succeeded = dropThenCloseRequest(
			[&] { DragAndDrop::forWindow( *window ).setDataDropHandler( {}, {} ); } );
		passed &=
			report( "removed handler not called", succeeded && drops.empty() && otherDrops == 0 );
	}

	// Replaced, with other types and callback, while a file save handler keeps the window
	// registered: the queued drop belonged to the previous registration, and reaches neither.
	{
		dnd.setDataDropHandler( { "application/x-eepp-test-a" },
								[&]( DataDrop drop ) { drops.push_back( std::move( drop ) ); } );
		dnd.setFileSaveDropHandler(
			[]( const FileSaveDrop& ) -> std::optional<std::string> { return std::nullopt; } );
		int replacementDrops = 0;
		const bool succeeded = dropThenCloseRequest( [&] {
			DragAndDrop::forWindow( *window ).setDataDropHandler(
				{ "application/x-eepp-test-b" }, [&]( DataDrop ) { ++replacementDrops; } );
		} );
		passed &= report( "replaced handler not called",
						  succeeded && drops.empty() && replacementDrops == 0 && otherDrops == 0 );
		dnd.setDataDropHandler( {}, {} );
		dnd.setFileSaveDropHandler( {} );
	}

	Engine::destroySingleton();
	source.reset();
	x11.XCloseDisplay( peer );
	return passed ? 0 : 1;
}

#else

int runAutomatedNativeDropTests() {
	return TestSkipped;
}

#endif
