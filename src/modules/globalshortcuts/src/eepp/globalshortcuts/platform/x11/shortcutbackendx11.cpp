#include <eepp/config.hpp>

#ifdef EE_GLOBALSHORTCUTS_X11

#include "shortcutbackendx11.hpp"
#include "shortcutkeymapx11.hpp"
#include <algorithm>
#include <atomic>
#include <bitset>
#include <cerrno>
#include <cstdlib>
#include <eepp/core/small_vector.hpp>
#include <fcntl.h>
#include <iterator>
#include <map>
#include <poll.h>
#include <string_view>
#include <unistd.h>

namespace EE::System::GlobalShortcuts::Private {

namespace {

constexpr unsigned EventBatchSize = 32;

template <typename T> using XcbReply = std::unique_ptr<T, decltype( &std::free )>;

Error unavailable( const char* message ) {
	return { ErrorCode::BackendUnavailable, message };
}

Error disconnected( xcb_connection_t* connection ) {
	return { ErrorCode::Disconnected, "X11 shortcut connection lost",
			 xcb_connection_has_error( connection ) };
}

struct Grab {
	xcb_window_t root;
	X11ShortcutKey key;

	auto operator<=>( const Grab& ) const = default;
};

bool overlaps( const std::vector<Grab>& a, const std::vector<Grab>& b ) {
	return std::any_of( a.begin(), a.end(), [&]( const Grab& grab ) {
		return std::binary_search( b.begin(), b.end(), grab );
	} );
}

class X11ShortcutBackend final : public ShortcutBackend {
  public:
	explicit X11ShortcutBackend( std::string display ) : mDisplay( std::move( display ) ) {}

	~X11ShortcutBackend() override;

	Backend type() const override { return Backend::X11; }

	Result<Capabilities, Error> open() override;

	void reconcile( Client& client, Uint64 request, std::vector<Shortcut> shortcuts,
					bool retry ) override;

	// The worker completes checked native requests before processing the next command.
	void cancel( Client&, Uint64 ) override {}

	void wait( Client& client ) override;

	void wake() noexcept override;

	Result<void, Error> close() override;

  private:
	struct Registered {
		std::vector<Grab> grabs;
		std::optional<Trigger> trigger;
		std::optional<Error> error;
		Uint64 incarnation{ 0 };
		Uint32 firstSequence{ 0 };
	};

	struct Change {
		const std::string* id;
		Registered* registered;
		std::vector<Grab> wanted;
		Trigger trigger;
		bool pending{ false };
	};

	struct Retired {
		std::string id;
		Error error;
		Grab grab;
	};

	using Component = SmallVector<Change*, 4>;

	Result<void, Error> openWakePipe();

	void drainWake();

	void dropConnection();

	void connectionLost( Client& client, Error error );

	void handle( Client& client, const xcb_generic_event_t& event );

	void keyEvent( Client& client, const xcb_generic_event_t& event );

	bool stillDesired( const std::string& id ) const;

	Result<std::vector<Grab>, Error> resolve( const Trigger& trigger ) const;

	std::optional<Error> changeGrab( const Grab& grab, bool acquire );

	void retire( const std::string& id, const Grab& grab );

	void discardOldBindings( bool remap );

	std::optional<Error> apply( bool remap );

	Component takeComponent( std::vector<Change>& changes, Change& first ) const;

	std::optional<Error> applyComponent( const Component& component );

	std::vector<Binding> report() const;

	std::string mDisplay;
	std::unique_ptr<xcb_connection_t, decltype( &xcb_disconnect )> mConnection{ nullptr,
																				&xcb_disconnect };
	X11ShortcutKeymap mKeymap;
	std::vector<xcb_window_t> mRoots;
	std::vector<Shortcut> mDesired;
	std::map<std::string, Registered> mRegistered;
	std::vector<Retired> mRetired;
	std::bitset<256> mPressed;
	Uint64 mNextIncarnation{ 0 };
	std::atomic<int> mWakeWrite{ -1 };
	std::atomic<bool> mPendingWake{ false };
	int mWakeRead{ -1 };
	bool mWakeBroken{ false };
};

X11ShortcutBackend::~X11ShortcutBackend() {
	// The worker and its cancellation callbacks have stopped before the pipe is destroyed.
	if ( mWakeRead >= 0 ) {
		::close( mWakeRead );
		::close( mWakeWrite.load() );
	}
}

Result<void, Error> X11ShortcutBackend::openWakePipe() {
	if ( mWakeRead >= 0 )
		return Ok();
	int ends[2];
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( pipe2( ends, O_CLOEXEC | O_NONBLOCK ) < 0 )
#else
	if ( pipe( ends ) < 0 )
#endif
		return Err( Error{ ErrorCode::BackendUnavailable, "Could not open X11 shortcut wake pipe",
						   errno } );
#if EE_PLATFORM != EE_PLATFORM_LINUX
	for ( int fd : ends ) {
		if ( fcntl( fd, F_SETFD, FD_CLOEXEC ) < 0 || fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 ) {
			const int error = errno;
			::close( ends[0] );
			::close( ends[1] );
			return Err( Error{ ErrorCode::BackendUnavailable,
							   "Could not configure X11 shortcut wake pipe", error } );
		}
	}
#endif
	mWakeRead = ends[0];
	mWakeWrite.store( ends[1] );
	return Ok();
}

Result<Capabilities, Error> X11ShortcutBackend::open() {
	if ( mWakeBroken )
		return Err( unavailable( "The X11 shortcut worker wake pipe failed" ) );
	auto wakePipe = openWakePipe();
	if ( !wakePipe )
		return Err( std::move( wakePipe ).error() );

	mConnection.reset( xcb_connect( mDisplay.c_str(), nullptr ) );
	const int error = xcb_connection_has_error( mConnection.get() );
	if ( error ) {
		dropConnection();
		return Err(
			Error{ ErrorCode::BackendUnavailable, "Could not connect to the X11 display", error } );
	}
	constexpr std::string_view xwayland = "XWAYLAND";
	auto extension = xcb_query_extension( mConnection.get(), xwayland.size(), xwayland.data() );
	XcbReply<xcb_query_extension_reply_t> extensionReply(
		xcb_query_extension_reply( mConnection.get(), extension, nullptr ), &std::free );
	if ( !extensionReply ) {
		auto failure = disconnected( mConnection.get() );
		dropConnection();
		return Err( std::move( failure ) );
	}
	if ( extensionReply->present ) {
		dropConnection();
		return Err( Error{ ErrorCode::Unsupported,
						   "XWayland requires a Wayland global shortcuts backend" } );
	}
	for ( auto screen = xcb_setup_roots_iterator( xcb_get_setup( mConnection.get() ) ); screen.rem;
		  xcb_screen_next( &screen ) )
		mRoots.push_back( screen.data->root );
	auto keymap = mKeymap.open( mConnection.get() );
	if ( !keymap || mRoots.empty() ) {
		dropConnection();
		return Err( keymap ? unavailable( "The X11 display exposes no root windows" )
						   : std::move( keymap ).error() );
	}
	return Ok( Capabilities{ true, false, true } );
}

void X11ShortcutBackend::wake() noexcept {
	mPendingWake.store( true );
	mPendingWake.notify_one();
	const int fd = mWakeWrite.load();
	if ( fd < 0 )
		return;
	const char byte = 1;
	while ( write( fd, &byte, 1 ) < 0 && errno == EINTR ) {
	}
}

void X11ShortcutBackend::drainWake() {
	if ( mWakeRead < 0 )
		return;
	char bytes[128];
	ssize_t size;
	do {
		size = read( mWakeRead, bytes, sizeof( bytes ) );
	} while ( size > 0 || ( size < 0 && errno == EINTR ) );
}

void X11ShortcutBackend::wait( Client& client ) {
	if ( mPendingWake.exchange( false ) ) {
		drainWake();
		return;
	}
	if ( mWakeRead < 0 || mWakeBroken ) {
		mPendingWake.wait( false );
		return;
	}

	for ( unsigned count = 0; mConnection && count < EventBatchSize; ++count ) {
		XcbReply<xcb_generic_event_t> event( xcb_poll_for_event( mConnection.get() ), &std::free );
		if ( !event )
			break;
		handle( client, *event );
		if ( count + 1 == EventBatchSize || mPendingWake.load() )
			return;
	}
	if ( mConnection && xcb_connection_has_error( mConnection.get() ) ) {
		connectionLost( client, disconnected( mConnection.get() ) );
		return;
	}
	pollfd descriptors[] = {
		{ mWakeRead, POLLIN, 0 },
		{ mConnection ? xcb_get_file_descriptor( mConnection.get() ) : -1, POLLIN, 0 } };
	const auto result = poll( descriptors, 2, -1 );
	if ( result < 0 && errno == EINTR )
		return;
	constexpr short errors = POLLERR | POLLHUP | POLLNVAL;
	if ( result < 0 || ( descriptors[0].revents & errors ) ) {
		mWakeBroken = true;
		connectionLost( client, { ErrorCode::BackendUnavailable, "X11 shortcut worker poll failed",
								  result < 0 ? std::optional<int>{ errno } : std::nullopt } );
	} else if ( mConnection && ( descriptors[1].revents & errors ) ) {
		connectionLost( client, disconnected( mConnection.get() ) );
	}
	if ( !mWakeBroken && ( descriptors[0].revents & POLLIN ) )
		drainWake();
}

void X11ShortcutBackend::dropConnection() {
	mKeymap.close();
	mConnection.reset();
	mRoots.clear();
	mRegistered.clear();
	mRetired.clear();
	mPressed.reset();
}

void X11ShortcutBackend::connectionLost( Client& client, Error error ) {
	dropConnection();
	client.bindingsChanged( {}, std::move( error ) );
}

Result<void, Error> X11ShortcutBackend::close() {
	std::optional<Error> failure;
	for ( const auto& [id, binding] : mRegistered ) {
		for ( const auto& grab : binding.grabs ) {
			if ( auto error = changeGrab( grab, false ); error && !failure )
				failure = std::move( error );
		}
	}
	for ( const auto& retired : mRetired ) {
		if ( auto error = changeGrab( retired.grab, false ); error && !failure )
			failure = std::move( error );
	}
	dropConnection();
	return failure ? Result<void, Error>( Err( std::move( *failure ) ) )
				   : Result<void, Error>( Ok() );
}

std::optional<Error> X11ShortcutBackend::changeGrab( const Grab& grab, bool acquire ) {
	auto* connection = mConnection.get();
	const auto request =
		acquire
			? xcb_grab_key_checked( connection, 0, grab.root, grab.key.modifiers, grab.key.code,
									XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC )
			: xcb_ungrab_key_checked( connection, grab.key.code, grab.root, grab.key.modifiers );
	XcbReply<xcb_generic_error_t> error( xcb_request_check( connection, request ), &std::free );
	if ( xcb_connection_has_error( connection ) )
		return disconnected( connection );
	if ( !error )
		return {};
	if ( acquire && error->error_code == XCB_ACCESS ) {
		return Error{ ErrorCode::Conflict, "Another X11 client holds this hotkey",
					  error->error_code, "BadAccess" };
	}
	return Error{ ErrorCode::NativeFailure,
				  acquire ? "X11 key grab failed" : "X11 key ungrab failed", error->error_code };
}

void X11ShortcutBackend::retire( const std::string& id, const Grab& grab ) {
	if ( auto error = changeGrab( grab, false ) )
		mRetired.push_back( { id, std::move( *error ), grab } );
}

bool X11ShortcutBackend::stillDesired( const std::string& id ) const {
	const auto it = std::lower_bound(
		mDesired.begin(), mDesired.end(), id,
		[]( const Shortcut& shortcut, const std::string& key ) { return shortcut.id < key; } );
	return it != mDesired.end() && it->id == id;
}

Result<std::vector<Grab>, Error> X11ShortcutBackend::resolve( const Trigger& trigger ) const {
	auto keys = mKeymap.resolve( trigger );
	if ( !keys )
		return Err( std::move( keys ).error() );
	std::vector<Grab> grabs;
	grabs.reserve( mRoots.size() * keys->size() );
	for ( auto root : mRoots ) {
		for ( auto key : keys.value() )
			grabs.push_back( { root, key } );
	}
	std::sort( grabs.begin(), grabs.end() );
	return Ok( std::move( grabs ) );
}

void X11ShortcutBackend::discardOldBindings( bool remap ) {
	for ( size_t i = mRetired.size(); i > 0; --i ) {
		auto& retired = mRetired[i - 1];
		if ( auto error = changeGrab( retired.grab, false ) )
			retired.error = std::move( *error );
		else
			mRetired.erase( mRetired.begin() + ( i - 1 ) );
	}
	for ( auto it = mRegistered.begin(); it != mRegistered.end(); ) {
		const bool keep = stillDesired( it->first );
		auto& binding = it->second;
		bool valid = keep;
		if ( valid && remap && binding.trigger ) {
			auto current = resolve( *binding.trigger );
			valid = current && current.value() == binding.grabs;
		}
		// An old native grab is usable after a failed replacement only while it still
		// denotes the old logical shortcut in the updated map.
		if ( !valid ) {
			for ( const auto& grab : binding.grabs )
				retire( it->first, grab );
			binding = {};
		}
		if ( keep )
			++it;
		else
			it = mRegistered.erase( it );
	}
}

std::optional<Error> X11ShortcutBackend::apply( bool remap ) {
	discardOldBindings( remap );
	std::vector<Change> changes;
	changes.reserve( mDesired.size() );
	for ( const auto& shortcut : mDesired ) {
		auto it = mRegistered.try_emplace( shortcut.id ).first;
		it->second.error.reset();
		auto wanted = resolve( *shortcut.preferred );
		if ( wanted ) {
			const bool changed =
				it->second.trigger != shortcut.preferred || it->second.grabs != wanted.value();
			changes.push_back( { &it->first, &it->second, std::move( wanted ).value(),
								 *shortcut.preferred, changed } );
		} else {
			it->second.error = std::move( wanted ).error();
		}
	}
	for ( auto a = changes.begin(); a != changes.end(); ++a ) {
		for ( auto b = std::next( a ); b != changes.end(); ++b ) {
			if ( overlaps( a->wanted, b->wanted ) ) {
				const Error conflict{ ErrorCode::Conflict,
									  "Two shortcuts map to the same X11 key grab" };
				for ( auto* change : { &*a, &*b } ) {
					if ( change->registered->trigger != change->trigger ||
						 change->registered->grabs != change->wanted ) {
						change->registered->error = conflict;
						change->pending = false;
					}
				}
			}
		}
	}
	for ( auto& change : changes ) {
		if ( change.pending ) {
			auto component = takeComponent( changes, change );
			if ( auto error = applyComponent( component ) )
				return error;
		}
	}
	return xcb_connection_has_error( mConnection.get() )
			   ? std::optional<Error>( disconnected( mConnection.get() ) )
			   : std::nullopt;
}

X11ShortcutBackend::Component X11ShortcutBackend::takeComponent( std::vector<Change>& changes,
																 Change& first ) const {
	Component component;
	component.push_back( &first );
	first.pending = false;
	// Changes that want each other's existing pairs commit as one group. A swap can reuse
	// both server-owned grabs, and a failure keeps the whole group's previous coverage.
	for ( size_t i = 0; i < component.size(); ++i ) {
		const auto& current = *component[i];
		for ( auto& other : changes ) {
			if ( other.pending && ( overlaps( current.registered->grabs, other.wanted ) ||
									overlaps( other.registered->grabs, current.wanted ) ) ) {
				other.pending = false;
				component.push_back( &other );
			}
		}
	}
	return component;
}

std::optional<Error> X11ShortcutBackend::applyComponent( const Component& component ) {
	struct Staged {
		const std::string* id;
		Grab grab;
	};
	SmallVector<Staged, 8> staged;
	std::optional<Error> failure;
	for ( const auto* change : component ) {
		for ( const auto& grab : change->wanted ) {
			const auto owner =
				std::find_if( mRegistered.begin(), mRegistered.end(), [&]( const auto& entry ) {
					return std::binary_search( entry.second.grabs.begin(), entry.second.grabs.end(),
											   grab );
				} );
			if ( owner != mRegistered.end() ) {
				if ( std::any_of( component.begin(), component.end(), [&]( const Change* current ) {
						 return current->registered == &owner->second;
					 } ) )
					continue;
				failure =
					Error{ ErrorCode::Conflict, "Another shortcut still owns this X11 key grab" };
			} else if ( std::any_of(
							mRetired.begin(), mRetired.end(),
							[&]( const Retired& retired ) { return retired.grab == grab; } ) ) {
				failure = Error{ ErrorCode::NativeFailure,
								 "An earlier X11 key grab could not be released" };
			} else {
				failure = changeGrab( grab, true );
			}
			if ( failure )
				break;
			staged.push_back( { change->id, grab } );
		}
		if ( failure )
			break;
	}
	if ( failure ) {
		for ( const auto& added : staged )
			retire( *added.id, added.grab );
		for ( const auto* change : component )
			change->registered->error = *failure;
		return {};
	}

	for ( const auto* change : component ) {
		for ( const auto& grabbed : change->registered->grabs ) {
			if ( std::none_of( component.begin(), component.end(), [&]( const Change* current ) {
					 return std::binary_search( current->wanted.begin(), current->wanted.end(),
												grabbed );
				 } ) )
				retire( *change->id, grabbed );
		}
	}
	// The fence separates already queued events from the new pair ownership, including
	// pairs transferred within a group without another GrabKey request.
	const auto fence = xcb_get_input_focus( mConnection.get() );
	XcbReply<xcb_get_input_focus_reply_t> reply(
		xcb_get_input_focus_reply( mConnection.get(), fence, nullptr ), &std::free );
	if ( !reply )
		return disconnected( mConnection.get() );
	for ( auto* change : component ) {
		auto& binding = *change->registered;
		binding.grabs = std::move( change->wanted );
		binding.trigger = change->trigger;
		binding.firstSequence = fence.sequence;
		binding.incarnation = ++mNextIncarnation;
	}
	return {};
}

std::vector<ShortcutBackend::Binding> X11ShortcutBackend::report() const {
	std::vector<Binding> result;
	result.reserve( mRegistered.size() );
	for ( const auto& [id, current] : mRegistered ) {
		result.push_back( { { id,
							  {},
							  current.trigger,
							  current.error,
							  current.trigger ? BindingStatus::Bound : BindingStatus::Unbound },
							current.incarnation } );
	}
	for ( const auto& retired : mRetired ) {
		const auto it = std::find_if( result.begin(), result.end(), [&]( const Binding& binding ) {
			return binding.shortcut.id == retired.id;
		} );
		if ( it == result.end() ) {
			result.push_back(
				{ { retired.id, {}, {}, retired.error, BindingStatus::Unknown }, 0 } );
		} else {
			it->shortcut.error = retired.error;
			if ( it->shortcut.status != BindingStatus::Bound )
				it->shortcut.status = BindingStatus::Unknown;
		}
	}
	return result;
}

void X11ShortcutBackend::reconcile( Client& client, Uint64 request, std::vector<Shortcut> shortcuts,
									bool retry ) {
	const bool sameTriggers =
		std::equal( shortcuts.begin(), shortcuts.end(), mDesired.begin(), mDesired.end(),
					[]( const Shortcut& a, const Shortcut& b ) {
						return a.id == b.id && a.preferred == b.preferred;
					} );
	mDesired = std::move( shortcuts );
	if ( !mConnection ) {
		if ( mDesired.empty() ) {
			client.complete( request, Ok( std::vector<Binding>{} ) );
			return;
		}
		auto opened = open();
		if ( !opened ) {
			client.complete( request, Err( std::move( opened ).error() ) );
			return;
		}
		retry = true;
	}
	if ( retry || !sameTriggers || !mRetired.empty() ) {
		if ( auto error = apply( false ) ) {
			connectionLost( client, *error );
			client.complete( request, Err( std::move( *error ) ) );
			return;
		}
	}
	auto effective = report();
	if ( mDesired.empty() && mRetired.empty() )
		dropConnection();
	client.complete( request, Ok( std::move( effective ) ) );
}

void X11ShortcutBackend::handle( Client& client, const xcb_generic_event_t& event ) {
	if ( event.response_type == XCB_KEY_PRESS || event.response_type == XCB_KEY_RELEASE ) {
		keyEvent( client, event );
		return;
	}
	auto changed = mKeymap.update( event );
	if ( !changed ) {
		connectionLost( client, std::move( changed ).error() );
	} else if ( changed.value() ) {
		if ( auto error = apply( true ) ) {
			connectionLost( client, std::move( *error ) );
		} else {
			client.bindingsChanged( report(), {} );
		}
	}
}

void X11ShortcutBackend::keyEvent( Client& client, const xcb_generic_event_t& event ) {
	const auto& key = reinterpret_cast<const xcb_key_press_event_t&>( event );
	if ( event.response_type == XCB_KEY_RELEASE ) {
		mPressed.reset( key.detail );
		return;
	}
	if ( mKeymap.detectableRepeat() && mPressed[key.detail] )
		return;
	const Grab pair{ key.event, { key.detail, static_cast<Uint8>( key.state ) } };
	for ( const auto& [id, binding] : mRegistered ) {
		if ( !std::binary_search( binding.grabs.begin(), binding.grabs.end(), pair ) )
			continue;
		mPressed.set( key.detail );
		// XCB expands the 16-bit wire sequence. Unsigned distance also handles a wrap.
		if ( event.full_sequence - binding.firstSequence < 0x80000000U )
			client.activate( binding.incarnation, {} );
		return;
	}
}

} // namespace

std::unique_ptr<ShortcutBackend> createX11ShortcutBackend() {
	const auto present = []( const char* name ) {
		const auto* value = std::getenv( name );
		return value && *value;
	};
	const auto* session = std::getenv( "XDG_SESSION_TYPE" );
	if ( ( session && std::string_view( session ) == "wayland" ) || present( "WAYLAND_DISPLAY" ) ||
		 present( "WAYLAND_SOCKET" ) )
		return nullptr;
	for ( const char* name : { "SDL_VIDEO_DRIVER", "SDL_VIDEODRIVER" } ) {
		const char* value = std::getenv( name );
		const auto driver = value ? std::string_view( value ) : std::string_view{};
		if ( driver == "wayland" || driver == "offscreen" || driver == "dummy" )
			return nullptr;
	}
	const auto* display = std::getenv( "DISPLAY" );
	if ( !display || !*display )
		return nullptr;
	return std::make_unique<X11ShortcutBackend>( display );
}

} // namespace EE::System::GlobalShortcuts::Private

#endif
