#include "x11draganddrop.hpp"
#include "dragpreview.hpp"

#ifdef EE_HAS_X11_DRAG_SOURCE

// Keep Xlib's timestamp typedef out of eepp headers, where Time names EE::System::Time.
#define Time X11NativeTime
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#undef Time
#undef None
#undef Status
#include "dragaccess.hpp"
#include "x11library.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <eepp/system/clock.hpp>

namespace EE { namespace Window { namespace Private {

namespace {
using System::Clock;
using System::Milliseconds;
using System::Seconds;
using System::Time;

#define EE_X11_SOURCE_FUNCTIONS( F ) \
	F( XChangeProperty )             \
	F( XCreateSimpleWindow )         \
	F( XDeleteProperty )             \
	F( XDestroyWindow )              \
	F( XFree )                       \
	F( XGetSelectionOwner )          \
	F( XGetWindowAttributes )        \
	F( XGetWindowProperty )          \
	F( XInternAtoms )                \
	F( XLookupKeysym )               \
	F( XMaxRequestSize )             \
	F( XQueryPointer )               \
	F( XSelectInput )                \
	F( XSendEvent )                  \
	F( XSetErrorHandler )            \
	F( XSetSelectionOwner )          \
	F( XSync )

struct X11Functions : X11Library {
#define EE_DECLARE_X11_FUNCTION( name ) decltype( &::name ) name{ nullptr };
	EE_X11_SOURCE_FUNCTIONS( EE_DECLARE_X11_FUNCTION )
#undef EE_DECLARE_X11_FUNCTION

	bool load() {
		if ( !X11Library::load() )
			return false;
#define EE_LOAD_X11_FUNCTION( name )                           \
	if ( !( name = loadFunction<decltype( name )>( #name ) ) ) \
		return false;
		EE_X11_SOURCE_FUNCTIONS( EE_LOAD_X11_FUNCTION )
#undef EE_LOAD_X11_FUNCTION
		return true;
	}
};
#undef EE_X11_SOURCE_FUNCTIONS

/** Foreign windows may disappear between querying them and sending a protocol message. Scope the
 * process-global handler to our own synchronous request batch, and preserve SDL's error handler. */
class XErrorScope {
  public:
	XErrorScope( const X11Functions& x11, Display* display ) : x11( x11 ), display( display ) {
		// Older queued errors are forwarded by serial range; no leading round trip is needed.
		firstRequest = NextRequest( display );
		synchronizedRequest = firstRequest - 1;
		parent = active;
		previous = x11.XSetErrorHandler( handleError );
		active = this;
	}

	~XErrorScope() {
		sync();
		active = parent;
		x11.XSetErrorHandler( previous );
	}

	/** Confirm property writes before sending SelectionNotify, as required by ICCCM. */
	bool sync() {
		// Explicit validation can drain the batch before scope exit. Only synchronize again
		// if another request was issued, such as the subsequent SelectionNotify reply.
		if ( NextRequest( display ) != synchronizedRequest ) {
			x11.XSync( display, False );
			synchronizedRequest = NextRequest( display );
		}
		return !failed;
	}

  private:
	static int handleError( Display* display, XErrorEvent* event ) {
		for ( auto* scope = active; scope; scope = scope->parent ) {
			if ( display == scope->display && event->serial >= scope->firstRequest ) {
				scope->failed = true;
				return 0;
			}
			// Nested scopes share this handler. Forward unrelated connections/requests only
			// through the outermost original handler, avoiding recursion into ourselves.
			if ( !scope->parent && scope->previous )
				return scope->previous( display, event );
		}
		return 0;
	}

	const X11Functions& x11;
	Display* display;
	XErrorHandler previous;
	XErrorScope* parent;
	unsigned long firstRequest;
	unsigned long synchronizedRequest{ 0 };
	bool failed{ false };
	static inline XErrorScope* active{ nullptr };
};

class X11DragSource : public DragSource {
  public:
	X11DragSource( NativeWindowContext context, std::string payload,
				   DragCompletionCallback callback ) :
		DragSource( context.owner, std::move( callback ) ),
		payload( std::move( payload ) ),
		display( static_cast<Display*>( context.handles.x11.display ) ),
		origin( static_cast<Uint32>( context.handles.x11.window ) ) {}

	~X11DragSource() override { release(); }

	DragStartResult start( const DragOffer& offer ) {
		if ( !x11.load() )
			return { "The X11 client library is unavailable or incomplete",
					 DragStartStatus::Unsupported };
		::Window root, child;
		int wx, wy;
		unsigned mask;
		if ( !x11.XQueryPointer( display, origin, &root, &child, &x, &y, &wx, &wy, &mask ) ||
			 !( mask & Button1Mask ) )
			return { "A left-button pointer gesture is required", DragStartStatus::InvalidGesture };
		preview = NativeDragAccess::createNativeDragPreview( *owner, offer, true );
		sourceHandlesDrops = offer.sourceHandlesDrops;
		if ( !preview )
			return { "Could not create the drag preview", DragStartStatus::Failed };
		XErrorScope setup( x11, display );
		const char* names[] = { "XdndAware",	 "XdndProxy",	   "XdndEnter",		"XdndPosition",
								"XdndStatus",	 "XdndLeave",	   "XdndDrop",		"XdndFinished",
								"XdndSelection", "XdndActionCopy", "text/uri-list", "TARGETS",
								"TIMESTAMP",	 "MULTIPLE",	   "INCR",			"ATOM_PAIR",
								"EEPP_DND_TIME", "XdndActionList" };
		// Xlib's legacy signature is mutable; XInternAtoms only reads these names.
		x11.XInternAtoms( display, const_cast<char**>( names ), AtomCount, False, atoms.data() );
		helper = x11.XCreateSimpleWindow( display, root, 0, 0, 1, 1, 0, 0, 0 );
		x11.XSelectInput( display, helper, PropertyChangeMask );
		unsigned long protocolVersion = 5;
		x11.XChangeProperty( display, helper, atoms[Aware], XA_ATOM, 32, PropModeReplace,
							 reinterpret_cast<const unsigned char*>( &protocolVersion ), 1 );
		x11.XChangeProperty( display, helper, atoms[ActionList], XA_ATOM, 32, PropModeReplace,
							 reinterpret_cast<const unsigned char*>( &atoms[Copy] ), 1 );
		x11.XChangeProperty( display, helper, atoms[TimeProperty], XA_INTEGER, 8, PropModeReplace,
							 nullptr, 0 );
		// SDL's implicit XI2 grab cannot be replaced by a core XGrabPointer. Public SDL capture
		// preserves that grab and delivers input through the normal per-window event bridge.
		if ( !NativeDragAccess::acquireNativeDragCapture( *owner->getInput() ) )
			return { "Could not capture the source pointer gesture",
					 DragStartStatus::InvalidGesture };
		pointerGrabbed = true;
		if ( !setup.sync() )
			return { "Could not initialize the XDND selection window", DragStartStatus::Failed };
		chunkSize = std::min<size_t>( 65536, ( x11.XMaxRequestSize( display ) - 64 ) * 4 );
		deadline = clock.getElapsedTime() + Seconds( 10 );
		return { {}, DragStartStatus::Started };
	}

	void cancel() override {
		if ( completed )
			return;
		if ( target && !dropped ) {
			XErrorScope scope( x11, display );
			send( atoms[Leave] );
		}
		finish( DragEndReason::Cancelled );
		release();
	}

	void tick() override {
		if ( completed )
			return;
		if ( clock.getElapsedTime() > deadline &&
			 ( selectionTime == CurrentTime || waitingStatus || dropped || released ) ) {
			finish( DragEndReason::Failed, DragAction::None,
					"XDND target did not finish the drag" );
			release();
			return;
		}
		if ( selectionTime == CurrentTime )
			return;
		if ( !dropped ) {
			::Window pointerRoot, pointerChild;
			int localX, localY, currentX, currentY;
			unsigned pointerMask;
			if ( !x11.XQueryPointer( display, origin, &pointerRoot, &pointerChild, &currentX,
									 &currentY, &localX, &localY, &pointerMask ) ) {
				cancel();
				return;
			}
			// Also detect a release when a backend suppresses a redundant button event.
			if ( !released && !( pointerMask & Button1Mask ) ) {
				released = true;
				dirty = true;
			}
			dirty |= currentX != x || currentY != y;
			x = currentX;
			y = currentY;
			preview->move( x, y );
		}
		bool expired = false;
		for ( const auto& transfer : transfers ) {
			expired |=
				transfer.requestor && clock.getElapsedTime() - transfer.updated > Seconds( 10 );
		}
		// Browsers can reject while asynchronously resolving the URI offer. Recheck the same
		// target while held so the cursor can become Copy without another pointer movement.
		const bool refreshFeedback = !released && target && uriRequested && !accepted &&
									 !waitingStatus && clock.getElapsedTime() >= nextFeedback;
		if ( ( dropped || ( !dirty && !released ) ) && !expired && !refreshFeedback )
			return;
		XErrorScope scope( x11, display );
		if ( !dropped && dirty )
			position();
		else if ( !dropped && refreshFeedback )
			negotiatePosition();
		if ( released && !dropped && !waitingStatus && !dirty ) {
			if ( target && accepted ) {
				// XI2 input can suppress core ButtonRelease. Always supply a server timestamp.
				send( atoms[Drop], 0,
					  releaseTime  ? releaseTime
					  : motionTime ? motionTime
								   : selectionTime );
				dropped = true;
				deadline = clock.getElapsedTime() + Seconds( 10 );
				resetPreview();
				ungrab();
			} else {
				releasedOverSource = !target && overOrigin;
				cancel();
			}
		}
		for ( auto& transfer : transfers ) {
			if ( transfer.requestor && clock.getElapsedTime() - transfer.updated > Seconds( 10 ) )
				terminateTransfer( transfer );
		}
	}

	void handleInput( NativeDragInput input ) override {
		if ( input == NativeDragInput::Escape ) {
			cancel();
		} else if ( !dropped ) {
			if ( input == NativeDragInput::Release )
				released = true;
			dirty = true;
		}
	}

	bool handleEvent( void* raw ) override {
		const auto& event = *static_cast<XEvent*>( raw );
		if ( event.xany.display != display )
			return false;
		switch ( event.type ) {
			case MotionNotify:
				if ( event.xmotion.window == origin && !dropped ) {
					x = event.xmotion.x_root;
					y = event.xmotion.y_root;
					motionTime = event.xmotion.time;
					dirty = true;
				}
				break;
			case ButtonRelease:
				if ( event.xbutton.window == origin && event.xbutton.button == Button1 &&
					 !dropped ) {
					x = event.xbutton.x_root;
					y = event.xbutton.y_root;
					released = true;
					releaseTime = event.xbutton.time;
					// Negotiate once more on release: browsers may initially reject while
					// fetching selection data, then accept once it has arrived.
					dirty = true;
				}
				break;
			case KeyPress:
				if ( event.xkey.window == origin ) {
					auto key = event.xkey;
					if ( x11.XLookupKeysym( &key, 0 ) == XK_Escape )
						cancel();
				}
				break;
			case SelectionClear:
				if ( event.xselectionclear.window == helper &&
					 event.xselectionclear.selection == atoms[Selection] )
					cancel();
				break;
			case SelectionRequest:
				if ( event.xselectionrequest.owner == helper &&
					 event.xselectionrequest.selection == atoms[Selection] ) {
					selection( event.xselectionrequest );
					return true;
				}
				break;
			case PropertyNotify:
				property( event.xproperty );
				if ( event.xproperty.window == helper )
					return true;
				break;
			case ClientMessage:
				if ( event.xclient.window == helper && event.xclient.format == 32 &&
					 static_cast<::Window>( event.xclient.data.l[0] ) == target ) {
					if ( event.xclient.message_type == atoms[Status] && !dropped ) {
						waitingStatus = false;
						accepted = ( event.xclient.data.l[1] & 1 ) &&
								   static_cast<Atom>( event.xclient.data.l[4] ) == atoms[Copy];
						preview->setFeedback( accepted ? DragFeedback::Copy
													   : DragFeedback::Rejected );
					} else if ( event.xclient.message_type == atoms[Finished] && dropped ) {
						const bool success =
							version < 5 ||
							( ( event.xclient.data.l[1] & 1 ) &&
							  static_cast<Atom>( event.xclient.data.l[2] ) == atoms[Copy] );
						finish( success ? DragEndReason::Dropped : DragEndReason::Cancelled,
								DragAction::Copy );
						release();
					}
					return true;
				}
				break;
		}
		return false;
	}

  private:
	enum AtomIndex {
		Aware,
		Proxy,
		Enter,
		Position,
		Status,
		Leave,
		Drop,
		Finished,
		Selection,
		Copy,
		UriList,
		Targets,
		Timestamp,
		Multiple,
		Incr,
		AtomPair,
		TimeProperty,
		ActionList,
		AtomCount
	};

	struct Transfer {
		Time updated;
		size_t offset{ 0 };
		long originalMask{ 0 };
		// XIDs and atoms are CARD32 on the wire, even though Xlib uses unsigned long on LP64.
		Uint32 requestor{ 0 };
		Uint32 property{ 0 };
	};

	struct Destination {
		Uint32 window{ 0 };
		Uint32 proxy{ 0 };
		Uint32 version{ 0 };
	};

	bool readProperty( ::Window window, Atom property, Atom type, unsigned long limit,
					   unsigned char*& bytes, unsigned long& count, int& format ) {
		Atom actual;
		unsigned long remaining;
		bytes = nullptr;
		return x11.XGetWindowProperty( display, window, property, 0, limit, False, type, &actual,
									   &format, &count, &remaining, &bytes ) == Success &&
			   actual == type && remaining == 0;
	}

	unsigned long scalar( ::Window window, Atom property, Atom type ) {
		unsigned char* bytes;
		unsigned long count;
		int format;
		const bool valid = readProperty( window, property, type, 1, bytes, count, format );
		const auto value =
			valid && format == 32 && count == 1 ? *reinterpret_cast<unsigned long*>( bytes ) : 0;
		if ( bytes )
			x11.XFree( bytes );
		return value;
	}

	void send( Atom message, long b = 0, long c = 0, long d = 0, long e = 0 ) {
		XEvent event{};
		event.xclient.type = ClientMessage;
		event.xclient.display = display;
		event.xclient.window = target;
		event.xclient.message_type = message;
		event.xclient.format = 32;
		event.xclient.data.l[0] = helper;
		event.xclient.data.l[1] = b;
		event.xclient.data.l[2] = c;
		event.xclient.data.l[3] = d;
		event.xclient.data.l[4] = e;
		x11.XSendEvent( display, proxy ? proxy : target, False, NoEventMask, &event );
	}

	Destination destination( ::Window window ) {
		for ( const auto& cached : destinations ) {
			if ( cached.window == window )
				return cached;
		}
		::Window delivery = scalar( window, atoms[Proxy], XA_WINDOW );
		if ( delivery && scalar( delivery, atoms[Proxy], XA_WINDOW ) != delivery )
			delivery = 0;
		const unsigned aware = scalar( delivery ? delivery : window, atoms[Aware], XA_ATOM );
		Destination result{ static_cast<Uint32>( window ), static_cast<Uint32>( delivery ),
							std::min( aware, 5u ) };
		destinations[nextDestination] = result;
		nextDestination = ( nextDestination + 1 ) % destinations.size();
		return result;
	}

	void position() {
		dirty = false;
		const auto now = clock.getElapsedTime();
		// Avoid repeated metadata round trips during motion, without hiding property changes
		// for an entire session. Always revalidate proxies/awareness for the final release.
		if ( released || now >= destinationExpiry ) {
			destinations.fill( {} );
			nextDestination = 0;
			destinationExpiry = now + Milliseconds( 250 );
		}

		const ::Window desktop = DefaultRootWindow( display );
		::Window root, child, under = desktop, candidate = 0;
		::Window candidateProxy = 0;
		int rx, ry, wx, wy;
		unsigned mask;
		unsigned candidateVersion = 0;
		const bool wasOverOrigin = overOrigin;
		overOrigin = false;
		// XDND v3+ advertises awareness on the top-level client, which routes to its widgets.
		// Walk through WM containers, then stop at that client instead of traversing its children.
		for ( unsigned depth = 0; under && depth < 64; ++depth ) {
			if ( under == origin || under == helper ) {
				candidate = 0;
				candidateProxy = 0;
				overOrigin = under == origin;
				break;
			}
			// XDND specifies XdndAware on the validated proxy, which can be the only aware
			// window. Keep the physical target XID in every message; only delivery is proxied.
			const auto resolved = destination( under );
			if ( resolved.version >= 3 ) {
				candidate = under;
				candidateProxy = resolved.proxy;
				candidateVersion = resolved.version;
				// Root proxies handle desktop drops. Keep them as a fallback while looking for
				// the client under the pointer; they must not intercept other clients' drops.
				if ( under != desktop )
					break;
			}
			if ( !x11.XQueryPointer( display, under, &root, &child, &rx, &ry, &wx, &wy, &mask ) )
				break;
			under = child;
		}
		// Entering or leaving the source window changes no target (it is never one).
		if ( candidate != target || candidateProxy != proxy || overOrigin != wasOverOrigin ) {
			if ( target )
				send( atoms[Leave] );
			target = candidate;
			version = candidateVersion;
			proxy = candidateProxy;
			accepted = false;
			waitingStatus = false;
			uriRequested = false;
			// Over the source window, an application that handles its own drops resolves it.
			preview->setFeedback( target || ( overOrigin && sourceHandlesDrops )
									  ? DragFeedback::Pending
									  : DragFeedback::Rejected );
			if ( target )
				send( atoms[Enter], static_cast<long>( version ) << 24, atoms[UriList] );
		}
		if ( target ) {
			if ( waitingStatus ) {
				dirty = true;
				return;
			}
			negotiatePosition();
		}
	}

	void negotiatePosition() {
		const auto packed = ( static_cast<unsigned long>( x ) & 0xffff ) << 16 |
							( static_cast<unsigned long>( y ) & 0xffff );
		send( atoms[Position], 0, packed, motionTime, atoms[Copy] );
		waitingStatus = true;
		accepted = false;
		deadline = clock.getElapsedTime() + Seconds( 10 );
		nextFeedback = clock.getElapsedTime() + Milliseconds( 100 );
	}

	bool convert( ::Window requestor, Atom requested, Atom property ) {
		if ( property == 0 )
			return false;
		if ( requested == atoms[Targets] ) {
			Atom supported[] = { atoms[UriList], atoms[Targets], atoms[Timestamp],
								 atoms[Multiple] };
			x11.XChangeProperty( display, requestor, property, XA_ATOM, 32, PropModeReplace,
								 reinterpret_cast<const unsigned char*>( supported ), 4 );
		} else if ( requested == atoms[Timestamp] ) {
			// XChangeProperty's format=32 interface reads native unsigned longs, not Uint32s.
			const unsigned long timestamp = selectionTime;
			x11.XChangeProperty( display, requestor, property, XA_INTEGER, 32, PropModeReplace,
								 reinterpret_cast<const unsigned char*>( &timestamp ), 1 );
		} else if ( requested == atoms[UriList] ) {
			if ( payload.size() <= chunkSize ) {
				x11.XChangeProperty(
					display, requestor, property, atoms[UriList], 8, PropModeReplace,
					reinterpret_cast<const unsigned char*>( payload.data() ), payload.size() );
			} else {
				Transfer* free = nullptr;
				long originalMask = 0;
				bool watched = false;
				for ( auto& transfer : transfers ) {
					if ( !transfer.requestor && !free )
						free = &transfer;
					if ( transfer.requestor == requestor ) {
						if ( transfer.property == property )
							return false;
						watched = true;
						originalMask = transfer.originalMask;
					}
				}
				if ( !free )
					return false;
				if ( !watched ) {
					XWindowAttributes attributes;
					if ( !x11.XGetWindowAttributes( display, requestor, &attributes ) )
						return false;
					originalMask = attributes.your_event_mask;
					x11.XSelectInput( display, requestor, originalMask | PropertyChangeMask );
				}
				*free = { clock.getElapsedTime(), 0, originalMask, static_cast<Uint32>( requestor ),
						  static_cast<Uint32>( property ) };
				unsigned long size = std::min<size_t>( payload.size(), UINT_MAX );
				x11.XChangeProperty( display, requestor, property, atoms[Incr], 32, PropModeReplace,
									 reinterpret_cast<const unsigned char*>( &size ), 1 );
			}
			uriRequested = true;
		} else {
			return false;
		}
		return true;
	}

	void selection( const XSelectionRequestEvent& request ) {
		XErrorScope scope( x11, display );
		Atom property = request.property ? request.property : request.target;
		bool success = false;
		if ( request.target == atoms[Multiple] && request.property ) {
			unsigned char* bytes;
			unsigned long count;
			int format;
			if ( readProperty( request.requestor, property, atoms[AtomPair], 512, bytes, count,
							   format ) &&
				 format == 32 && count % 2 == 0 ) {
				auto* pairs = reinterpret_cast<Atom*>( bytes );
				for ( unsigned long i = 0; i < count; i += 2 ) {
					if ( pairs[i + 1] == property ||
						 !convert( request.requestor, pairs[i], pairs[i + 1] ) )
						pairs[i + 1] = 0;
				}
				x11.XChangeProperty( display, request.requestor, property, atoms[AtomPair], 32,
									 PropModeReplace, bytes, count );
				success = scope.sync();
				if ( !success ) {
					// A storage error refuses this MULTIPLE conversion. Remove its partial
					// results and streams before replying, rather than exposing stale data.
					for ( unsigned long i = 1; i < count; i += 2 )
						discardProperty( request.requestor, pairs[i] );
					discardProperty( request.requestor, property );
				}
			}
			if ( bytes )
				x11.XFree( bytes );
		} else {
			success = convert( request.requestor, request.target, property );
			if ( !scope.sync() ) {
				discardProperty( request.requestor, property );
				success = false;
			}
		}
		XEvent reply{};
		reply.xselection.type = SelectionNotify;
		reply.xselection.display = display;
		reply.xselection.requestor = request.requestor;
		reply.xselection.selection = request.selection;
		reply.xselection.target = request.target;
		reply.xselection.property = success ? property : 0;
		reply.xselection.time = request.time;
		x11.XSendEvent( display, request.requestor, False, NoEventMask, &reply );
	}

	void discardProperty( ::Window requestor, Atom property ) {
		if ( !property )
			return;
		for ( auto& transfer : transfers ) {
			if ( transfer.requestor == requestor && transfer.property == property )
				endTransfer( transfer );
		}
		x11.XDeleteProperty( display, requestor, property );
	}

	void property( const XPropertyEvent& event ) {
		if ( event.window == helper && event.atom == atoms[TimeProperty] &&
			 event.state == PropertyNewValue && selectionTime == CurrentTime ) {
			// Claim with the same genuine server timestamp advertised by TIMESTAMP/Position.
			// CurrentTime here would make our recorded acquisition time precede ownership.
			XErrorScope scope( x11, display );
			x11.XSetSelectionOwner( display, atoms[Selection], helper, event.time );
			if ( x11.XGetSelectionOwner( display, atoms[Selection] ) != helper || !scope.sync() ) {
				finish( DragEndReason::Failed, DragAction::None, "Could not own XdndSelection" );
				release();
				return;
			}
			selectionTime = event.time;
		}
		if ( motionTime == CurrentTime )
			motionTime = selectionTime;
		if ( event.state != PropertyDelete )
			return;
		for ( auto& transfer : transfers ) {
			if ( transfer.requestor != event.window || transfer.property != event.atom )
				continue;
			XErrorScope scope( x11, display );
			const size_t size = std::min( chunkSize, payload.size() - transfer.offset );
			x11.XChangeProperty(
				display, transfer.requestor, transfer.property, atoms[UriList], 8, PropModeReplace,
				reinterpret_cast<const unsigned char*>( payload.data() + transfer.offset ), size );
			transfer.offset += size;
			transfer.updated = clock.getElapsedTime();
			if ( size == 0 )
				endTransfer( transfer );
			if ( !scope.sync() ) {
				finish( DragEndReason::Failed, DragAction::None,
						"Could not write the incremental XDND selection" );
				release();
			}
			break;
		}
	}

	void endTransfer( Transfer& transfer ) {
		const auto requestor = transfer.requestor;
		transfer.requestor = 0;
		for ( const auto& other : transfers ) {
			if ( other.requestor == requestor )
				return;
		}
		x11.XSelectInput( display, requestor, transfer.originalMask );
	}

	void terminateTransfer( Transfer& transfer ) {
		// End an outstanding INCR stream before releasing its event subscription.
		x11.XChangeProperty( display, transfer.requestor, transfer.property, atoms[UriList], 8,
							 PropModeReplace, nullptr, 0 );
		endTransfer( transfer );
	}

	void resetPreview() {
		if ( preview ) {
			preview->reset();
			preview = nullptr;
		}
	}

	void ungrab() {
		if ( pointerGrabbed )
			NativeDragAccess::restoreNativeDragCapture( *owner->getInput() );
		pointerGrabbed = false;
	}

	void release() {
		resetPreview();
		if ( !helper )
			return;
		XErrorScope scope( x11, display );
		ungrab();
		for ( auto& transfer : transfers ) {
			if ( transfer.requestor )
				terminateTransfer( transfer );
		}
		// Destroying the helper releases only selections still owned by it. Querying then
		// clearing with CurrentTime can race with a different client acquiring the selection.
		x11.XDestroyWindow( display, helper );
		helper = 0;
	}

	X11Functions x11;
	NativeDragPreview* preview{ nullptr };
	std::string payload;
	std::array<Transfer, 8> transfers{};
	std::array<Atom, AtomCount> atoms{};
	std::array<Destination, 4> destinations{};
	Clock clock;
	Time deadline;
	Time nextFeedback;
	Time destinationExpiry{};
	Display* display;
	size_t chunkSize{ 0 };
	// Protocol identifiers/timestamps have 32-bit width; promote only at Xlib boundaries.
	Uint32 origin;
	Uint32 helper{ 0 };
	Uint32 target{ 0 };
	Uint32 proxy{ 0 };
	Uint32 selectionTime{ CurrentTime };
	Uint32 motionTime{ CurrentTime };
	Uint32 releaseTime{ CurrentTime };
	int x{ 0 }, y{ 0 };
	unsigned version{ 0 };
	bool pointerGrabbed{ false };
	/** The pointer is over the source window (never a target), as of the last position(). */
	bool overOrigin{ false };
	/** DragOffer::sourceHandlesDrops of the session. */
	bool sourceHandlesDrops{ false };
	bool dirty{ true };
	bool released{ false };
	bool waitingStatus{ false };
	bool accepted{ false };
	bool dropped{ false };
	bool uriRequested{ false };
	Uint8 nextDestination{ 0 };
};
} // namespace

bool X11DragAndDrop::isSupported() {
	// Probe lazily only on an X11 driver. A bool retains no loader reference or function table.
	static const bool supported = [] {
		X11Functions x11;
		return x11.load();
	}();
	return supported;
}

std::unique_ptr<DragSource> X11DragAndDrop::createSource( NativeWindowContext context,
														  const DragOffer& offer,
														  std::string payload,
														  DragCompletionCallback completion,
														  DragStartResult& result ) {
	auto source =
		std::make_unique<X11DragSource>( context, std::move( payload ), std::move( completion ) );
	result = source->start( offer );
	if ( result.status != DragStartStatus::Started )
		return nullptr;
	return source;
}

}}} // namespace EE::Window::Private

#endif
