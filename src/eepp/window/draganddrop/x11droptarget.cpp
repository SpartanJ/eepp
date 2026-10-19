#include "x11droptarget.hpp"

#ifdef EE_HAS_X11_DRAG_SOURCE

// Keep Xlib's timestamp typedef out of eepp headers, where Time names EE::System::Time.
#define Time X11NativeTime
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#undef Time
#undef None
#undef Status

#include "x11errorscope.hpp"
#include "x11library.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <eepp/core/uri.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/window/input.hpp>
#include <optional>

namespace EE { namespace Window { namespace Private {

namespace {

using System::Clock;
using System::Minutes;
using System::Seconds;

#define EE_X11_TARGET_FUNCTIONS( F ) \
	F( XChangeProperty )             \
	F( XConvertSelection )           \
	F( XDeleteProperty )             \
	F( XFlush )                      \
	F( XFree )                       \
	F( XGetWindowProperty )          \
	F( XInternAtom )                 \
	F( XInternAtoms )                \
	F( XSendEvent )                  \
	F( XSetErrorHandler )            \
	F( XSync )                       \
	F( XTranslateCoordinates )

struct X11TargetFunctions : X11Library {
#define EE_DECLARE_X11_FUNCTION( name ) decltype( &::name ) name{ nullptr };
	EE_X11_TARGET_FUNCTIONS( EE_DECLARE_X11_FUNCTION )
#undef EE_DECLARE_X11_FUNCTION

	bool load() {
		if ( !X11Library::load() )
			return false;
#define EE_LOAD_X11_FUNCTION( name )                           \
	if ( !( name = loadFunction<decltype( name )>( #name ) ) ) \
		return false;
		EE_X11_TARGET_FUNCTIONS( EE_LOAD_X11_FUNCTION )
#undef EE_LOAD_X11_FUNCTION
		return true;
	}
};
#undef EE_X11_TARGET_FUNCTIONS

/** Receives two kinds of drags that SDL's XDND support cannot read:
 *  - Data drops: the drag offers a type the window registered (and no file list). After the drop
 *    each registered type is requested in turn, then the data handler gets them all.
 *  - File save drops (XDS, XdndDirectSave0): the source names a file in a property of its window.
 *    After the drop the file save handler picks a folder, the target writes the file's URI back
 *    into that property and requests XdndDirectSave0, and the source answers once it saved the
 *    file ("S"), or failed ("F" / "E").
 * Each drop receives its data in its own property, and replies must also carry the drop's
 * timestamp: a late reply to an abandoned drop can never complete a newer one. */
class X11DataDropTarget : public DropTarget {
  public:
	bool load() { return x11.load(); }

	bool setDataHandler( const NativeWindowContext& context, std::vector<std::string> mimeTypes,
						 DataDropCallback callback ) override {
		Handler* handler = handlerFor( context, static_cast<bool>( callback ) );
		if ( !handler )
			return !callback;
		std::vector<char*> names;
		names.reserve( mimeTypes.size() );
		for ( const auto& type : mimeTypes )
			names.push_back( const_cast<char*>( type.c_str() ) );
		handler->types.assign( mimeTypes.size(), 0 );
		if ( !names.empty() ) {
			x11.XInternAtoms( display, names.data(), static_cast<int>( names.size() ), False,
							  handler->types.data() );
		}
		// A drop received for the previous registration belongs to it: a drop in transfer
		// fails, and queued ones are discarded with the old generation.
		if ( session && session->handler == handler->id && !session->fileSave )
			finish( false );
		handler->mimeTypes = std::move( mimeTypes );
		handler->dataDropped = std::move( callback );
		handler->dataGeneration = ++lastId;
		discardStaleDrops();
		removeUnused();
		return true;
	}

	bool setFileSaveHandler( const NativeWindowContext& context,
							 FileSaveDropCallback callback ) override {
		Handler* handler = handlerFor( context, static_cast<bool>( callback ) );
		if ( !handler )
			return !callback;
		handler->fileSaveDropped = std::move( callback );
		removeUnused();
		return true;
	}

	void removeWindow( Window* window ) override {
		removeHandlers( [window]( const Handler& handler ) { return handler.owner == window; } );
	}

	bool empty() const override { return handlers.empty(); }

	bool handleEvent( void* nativeEvent ) override {
		auto* event = static_cast<XEvent*>( nativeEvent );
		if ( !display || event->xany.display != display )
			return false;
		if ( event->type == ClientMessage )
			return handleMessage( event->xclient );
		if ( event->type != SelectionNotify || event->xselection.selection != atoms[Selection] )
			return false;
		const auto& selection = event->xselection;
		if ( session && session->stage == Stage::Transfer &&
			 selection.requestor == session->xwindow && selection.time == session->time &&
			 ( !selection.property || selection.property == session->property ) ) {
			receive( selection );
			return true;
		}
		// A late reply to an abandoned drop: discard what it stored.
		auto late =
			std::find( retiredProperties.begin(), retiredProperties.end(), selection.property );
		if ( !selection.property || late == retiredProperties.end() )
			return false;
		XErrorScope scope( x11, display );
		x11.XDeleteProperty( display, selection.requestor, selection.property );
		return true;
	}

	void tick() override {
		// Each callback is looked up when it runs: an earlier one may remove or replace any
		// handler, and drops of a removed or replaced data handler are never delivered.
		for ( auto& [generation, drop] : std::exchange( dataDrops, {} ) ) {
			const Handler* handler = handlerWithData( generation );
			if ( !handler )
				continue;
			const auto callback = handler->dataDropped;
			callback( std::move( drop ) );
		}
		if ( session && session->stage == Stage::ChooseFolder )
			chooseFolder();
		// XDS sources may save the whole file (extract an archive) before they answer.
		if ( session && session->stage == Stage::Transfer &&
			 session->clock.getElapsedTime() >
				 ( session->fileSave ? Minutes( 10 ) : Seconds( 5 ) ) )
			finish( false );
	}

  private:
	enum AtomName {
		Enter,
		Position,
		Status,
		Leave,
		Drop,
		Finished,
		Selection,
		TypeList,
		Copy,
		UriList,
		DirectSave,
		TextPlain,
		Incr,
		AtomCount
	};

	struct Handler {
		/** Never reused: identifies the handler in sessions. */
		Uint64 id;
		/** Never reused: identifies the current data handler registration in queued drops, so a
		 * replaced one never receives them. */
		Uint64 dataGeneration;
		Window* owner;
		::Window xwindow;
		std::vector<std::string> mimeTypes;
		std::vector<Atom> types;
		DataDropCallback dataDropped;
		FileSaveDropCallback fileSaveDropped;
	};

	enum class Stage : Uint8 {
		/** Over the window, until it leaves or drops. */
		Drag,
		/** Dropped (XDS): the handler picks the folder in the next tick(). */
		ChooseFolder,
		/** Dropped: waiting for the source's data. */
		Transfer
	};

	/** One drag over a registered window, from XdndEnter until it leaves or its data arrived. */
	struct Session {
		DataDrop drop;
		/** Data drops: the registered types this drag offers, requested in order. */
		std::vector<std::pair<Atom, std::string>> wanted;
		/** XDS: the file name the source suggested. */
		std::string fileName;
		Clock clock;
		Uint64 handler{ 0 };
		/** Data drops: the data handler registration the drag was accepted for. */
		Uint64 dataGeneration{ 0 };
		::Window xwindow{ 0 };
		::Window source{ 0 };
		/** Where this drop's data is received, unique to the drop. */
		Atom property{ 0 };
		X11NativeTime time{ 0 };
		size_t next{ 0 };
		Uint64 id{ 0 };
		Stage stage{ Stage::Drag };
		bool fileSave{ false };
	};

	/** The handler of context.owner, created when adding. Interns the protocol atoms on first
	 * use, on SDL's display. */
	Handler* handlerFor( const NativeWindowContext& context, bool adding ) {
		auto handler =
			std::find_if( handlers.begin(), handlers.end(), [&context]( const Handler& handler ) {
				return handler.owner == context.owner;
			} );
		if ( handler != handlers.end() )
			return &*handler;
		auto* handlerDisplay = static_cast<Display*>( context.handles.x11.display );
		const auto xwindow = static_cast<::Window>( context.handles.x11.window );
		if ( !adding || !handlerDisplay || !xwindow || ( display && display != handlerDisplay ) )
			return nullptr;
		if ( !display ) {
			display = handlerDisplay;
			const char* names[] = { "XdndEnter",	 "XdndPosition",	"XdndStatus",
									"XdndLeave",	 "XdndDrop",		"XdndFinished",
									"XdndSelection", "XdndTypeList",	"XdndActionCopy",
									"text/uri-list", "XdndDirectSave0", "text/plain",
									"INCR" };
			// Xlib's legacy signature is mutable; XInternAtoms only reads these names.
			x11.XInternAtoms( display, const_cast<char**>( names ), AtomCount, False,
							  atoms.data() );
		}
		handlers.push_back( { ++lastId, 0, context.owner, xwindow, {}, {}, {}, {} } );
		return &handlers.back();
	}

	/** Removes the matching handlers, ending their drag and discarding their queued drops. */
	template <typename Predicate> void removeHandlers( Predicate matches ) {
		if ( session ) {
			const Handler* handler = handlerById( session->handler );
			if ( !handler || matches( *handler ) )
				finish( false );
		}
		handlers.erase( std::remove_if( handlers.begin(), handlers.end(), matches ),
						handlers.end() );
		discardStaleDrops();
	}

	/** Forgets queued drops whose data handler registration is gone. */
	void discardStaleDrops() {
		dataDrops.erase( std::remove_if( dataDrops.begin(), dataDrops.end(),
										 [this]( const auto& pending ) {
											 return !handlerWithData( pending.first );
										 } ),
						 dataDrops.end() );
	}

	/** Forgets the windows left without any handler. */
	void removeUnused() {
		removeHandlers( []( const Handler& handler ) {
			return !handler.dataDropped && !handler.fileSaveDropped;
		} );
	}

	const Handler* handlerById( Uint64 id ) const {
		auto handler = std::find_if( handlers.begin(), handlers.end(),
									 [id]( const Handler& handler ) { return handler.id == id; } );
		return handler != handlers.end() ? &*handler : nullptr;
	}

	/** The handler whose current data registration is generation, if it still has a callback. */
	const Handler* handlerWithData( Uint64 generation ) const {
		auto handler =
			std::find_if( handlers.begin(), handlers.end(), [generation]( const Handler& handler ) {
				return handler.dataGeneration == generation && handler.dataDropped;
			} );
		return handler != handlers.end() ? &*handler : nullptr;
	}

	bool handleMessage( const XClientMessageEvent& message ) {
		if ( message.message_type == atoms[Enter] )
			return enter( message );
		if ( !session || message.window != session->xwindow )
			return false;
		const auto source = static_cast<::Window>( message.data.l[0] );
		const bool current = source == session->source && session->stage == Stage::Drag;
		if ( message.message_type == atoms[Position] ) {
			if ( current )
				position( message );
			return true;
		}
		if ( message.message_type == atoms[Leave] ) {
			if ( current )
				session.reset();
			return true;
		}
		if ( message.message_type == atoms[Drop] ) {
			if ( current )
				drop( message );
			return true;
		}
		return false;
	}

	bool enter( const XClientMessageEvent& message ) {
		if ( session )
			finish( false );
		auto handler =
			std::find_if( handlers.begin(), handlers.end(), [&message]( const Handler& handler ) {
				return handler.xwindow == message.window;
			} );
		if ( handler == handlers.end() )
			return false;
		const auto source = static_cast<::Window>( message.data.l[0] );
		std::vector<Atom> offered;
		if ( message.data.l[1] & 1 ) {
			offered = readTypeList( source );
		} else {
			for ( int i = 2; i <= 4; ++i ) {
				if ( message.data.l[i] )
					offered.push_back( static_cast<Atom>( message.data.l[i] ) );
			}
		}
		auto offers = [&offered]( Atom type ) {
			return std::find( offered.begin(), offered.end(), type ) != offered.end();
		};
		Session next;
		// XDS first: such sources offer a file list only with temporary copies, if at all. Other
		// drags with a file list keep arriving through the backend's own XDND support.
		if ( handler->fileSaveDropped && offers( atoms[DirectSave] ) ) {
			next.fileSave = true;
		} else if ( handler->dataDropped && !offers( atoms[UriList] ) ) {
			for ( size_t i = 0; i < handler->types.size(); ++i ) {
				if ( offers( handler->types[i] ) )
					next.wanted.emplace_back( handler->types[i], handler->mimeTypes[i] );
			}
		}
		if ( !next.fileSave && next.wanted.empty() )
			return false;
		next.handler = handler->id;
		next.dataGeneration = handler->dataGeneration;
		next.xwindow = handler->xwindow;
		next.source = source;
		next.id = ++lastSessionId;
		session = std::move( next );
		return true;
	}

	std::vector<Atom> readTypeList( ::Window source ) {
		const auto list = readProperty( source, atoms[TypeList], XA_ATOM, 32, false );
		if ( !list )
			return {};
		std::vector<Atom> types( list->size() / sizeof( Atom ) );
		std::memcpy( types.data(), list->data(), types.size() * sizeof( Atom ) );
		return types;
	}

	/** A whole property; nullopt when it is missing, of another type or format, or larger than
	 * 1 MiB (incremental transfers are not supported). Xlib returns format 32 items as longs. */
	std::optional<std::string> readProperty( ::Window window, Atom property, Atom type, int format,
											 bool remove ) {
		XErrorScope scope( x11, display );
		Atom actualType = 0;
		int actualFormat = 0;
		unsigned long count = 0, after = 0;
		unsigned char* data = nullptr;
		x11.XGetWindowProperty( display, window, property, 0, 1 << 18, remove ? True : False, type,
								&actualType, &actualFormat, &count, &after, &data );
		std::optional<std::string> value;
		if ( data && actualFormat == format && actualType != atoms[Incr] && after == 0 &&
			 ( type == AnyPropertyType || actualType == type ) ) {
			const size_t itemSize = format == 32 ? sizeof( long ) : format / 8;
			value.emplace( reinterpret_cast<const char*>( data ), count * itemSize );
		}
		if ( data )
			x11.XFree( data );
		return value;
	}

	void position( const XClientMessageEvent& message ) {
		const int rootX = static_cast<int>( ( message.data.l[2] >> 16 ) & 0xffff );
		const int rootY = static_cast<int>( message.data.l[2] & 0xffff );
		XErrorScope scope( x11, display );
		int x = 0, y = 0;
		::Window child;
		if ( x11.XTranslateCoordinates( display, DefaultRootWindow( display ), session->xwindow,
										rootX, rootY, &x, &y, &child ) )
			session->drop.position = { static_cast<Float>( x ), static_cast<Float>( y ) };
		// Accept anywhere in the window, and keep receiving positions for the drop location.
		send( atoms[Status], { static_cast<long>( session->xwindow ), 3, 0, 0,
							   static_cast<long>( atoms[Copy] ) } );
	}

	void drop( const XClientMessageEvent& message ) {
		session->time = static_cast<X11NativeTime>( message.data.l[2] );
		// One atom per drop (the server keeps it): replies are told apart by their property.
		const std::string property = "EEPP_DND_DROP_" + std::to_string( session->id );
		session->property = x11.XInternAtom( display, property.c_str(), False );
		if ( !session->fileSave ) {
			session->stage = Stage::Transfer;
			session->clock.restart();
			request( session->wanted[0].first );
			return;
		}
		session->fileName =
			readProperty( session->source, atoms[DirectSave], AnyPropertyType, 8, false )
				.value_or( std::string() );
		// Only a plain name: the folder is ours to choose.
		const auto& name = session->fileName;
		if ( name.empty() || name == "." || name == ".." ||
			 name.find_first_of( std::string( "/\0", 2 ) ) != std::string::npos ) {
			finish( false );
			return;
		}
		session->stage = Stage::ChooseFolder;
		// The consumed events produced no backend event that would wake a waiting loop.
		Input::wakeUpEventLoop();
	}

	void chooseFolder() {
		const Handler* handler = handlerById( session->handler );
		if ( !handler || !handler->fileSaveDropped ) {
			finish( false );
			return;
		}
		// The callback may remove handlers or start another drag: copy what it needs.
		const auto callback = handler->fileSaveDropped;
		const Uint64 id = session->id;
		session->stage = Stage::Transfer;
		session->clock.restart();
		std::optional<std::string> folder =
			callback( FileSaveDrop{ session->fileName, session->drop.position } );
		if ( !session || session->id != id )
			return;
		if ( !folder || folder->empty() ) {
			finish( false );
			return;
		}
		if ( folder->back() != '/' )
			*folder += '/';
		const std::string uri = URI::fromFSPath( *folder + session->fileName ).toString();
		{
			XErrorScope scope( x11, display );
			x11.XChangeProperty( display, session->source, atoms[DirectSave], atoms[TextPlain], 8,
								 PropModeReplace,
								 reinterpret_cast<const unsigned char*>( uri.data() ),
								 static_cast<int>( uri.size() ) );
			if ( !scope.sync() ) {
				finish( false );
				return;
			}
		}
		request( atoms[DirectSave] );
	}

	void request( Atom type ) {
		x11.XConvertSelection( display, atoms[Selection], type, session->property, session->xwindow,
							   session->time );
		x11.XFlush( display );
	}

	void receive( const XSelectionEvent& selection ) {
		const Atom expected =
			session->fileSave ? atoms[DirectSave] : session->wanted[session->next].first;
		if ( selection.target != expected )
			return;
		std::optional<std::string> value;
		if ( selection.property )
			value = readProperty( session->xwindow, session->property, AnyPropertyType, 8, true );
		if ( session->fileSave ) {
			// "S": saved. "F" asks for the data instead (not supported), "E" is an error the
			// source reports itself.
			finish( value == "S" );
		} else if ( !value ) {
			// Refused, unreadable, or sent incrementally (INCR): not an empty item.
			finish( false );
		} else {
			session->drop.items.emplace_back( session->wanted[session->next].second,
											  std::move( *value ) );
			if ( ++session->next < session->wanted.size() )
				request( session->wanted[session->next].first );
			else
				finish( true );
		}
	}

	/** Tells the source how a dropped drag ended, and queues a received data drop for tick(). */
	void finish( bool success ) {
		if ( session->stage != Stage::Drag ) {
			XErrorScope scope( x11, display );
			send( atoms[Finished], { static_cast<long>( session->xwindow ), success ? 1 : 0,
									 success ? static_cast<long>( atoms[Copy] ) : 0, 0, 0 } );
		}
		const bool delivered = success && !session->fileSave;
		if ( delivered )
			dataDrops.emplace_back( session->dataGeneration, std::move( session->drop ) );
		// A request may still be answered: remember its property to discard the reply.
		if ( session->property && !success ) {
			if ( retiredProperties.size() == 8 )
				retiredProperties.erase( retiredProperties.begin() );
			retiredProperties.push_back( session->property );
		}
		session.reset();
		// The consumed events produced no backend event that would wake a waiting loop.
		if ( delivered )
			Input::wakeUpEventLoop();
	}

	void send( Atom type, const std::array<long, 5>& data ) {
		XEvent event{};
		event.xclient.type = ClientMessage;
		event.xclient.display = display;
		event.xclient.window = session->source;
		event.xclient.message_type = type;
		event.xclient.format = 32;
		std::copy( data.begin(), data.end(), event.xclient.data.l );
		x11.XSendEvent( display, session->source, False, NoEventMask, &event );
		x11.XFlush( display );
	}

	X11TargetFunctions x11;
	std::vector<Handler> handlers;
	std::optional<Session> session;
	/** Received data drops for tick(), by data handler generation. */
	std::vector<std::pair<Uint64, DataDrop>> dataDrops;
	/** The properties of the latest abandoned drops, whose replies may still arrive. */
	std::vector<Atom> retiredProperties;
	std::array<Atom, AtomCount> atoms{};
	Display* display{ nullptr };
	Uint64 lastSessionId{ 0 };
	/** Handler ids and data generations share it: neither is ever reused. */
	Uint64 lastId{ 0 };
};

} // namespace

std::unique_ptr<DropTarget> X11DropTarget::create() {
	auto target = std::make_unique<X11DataDropTarget>();
	if ( !target->load() )
		return nullptr;
	return target;
}

}}} // namespace EE::Window::Private

#endif
