#include "waylanddraganddrop.hpp"

#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
#include "dragaccess.hpp"
#include "dragmanager.hpp"
#include "waylanddragpreview.hpp"
#include "waylandlibrary.hpp"
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

namespace EE { namespace Window { namespace Private {
namespace {

constexpr uint32_t leftButton = 0x110; // Linux input button codes are part of the Wayland protocol.
constexpr const char* fileMime = "text/uri-list";

struct WaylandSeat {
	WaylandLibrary& library;
	wl_seat* seat{};
	wl_pointer* pointer{};
	wl_surface* focus{};
	wl_surface* origin{};
	uint32_t global{}, serial{}, buttons{};

	explicit WaylandSeat( WaylandLibrary& library ) : library( library ) {}

	~WaylandSeat() {
		clearPointer();
		if ( seat && library.version( seat ) >= WL_SEAT_RELEASE_SINCE_VERSION )
			library.destroy( seat, WL_SEAT_RELEASE );
		else
			library.releaseProxy( seat );
	}

	void clearPointer() {
		if ( pointer && library.version( pointer ) >= WL_POINTER_RELEASE_SINCE_VERSION )
			library.destroy( pointer, WL_POINTER_RELEASE );
		else
			library.releaseProxy( pointer );
		focus = origin = nullptr;
		serial = buttons = 0;
	}

	static void capabilities( void* data, wl_seat*, uint32_t capabilities ) {
		auto& self = *static_cast<WaylandSeat*>( data );
		if ( !( capabilities & WL_SEAT_CAPABILITY_POINTER ) ) {
			self.clearPointer();
			return;
		}
		if ( self.pointer )
			return;
		self.pointer =
			self.library.create<wl_pointer>( self.seat, WL_SEAT_GET_POINTER, self.library.pointer );
		static const wl_pointer_listener listener = [] {
			wl_pointer_listener result{};
			result.enter = []( void* data, wl_pointer*, uint32_t, wl_surface* surface, wl_fixed_t,
							   wl_fixed_t ) { static_cast<WaylandSeat*>( data )->focus = surface; };
			result.leave = []( void* data, wl_pointer*, uint32_t, wl_surface* ) {
				auto& self = *static_cast<WaylandSeat*>( data );
				self.focus = self.origin = nullptr;
				self.serial = self.buttons = 0;
			};
			result.motion = []( void*, wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t ) {};
			result.button = []( void* data, wl_pointer*, uint32_t serial, uint32_t, uint32_t button,
								uint32_t state ) {
				auto& self = *static_cast<WaylandSeat*>( data );
				if ( state == WL_POINTER_BUTTON_STATE_PRESSED ) {
					if ( button == leftButton && self.buttons == 0 ) {
						self.serial = serial;
						self.origin = self.focus;
					}
					++self.buttons;
				} else {
					if ( self.buttons )
						--self.buttons;
					if ( button == leftButton ) {
						self.serial = 0;
						self.origin = nullptr;
					}
				}
			};
			result.axis = []( void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t ) {};
			result.frame = []( void*, wl_pointer* ) {};
			result.axis_source = []( void*, wl_pointer*, uint32_t ) {};
			result.axis_stop = []( void*, wl_pointer*, uint32_t, uint32_t ) {};
			result.axis_discrete = []( void*, wl_pointer*, uint32_t, int32_t ) {};
			return result;
		}();
		if ( self.pointer )
			self.library.listen( self.pointer, listener, &self );
	}
};

} // namespace

/** One observer on SDL's connection. Additional pointer resources receive the same real press
 * serial without replacing SDL's listeners or borrowing private SDL structures. */
struct WaylandConnection {
	struct PendingDevice {
		wl_data_device* device{};
		wl_callback* callback{};
	};

	WaylandLibrary library;
	SmallVector<std::unique_ptr<WaylandSeat>, 1> seats;
	SmallVector<std::pair<const Window*, wl_surface*>, 2> windows;
	SmallVector<PendingDevice, 1> pendingDevices;
	SmallVector<wl_data_offer*, 2> offers;
	wl_display* display{};
	wl_registry* registry{};
	wl_compositor* compositor{};
	wl_subcompositor* subcompositor{};
	wl_shm* shm{};
	wl_data_device_manager* manager{};
	uint32_t compositorGlobal{}, subcompositorGlobal{}, shmGlobal{}, managerGlobal{};
	bool dragging{};

	void reset() {
		endDrag();
		for ( auto& pending : pendingDevices ) {
			library.releaseProxy( pending.callback );
			library.releaseProxy( pending.device ); // Its server resource was already released.
		}
		pendingDevices.clear();
		windows.clear();
		seats.clear();
		library.releaseProxy( manager );
		library.releaseProxy( shm );
		library.releaseProxy( compositor );
		library.destroy( subcompositor, WL_SUBCOMPOSITOR_DESTROY );
		library.releaseProxy( registry );
		display = nullptr;
		compositorGlobal = subcompositorGlobal = shmGlobal = managerGlobal = 0;
	}

	static void global( void* data, wl_registry* registry, uint32_t name, const char* interface,
						uint32_t version ) {
		auto& self = *static_cast<WaylandConnection*>( data );
		auto& lib = self.library;
		if ( std::strcmp( interface, "wl_compositor" ) == 0 && version >= 3 ) {
			self.compositor =
				lib.bind<wl_compositor>( registry, name, lib.compositor, std::min( version, 4u ) );
			self.compositorGlobal = name;
		} else if ( std::strcmp( interface, "wl_subcompositor" ) == 0 ) {
			self.subcompositor = lib.bind<wl_subcompositor>( registry, name, lib.subcompositor, 1 );
			self.subcompositorGlobal = name;
		} else if ( std::strcmp( interface, "wl_shm" ) == 0 ) {
			self.shm = lib.bind<wl_shm>( registry, name, lib.shm, 1 );
			self.shmGlobal = name;
			static constexpr wl_shm_listener listener{ []( void*, wl_shm*, uint32_t ) {} };
			if ( self.shm )
				lib.listen( self.shm, listener, nullptr );
		} else if ( std::strcmp( interface, "wl_data_device_manager" ) == 0 && version >= 3 ) {
			self.manager =
				lib.bind<wl_data_device_manager>( registry, name, lib.data_device_manager, 3 );
			self.managerGlobal = name;
		} else if ( std::strcmp( interface, "wl_seat" ) == 0 ) {
			// A seat's callback address must remain stable when another seat is hot-plugged.
			auto seat = std::make_unique<WaylandSeat>( lib );
			seat->global = name;
			seat->seat = lib.bind<wl_seat>( registry, name, lib.seat, std::min( version, 5u ) );
			static constexpr wl_seat_listener listener{ WaylandSeat::capabilities,
														[]( void*, wl_seat*, const char* ) {} };
			if ( seat->seat ) {
				lib.listen( seat->seat, listener, seat.get() );
				self.seats.push_back( std::move( seat ) );
			}
		}
	}

	static void removed( void* data, wl_registry*, uint32_t name ) {
		auto& self = *static_cast<WaylandConnection*>( data );
		for ( auto it = self.seats.begin(); it != self.seats.end(); ++it ) {
			if ( ( *it )->global == name ) {
				self.seats.erase( it );
				break;
			}
		}
		if ( name == self.managerGlobal )
			self.library.releaseProxy( self.manager );
		if ( name == self.compositorGlobal )
			self.library.releaseProxy( self.compositor );
		if ( name == self.subcompositorGlobal )
			self.library.destroy( self.subcompositor, WL_SUBCOMPOSITOR_DESTROY );
		if ( name == self.shmGlobal )
			self.library.releaseProxy( self.shm );
	}

	bool initialize( wl_display* connection ) {
		if ( !library.load() )
			return false;
		display = connection;
		registry =
			library.create<wl_registry>( display, WL_DISPLAY_GET_REGISTRY, library.registry );
		if ( !registry )
			return false;
		static constexpr wl_registry_listener listener{ global, removed };
		library.listen( registry, listener, this );
		// Setup only, never per drag/motion. The second barrier delivers seat capabilities so its
		// pointer listener is in place before the application handles its first gesture.
		if ( library.wl_display_roundtrip( display ) < 0 ||
			 library.wl_display_roundtrip( display ) < 0 )
			return false;
		library.wl_display_flush( display );
		return true;
	}

	bool startDrag( wl_data_source* source, wl_surface* origin, wl_surface* icon,
					const WaylandSeat& seat ) {
		auto* device = library.create<wl_data_device>(
			manager, WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE, library.data_device, seat.seat );
		if ( !device )
			return false;
		// This short-lived device must never replace SDL's receiver. Its events stay on the
		// connection's UI queue: even a late setup reply cannot access a destroyed session.
		static constexpr wl_data_device_listener deviceListener{
			[]( void* data, wl_data_device*, wl_data_offer* offer ) {
				auto& self = *static_cast<WaylandConnection*>( data );
				static constexpr wl_data_offer_listener listener{
					[]( void*, wl_data_offer*, const char* ) {},
					[]( void*, wl_data_offer*, uint32_t ) {},
					[]( void*, wl_data_offer*, uint32_t ) {} };
				self.library.listen( offer, listener, nullptr );
				if ( self.dragging )
					self.offers.push_back( offer );
				else
					self.library.destroy( offer, WL_DATA_OFFER_DESTROY );
			},
			[]( void*, wl_data_device*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t,
				wl_data_offer* ) {},
			[]( void*, wl_data_device* ) {},
			[]( void*, wl_data_device*, uint32_t, wl_fixed_t, wl_fixed_t ) {},
			[]( void*, wl_data_device* ) {},
			[]( void*, wl_data_device*, wl_data_offer* ) {} };
		library.listen( device, deviceListener, this );
		dragging = true;
		library.request( device, WL_DATA_DEVICE_START_DRAG, source, origin, icon, seat.serial );
		library.request( device, WL_DATA_DEVICE_RELEASE );
		auto* callback = library.create<wl_callback>( display, WL_DISPLAY_SYNC, library.callback );
		pendingDevices.push_back( { device, callback } );
		static constexpr wl_callback_listener releaseListener{
			[]( void* data, wl_callback* callback, uint32_t ) {
				auto& self = *static_cast<WaylandConnection*>( data );
				for ( auto it = self.pendingDevices.begin(); it != self.pendingDevices.end();
					  ++it ) {
					if ( it->callback == callback ) {
						self.library.releaseProxy( it->device );
						self.library.releaseProxy( it->callback );
						self.pendingDevices.erase( it );
						break;
					}
				}
			} };
		if ( callback )
			library.listen( callback, releaseListener, this );
		return callback != nullptr;
	}

	void endDrag() {
		dragging = false;
		// Destroy the source first: discarding its current offer while it is live may cancel it.
		// Offers arriving after cancellation are disposed of by the connection listener above.
		for ( auto* offer : offers )
			library.destroy( offer, WL_DATA_OFFER_DESTROY );
		offers.clear();
	}

	bool supports( const NativeWindowContext& context ) const {
		return context.kind == NativeWindowContext::Kind::Wayland &&
			   display == context.handles.wayland.display && compositor && subcompositor && shm &&
			   manager && std::any_of( seats.begin(), seats.end(), []( const auto& seat ) {
				   return seat->global && seat->pointer;
			   } );
	}
};

WaylandConnection WaylandDragAndDrop::sConnection;
bool WaylandDragAndDrop::sExitHandlerRegistered = false;

namespace {

class WaylandDragSource final : public DragSource {
  public:
	WaylandDragSource( WaylandConnection& connection, NativeWindowContext context,
					   std::string payload, DragCompletionCallback completion ) :
		DragSource( context.owner, std::move( completion ) ),
		connection( connection ),
		payload( std::move( payload ) ),
		preview( connection.library ),
		origin( static_cast<wl_surface*>( context.handles.wayland.surface ) ) {}

	~WaylandDragSource() override { reset(); }

	DragStartResult begin( const DragOffer& offer, const WaylandSeat& seat ) {
		seatGlobal = seat.global;
		auto& lib = connection.library;
		queue = lib.wl_display_create_queue( connection.display );
		if ( !queue || pipe( wake ) != 0 )
			return { "Could not create the Wayland transfer queue", DragStartStatus::Failed };
		for ( const auto fd : wake ) {
			if ( fcntl( fd, F_SETFD, FD_CLOEXEC ) < 0 || fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 )
				return { "Could not configure the transfer wake pipe", DragStartStatus::Failed };
		}
		// Assign the source queue through a wrapper before constructing the source. The manager
		// remains on the UI queue for its short-lived initiating devices.
		auto* managerProxy =
			static_cast<wl_proxy*>( lib.wl_proxy_create_wrapper( connection.manager ) );
		if ( !managerProxy )
			return { "Could not create the Wayland data source", DragStartStatus::Failed };
		lib.wl_proxy_set_queue( managerProxy, queue );
		source = lib.create<wl_data_source>(
			managerProxy, WL_DATA_DEVICE_MANAGER_CREATE_DATA_SOURCE, lib.data_source );
		lib.wl_proxy_wrapper_destroy( managerProxy );
		if ( !source )
			return { "Could not create the Wayland data source", DragStartStatus::Failed };
		static constexpr wl_data_source_listener sourceListener{
			[]( void* data, wl_data_source*, const char* mime ) {
				static_cast<WaylandDragSource*>( data )->accepted =
					mime && std::strcmp( mime, fileMime ) == 0;
			},
			send,
			[]( void* data, wl_data_source* ) {
				static_cast<WaylandDragSource*>( data )->cancelled = true;
			},
			[]( void* data, wl_data_source* ) {
				static_cast<WaylandDragSource*>( data )->dropped = true;
			},
			[]( void* data, wl_data_source* ) {
				static_cast<WaylandDragSource*>( data )->finished = true;
			},
			[]( void* data, wl_data_source*, uint32_t action ) {
				auto& self = *static_cast<WaylandDragSource*>( data );
				self.action = action;
				// A new destination may negotiate actions before accepting a MIME type. Do not
				// carry the previous target's acceptance across the intervening None action.
				if ( action == WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE )
					self.accepted = false;
			} };
		lib.listen( source, sourceListener, this );
		if ( !preview.initialize( connection.compositor, connection.subcompositor, connection.shm,
								  offer, owner->getScale() ) )
			return { "Could not create the Wayland drag image", DragStartStatus::Failed };
		lib.request( source, WL_DATA_SOURCE_OFFER, fileMime );
		lib.request( source, WL_DATA_SOURCE_SET_ACTIONS, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY );
		started = true;
		// SDL can synthesize button-up as soon as the compositor takes pointer focus. Arm the
		// suppression before that event, including cancellation before the first event pump.
		NativeDragAccess::suppressNativeDragRelease( *owner->getInput() );
		if ( !connection.startDrag( source, origin, preview.surface(), seat ) )
			return { "Could not start the Wayland drag", DragStartStatus::Failed };
		lib.request( preview.surface(), WL_SURFACE_COMMIT );
		// Session events use their own queue and transfer thread. SDL synchronously reads incoming
		// drops: serving those on the UI queue would deadlock drops into another eepp window.
		// Thread::launch() cannot report failure; reject the start if its transfer worker cannot
		// run.
		const int threadError = pthread_create( &worker, nullptr, run, this );
		if ( threadError )
			return { "Could not create the Wayland transfer thread", DragStartStatus::Failed };
		workerStarted = true;
		lib.wl_display_flush( connection.display );
		return { {}, DragStartStatus::Started };
	}

	void tick() override {
		if ( completed )
			return;
		preview.setFeedback( accepted ? ( action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY
											  ? DragFeedback::Copy
											  : DragFeedback::Pending )
									  : DragFeedback::Rejected );
		if ( failed || connection.library.wl_display_get_error( connection.display ) ) {
			reset();
			finish( DragEndReason::Failed, DragAction::None, "Wayland data transfer failed" );
		} else if ( cancelled || !std::any_of( connection.seats.begin(), connection.seats.end(),
											   [this]( const auto& seat ) {
												   return seat->global == seatGlobal &&
														  seat->pointer;
											   } ) ) {
			cancel();
		} else if ( finished ) {
			const bool copied = dropped && action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
			reset();
			finish( copied ? DragEndReason::Dropped : DragEndReason::Cancelled,
					copied ? DragAction::Copy : DragAction::None );
		}
	}

	bool handleEvent( void* ) override { return false; }

	void handleInput( NativeDragInput input ) override {
		if ( input == NativeDragInput::Escape )
			cancel();
	}

	void cancel() override {
		reset();
		finish( DragEndReason::Cancelled );
	}

  private:
	struct Transfer {
		size_t offset{};
		int fd{ -1 };
	};

	WaylandConnection& connection;
	std::string payload;
	WaylandDragPreview preview;
	wl_surface* origin{};
	wl_data_source* source{};
	wl_event_queue* queue{};
	pthread_t worker{};
	// Bounded concurrent receivers share the same immutable payload; no per-send copies.
	std::array<Transfer, 16> transfers;
	int wake[2]{ -1, -1 };
	uint32_t seatGlobal{};
	std::atomic<uint32_t> action{ WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE };
	std::atomic<bool> stopping{ false }, failed{ false }, cancelled{ false }, dropped{ false },
		finished{ false }, accepted{ false };
	bool workerStarted{ false }, started{ false };

	static void send( void* data, wl_data_source*, const char* mime, int32_t fd ) {
		auto& self = *static_cast<WaylandDragSource*>( data );
		if ( !mime || std::strcmp( mime, fileMime ) != 0 || fcntl( fd, F_SETFD, FD_CLOEXEC ) < 0 ||
			 fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 ) {
			close( fd );
			return;
		}
		for ( auto& transfer : self.transfers ) {
			if ( transfer.fd < 0 ) {
				transfer = { 0, fd };
				return;
			}
		}
		close( fd );
		self.failed = true;
	}

	static void* run( void* data ) {
		static_cast<WaylandDragSource*>( data )->transferLoop();
		return nullptr;
	}

	void transferLoop() {
		// Only this short-lived worker blocks SIGPIPE, never the UI thread or the process. A target
		// may close its pipe at any point; an EPIPE must not terminate the application.
		sigset_t pipeSignal;
		sigemptyset( &pipeSignal );
		sigaddset( &pipeSignal, SIGPIPE );
		pthread_sigmask( SIG_BLOCK, &pipeSignal, nullptr );
		auto& lib = connection.library;
		auto* display = connection.display;
		while ( !stopping && !failed ) {
			if ( lib.wl_display_dispatch_queue_pending( display, queue ) < 0 ) {
				failed = true;
				break;
			}
			if ( lib.wl_display_prepare_read_queue( display, queue ) != 0 )
				continue;
			std::array<pollfd, 18> descriptors{};
			descriptors[0] = { lib.wl_display_get_fd( display ), POLLIN, 0 };
			descriptors[1] = { wake[0], POLLIN, 0 };
			if ( lib.wl_display_flush( display ) < 0 ) {
				if ( errno == EAGAIN )
					descriptors[0].events |= POLLOUT;
				else {
					lib.wl_display_cancel_read( display );
					failed = true;
					break;
				}
			}
			for ( size_t i = 0; i < transfers.size(); ++i )
				descriptors[i + 2] = { transfers[i].fd, POLLOUT, 0 };
			const int ready = poll( descriptors.data(), descriptors.size(), 100 );
			if ( ready > 0 && ( descriptors[0].revents & POLLIN ) ) {
				if ( lib.wl_display_read_events( display ) < 0 )
					failed = true;
			} else {
				lib.wl_display_cancel_read( display );
			}
			if ( ( ready < 0 && errno != EINTR ) ||
				 ( descriptors[0].revents & ( POLLERR | POLLHUP | POLLNVAL ) ) ) {
				failed = true;
				break;
			}
			for ( size_t i = 0; i < transfers.size(); ++i ) {
				auto& transfer = transfers[i];
				if ( transfer.fd < 0 || !descriptors[i + 2].revents )
					continue;
				const auto bytes =
					write( transfer.fd, payload.data() + transfer.offset,
						   std::min<size_t>( payload.size() - transfer.offset, 65536 ) );
				if ( bytes > 0 )
					transfer.offset += bytes;
				if ( transfer.offset == payload.size() ||
					 ( bytes < 0 && errno != EAGAIN && errno != EINTR ) ) {
					close( transfer.fd );
					transfer.fd = -1;
				}
			}
		}
	}

	void reset() {
		auto& lib = connection.library;
		if ( workerStarted ) {
			stopping = true;
			const char byte = 0;
			const auto ignored = write( wake[1], &byte, 1 );
			(void)ignored;
			pthread_join( worker, nullptr );
			workerStarted = false;
		}
		for ( auto& transfer : transfers ) {
			if ( transfer.fd >= 0 )
				close( transfer.fd );
			transfer.fd = -1;
		}
		lib.destroy( source, WL_DATA_SOURCE_DESTROY );
		connection.endDrag();
		preview.reset();
		if ( queue ) {
			lib.wl_event_queue_destroy( queue );
			queue = nullptr;
		}
		for ( auto& fd : wake ) {
			if ( fd >= 0 )
				close( fd );
			fd = -1;
		}
		if ( started && owner )
			owner->getInput()->resetButton( EE_BUTTON_LEFT );
		started = false;
		if ( connection.display )
			lib.wl_display_flush( connection.display );
	}
};

} // namespace

void WaylandDragAndDrop::attachWindow( const NativeWindowContext& context ) {
	auto& connection = sConnection;
	if ( context.kind != NativeWindowContext::Kind::Wayland )
		return;
	if ( !sExitHandlerRegistered ) {
		// A caller may return from main or call std::exit without destroying Engine. Register
		// after static initialization so sources and observers are released before their loader,
		// independently of translation-unit destruction order. Normal backend teardown stays eager.
		if ( std::atexit( shutdown ) != 0 )
			return;
		sExitHandlerRegistered = true;
	}
	auto* display = static_cast<wl_display*>( context.handles.wayland.display );
	if ( connection.display && connection.display != display )
		return;
	connection.windows.emplace_back( context.owner,
									 static_cast<wl_surface*>( context.handles.wayland.surface ) );
	if ( !connection.display && !connection.initialize( display ) )
		connection.reset();
}

void WaylandDragAndDrop::detachWindow( const Window& window ) {
	auto& connection = sConnection;
	auto& windows = connection.windows;
	for ( auto it = windows.begin(); it != windows.end(); ++it ) {
		if ( it->first == &window ) {
			for ( auto& seat : connection.seats ) {
				if ( seat->focus == it->second )
					seat->focus = nullptr;
				if ( seat->origin == it->second ) {
					seat->origin = nullptr;
					seat->serial = seat->buttons = 0;
				}
			}
			windows.erase( it );
			break;
		}
	}
}

void WaylandDragAndDrop::shutdown() {
	// wl_display.get_registry's server resources live until disconnect. Retain one observer
	// across zero-window periods instead of adding a new registry for each reopening.
	if ( sConnection.display ) {
		DragManager::shutdown();
		sConnection.reset();
	}
}

bool WaylandDragAndDrop::isSupported( const NativeWindowContext& context ) {
	return sConnection.supports( context );
}

std::unique_ptr<DragSource> WaylandDragAndDrop::createSource( NativeWindowContext context,
															  const DragOffer& offer,
															  std::string payload,
															  DragCompletionCallback completion,
															  DragStartResult& result ) {
	const auto seat = std::find_if(
		sConnection.seats.begin(), sConnection.seats.end(), [&context]( const auto& candidate ) {
			return candidate->buttons == 1 && candidate->origin == context.handles.wayland.surface;
		} );
	if ( seat == sConnection.seats.end() || !context.owner->getInput()->isMouseLeftPressed() ) {
		result = { "A real Wayland left-button press on the source window is required",
				   DragStartStatus::InvalidGesture };
		return nullptr;
	}
	auto source = std::make_unique<WaylandDragSource>( sConnection, context, std::move( payload ),
													   std::move( completion ) );
	result = source->begin( offer, **seat );
	if ( result.status != DragStartStatus::Started )
		return nullptr;
	return source;
}

}}} // namespace EE::Window::Private
#endif
