#include "shortcutbackend.hpp"
#ifdef EE_GLOBALSHORTCUTS_PORTAL
#include "platform/wayland/shortcutbackendportal.hpp"
#endif
#if EE_PLATFORM == EE_PLATFORM_WIN
#include "platform/windows/shortcutbackendwindows.hpp"
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include "platform/macos/shortcutbackendmacos.hpp"
#elif defined( EE_GLOBALSHORTCUTS_X11 )
#include "platform/x11/shortcutbackendx11.hpp"
#endif
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	defined( EE_GLOBALSHORTCUTS_X11 ) || defined( EE_GLOBALSHORTCUTS_PORTAL )
#include <eepp/window/runtime.hpp>
#endif
#include <algorithm>
#include <eepp/core/small_vector.hpp>
#include <eepp/globalshortcuts/service.hpp>
#include <iterator>
#include <mutex>
#include <system_error>
#include <thread>
#include <utf8cpp/utf8.h>

namespace EE::System::GlobalShortcuts {

namespace {

using namespace EE::Window;
using Private::ShortcutBackend;

using RegistrationResult = Result<ServiceSnapshot, Error>;
using CloseResult = Result<void, Error>;
using Binding = ShortcutBackend::Binding;

constexpr size_t MaxPendingActivations = 64;

std::unique_ptr<ShortcutBackend> createDesktopBackend( [[maybe_unused]] Options options ) {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	defined( EE_GLOBALSHORTCUTS_X11 ) || defined( EE_GLOBALSHORTCUTS_PORTAL )
	if ( Window::Runtime::isOffscreen() )
		return nullptr;
#endif
#ifdef EE_GLOBALSHORTCUTS_PORTAL
	if ( Private::isWaylandShortcutSession() )
		return Private::createPortalShortcutBackend( std::move( options ) );
#endif
#if EE_PLATFORM == EE_PLATFORM_WIN
	return Private::createWindowsShortcutBackend();
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::createMacOSShortcutBackend();
#elif defined( EE_GLOBALSHORTCUTS_X11 )
	return Private::createX11ShortcutBackend();
#else
	return nullptr;
#endif
}

Error cancelled() {
	return { ErrorCode::Cancelled, "Shortcut registration cancelled" };
}

Error superseded() {
	return { ErrorCode::Superseded, "Shortcut declaration superseded" };
}

Error shuttingDown() {
	return { ErrorCode::ShuttingDown, "Shortcut service is closing" };
}

bool normalize( Trigger& trigger ) {
	constexpr Uint32 groups[] = { KEYMOD_SHIFT, KEYMOD_CTRL, KEYMOD_ALT, KEYMOD_META };
	if ( trigger.modifiers & ~static_cast<Uint32>( KEYMOD_CTRL_SHIFT_ALT_META ) )
		return false;
	for ( Uint32 group : groups ) {
		if ( trigger.modifiers & group )
			trigger.modifiers |= group;
	}
	if ( ( trigger.key >= KEY_A && trigger.key <= KEY_Z ) ||
		 ( trigger.key >= KEY_0 && trigger.key <= KEY_9 ) ||
		 ( trigger.key >= KEY_F1 && trigger.key <= KEY_F12 ) )
		return true;
	switch ( trigger.key ) {
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		case KEY_HOME:
		case KEY_END:
		case KEY_PAGEUP:
		case KEY_PAGEDOWN:
		case KEY_INSERT:
		case KEY_DELETE:
		case KEY_RETURN:
		case KEY_ESCAPE:
		case KEY_SPACE:
		case KEY_TAB:
		case KEY_BACKSPACE:
			return true;
		default:
			return false;
	}
}

std::optional<Error> validate( std::vector<Shortcut>& shortcuts ) {
	std::sort( shortcuts.begin(), shortcuts.end(),
			   []( const Shortcut& a, const Shortcut& b ) { return a.id < b.id; } );
	for ( auto it = shortcuts.begin(); it != shortcuts.end(); ++it ) {
		if ( it->id.empty() || it->id.find( '\0' ) != std::string::npos ||
			 it->description.find( '\0' ) != std::string::npos ||
			 !utf8::is_valid( it->id.begin(), it->id.end() ) ||
			 !utf8::is_valid( it->description.begin(), it->description.end() ) )
			return Error{ ErrorCode::InvalidShortcut, "Invalid shortcut ID or description" };
		if ( it != shortcuts.begin() && std::prev( it )->id == it->id )
			return Error{ ErrorCode::InvalidShortcut, "Duplicate shortcut ID: " + it->id };
		if ( !it->preferred )
			continue;
		if ( !normalize( *it->preferred ) )
			return Error{ ErrorCode::InvalidShortcut, "Unsupported shortcut trigger: " + it->id };
		if ( std::any_of( shortcuts.begin(), it, [&]( const Shortcut& other ) {
				 return other.preferred == it->preferred;
			 } ) )
			return Error{ ErrorCode::InvalidShortcut, "Duplicate shortcut trigger: " + it->id };
	}
	return {};
}

} // namespace

struct Service::Impl {
	struct Configuration {
		std::string activationToken;
		TaskResolver<CloseResult> resolve;
	};

	struct Request {
		std::vector<Shortcut> shortcuts;
		TaskResolver<RegistrationResult> resolve;
		CancellationToken cancel;
		Uint64 id;
		bool retry;
	};

	struct LiveBinding {
		Binding binding;
		Uint64 delivery;
	};

	struct PendingActivation {
		Activation activation;
		Uint64 delivery;
	};

	struct Shared final : ShortcutBackend::Client, std::enable_shared_from_this<Shared> {
		using Activations = SmallVector<PendingActivation, 4>;

		Shared( Executor& executor, std::unique_ptr<ShortcutBackend> backend ) :
			executor( executor ), backend( std::move( backend ) ) {}

		bool closing() const {
			return snapshot.state == State::Stopping || snapshot.state == State::Closed;
		}

		bool desires( const std::string& id ) const {
			const auto it =
				std::lower_bound( snapshot.desired.begin(), snapshot.desired.end(), id,
								  []( const Shortcut& shortcut, const std::string& key ) {
									  return shortcut.id < key;
								  } );
			return it != snapshot.desired.end() && it->id == id;
		}

		void revise() {
			++snapshot.revision;
			statePending = true;
		}

		auto findBinding( const std::string& id ) const {
			const auto it =
				std::lower_bound( bindings.begin(), bindings.end(), id,
								  []( const LiveBinding& binding, const std::string& key ) {
									  return binding.binding.shortcut.id < key;
								  } );
			return it != bindings.end() && it->binding.shortcut.id == id ? it : bindings.end();
		}

		void refreshEffective() {
			snapshot.effective.clear();
			snapshot.effective.reserve( snapshot.desired.size() + bindings.size() );
			for ( const auto& shortcut : snapshot.desired ) {
				const auto it = findBinding( shortcut.id );
				if ( it != bindings.end() ) {
					snapshot.effective.push_back( it->binding.shortcut );
				} else {
					snapshot.effective.emplace_back(
						BoundShortcut{ shortcut.id,
									   {},
									   {},
									   {},
									   latest ? BindingStatus::Pending : BindingStatus::Unbound } );
				}
			}
			for ( const auto& binding : bindings ) {
				if ( !desires( binding.binding.shortcut.id ) )
					snapshot.effective.push_back( binding.binding.shortcut );
			}
		}

		static void sortBindings( std::vector<Binding>& effective ) {
			std::sort( effective.begin(), effective.end(),
					   []( const Binding& a, const Binding& b ) {
						   return a.shortcut.id < b.shortcut.id;
					   } );
		}

		void updateBindings( std::vector<Binding> effective, bool committed ) {
			std::vector<LiveBinding> updated;
			updated.reserve( effective.size() );
			for ( auto& binding : effective ) {
				Uint64 delivery = 0;
				if ( binding.incarnation && binding.shortcut.status == BindingStatus::Bound &&
					 desires( binding.shortcut.id ) ) {
					const auto old = findBinding( binding.shortcut.id );
					const bool retained =
						old != bindings.end() && old->binding.incarnation == binding.incarnation;
					if ( retained )
						delivery = old->delivery;
					if ( !delivery && ( committed || ( !latest && !retained ) ) )
						delivery = ++nextDelivery;
				}
				updated.push_back( { std::move( binding ), delivery } );
			}
			bindings = std::move( updated );
			discardStaleActivations();
		}

		bool isCurrent( const PendingActivation& activation ) const {
			const auto it = findBinding( activation.activation.id );
			return it != bindings.end() && it->delivery == activation.delivery;
		}

		void discardStaleActivations() {
			const auto end =
				std::remove_if( activations.begin(), activations.end(),
								[&]( const auto& item ) { return !isCurrent( item ); } );
			activations.erase( end, activations.end() );
		}

		State settledState() const {
			const auto allBound = std::all_of( snapshot.effective.begin(), snapshot.effective.end(),
											   [&]( const BoundShortcut& binding ) {
												   return binding.status == BindingStatus::Bound &&
														  !binding.error && desires( binding.id );
											   } );
			if ( snapshot.error || !allBound )
				return State::Degraded;
			return snapshot.desired.empty() ? State::Idle : State::Ready;
		}

		void dispatch() {
			{
				std::lock_guard lock( mutex );
				if ( closing() || scheduled ||
					 ( activations.empty() && !( statePending && stateCallback ) ) )
					return;
				scheduled = true;
			}
			executor.execute( [weak = weak_from_this()] {
				if ( auto state = weak.lock() )
					state->drain();
			} );
		}

		void drain() {
			eeASSERT( std::this_thread::get_id() == applicationThread );
			Activations batch;
			std::shared_ptr<StateChangedCallback> onState;
			std::optional<ServiceSnapshot> current;
			{
				std::lock_guard lock( mutex );
				scheduled = false;
				if ( closing() )
					return;
				batch = std::move( activations );
				if ( statePending && stateCallback ) {
					onState = stateCallback;
					current = snapshot;
					statePending = false;
				}
			}
			if ( onState )
				( *onState )( *current );
			for ( const auto& activation : batch ) {
				std::shared_ptr<ActivationCallback> onActivation;
				{
					std::lock_guard lock( mutex );
					if ( closing() )
						break;
					if ( isCurrent( activation ) )
						onActivation = activationCallback;
				}
				if ( onActivation )
					( *onActivation )( activation.activation );
			}
		}

		void resolveRegistration( Request command, std::optional<Error> error,
								  bool unavailable = false,
								  std::optional<std::vector<Binding>> effective = {} ) {
			std::optional<ServiceSnapshot> report;
			{
				std::lock_guard lock( mutex );
				if ( effective && !closing() )
					updateBindings( std::move( *effective ), latest == command.id );
				if ( closing() ) {
					error = shuttingDown();
				} else if ( latest != command.id ) {
					error = superseded();
					if ( effective ) {
						refreshEffective();
						revise();
					}
				} else {
					latest = 0;
					refreshEffective();
					snapshot.error = error;
					snapshot.state =
						unavailable || ( error && ( error->code == ErrorCode::Unsupported ||
													error->code == ErrorCode::BackendUnavailable ) )
							? State::Unavailable
							: settledState();
					revise();
					if ( !error )
						report = snapshot;
				}
			}
			if ( error )
				command.resolve( Err( std::move( *error ) ) );
			else
				command.resolve( Ok( std::move( *report ) ) );
			dispatch();
		}

		void complete( Uint64 request, Result<std::vector<Binding>, Error> result ) override {
			if ( !active || request != active->id )
				return;
			auto command = std::move( *active );
			active.reset();
			if ( result ) {
				sortBindings( result.value() );
				resolveRegistration( std::move( command ), {}, false, std::move( result ).value() );
			} else {
				resolveRegistration( std::move( command ), std::move( result ).error() );
			}
		}

		void bindingsChanged( std::vector<Binding> effective,
							  std::optional<Error> issue ) override {
			sortBindings( effective );
			{
				std::lock_guard lock( mutex );
				if ( closing() )
					return;
				updateBindings( std::move( effective ), false );
				refreshEffective();
				if ( !latest )
					snapshot.state = issue ? State::Degraded : settledState();
				if ( issue )
					snapshot.lastBackendIssue = std::move( issue );
				revise();
			}
			dispatch();
		}

		void capabilitiesChanged( Capabilities capabilities ) override {
			{
				std::lock_guard lock( mutex );
				if ( closing() )
					return;
				snapshot.capabilities = capabilities;
				revise();
			}
			dispatch();
		}

		void activate( Uint64 incarnation, std::optional<std::string> token ) override {
			{
				std::lock_guard lock( mutex );
				if ( closing() || !activationCallback )
					return;
				const auto it =
					std::find_if( bindings.begin(), bindings.end(), [&]( const auto& b ) {
						return b.binding.incarnation == incarnation && b.delivery != 0;
					} );
				if ( it == bindings.end() )
					return;
				if ( activations.size() == MaxPendingActivations ) {
					++snapshot.droppedActivations;
					revise();
					if ( !makeRoomForActivation( it->delivery ) )
						return;
				}
				activations.push_back(
					{ { it->binding.shortcut.id, std::move( token ) }, it->delivery } );
			}
			dispatch();
		}

		bool makeRoomForActivation( Uint64 delivery ) {
			if ( std::any_of( activations.begin(), activations.end(),
							  [&]( const auto& item ) { return item.delivery == delivery; } ) )
				return false;
			for ( auto first = activations.begin(); first != activations.end(); ++first ) {
				const auto duplicate =
					std::find_if( std::next( first ), activations.end(), [&]( const auto& queued ) {
						return first->delivery == queued.delivery;
					} );
				if ( duplicate != activations.end() ) {
					activations.erase( duplicate );
					return true;
				}
			}
			return false;
		}

		void wakeNative() {
			if ( backend->usesApplicationThread() )
				scheduleNativeWork();
			else
				backend->wake();
		}

		void scheduleNativeWork() override {
			{
				std::lock_guard lock( mutex );
				if ( nativeScheduled || snapshot.state == State::Closed )
					return;
				nativeScheduled = true;
			}
			executor.execute( [weak = weak_from_this()] {
				if ( auto state = weak.lock() ) {
					{
						std::lock_guard lock( state->mutex );
						state->nativeScheduled = false;
					}
					state->pumpNative();
				}
			} );
		}

		void pumpNative() {
			eeASSERT( std::this_thread::get_id() == applicationThread );
			// A task continuation can destroy Service while a native command completes.
			if ( pumpingNative || snapshot.state == State::Closed )
				return;
			pumpingNative = true;
			if ( processCommands() && opened )
				backend->processEvents( *this );
			pumpingNative = false;
		}

		void run() {
			while ( processCommands() )
				backend->wait( *this );
		}

		bool processCommands() {
			for ( ;; ) {
				std::optional<Request> next;
				std::optional<Configuration> configuration;
				bool stop;
				bool activeSuperseded;
				{
					std::lock_guard lock( mutex );
					stop = closing();
					activeSuperseded = active && active->id != latest;
					next.swap( pending );
					configuration.swap( pendingConfiguration );
				}
				if ( configuration ) {
					if ( stop || next || active ) {
						configuration->resolve(
							Err( stop ? shuttingDown()
									  : Error{ ErrorCode::Busy,
											   "Shortcut registration is in progress" } ) );
					} else {
						backend->configure( std::move( configuration->activationToken ),
											std::move( configuration->resolve ) );
					}
				}
				if ( active &&
					 ( stop || activeSuperseded || active->cancel.isCancellationRequested() ) ) {
					auto command = std::move( *active );
					active.reset();
					if ( nativePending )
						backend->cancel( *this, command.id );
					resolveRegistration( std::move( command ), cancelled() );
				}
				if ( !active )
					cancellation.reset();
				if ( stop ) {
					if ( next )
						resolveRegistration( std::move( *next ), shuttingDown() );
					finishClose( backend->close() );
					return false;
				}
				if ( next ) {
					active = std::move( next );
					nativePending = false;
					if ( active->cancel.isCancellationRequested() ) {
						complete( active->id, Err( cancelled() ) );
						continue;
					}
					cancellation = active->cancel.onCancel( [this] { wakeNative(); } );
					if ( !opened ) {
						if ( active->shortcuts.empty() ) {
							complete( active->id, Ok( std::vector<Binding>{} ) );
							continue;
						}
						auto available = backend->open();
						if ( !available ) {
							auto command = std::move( *active );
							active.reset();
							resolveRegistration( std::move( command ),
												 std::move( available ).error(), true );
							continue;
						}
						opened = true;
						{
							std::lock_guard lock( mutex );
							snapshot.backend = backend->type();
							snapshot.capabilities = available.value();
							revise();
						}
						dispatch();
					}
					bool canOmitPreferredTrigger;
					{
						std::lock_guard lock( mutex );
						if ( closing() || active->id != latest ||
							 active->cancel.isCancellationRequested() )
							continue;
						canOmitPreferredTrigger = snapshot.capabilities.canOmitPreferredTrigger;
					}
					if ( !canOmitPreferredTrigger &&
						 std::any_of(
							 active->shortcuts.begin(), active->shortcuts.end(),
							 []( const Shortcut& shortcut ) { return !shortcut.preferred; } ) ) {
						complete( active->id,
								  Err( Error{ ErrorCode::InvalidShortcut,
											  "This backend requires a preferred trigger" } ) );
						continue;
					}
					nativePending = true;
					backend->reconcile( *this, active->id, std::move( active->shortcuts ),
										active->retry );
					continue;
				}
				return true;
			}
		}

		void finishClose( CloseResult result ) {
			SmallVector<TaskResolver<CloseResult>, 1> waiters;
			{
				std::lock_guard lock( mutex );
				bindings.clear();
				snapshot.desired.clear();
				snapshot.effective.clear();
				snapshot.capabilities = {};
				snapshot.error = result ? std::nullopt : std::optional<Error>( result.error() );
				snapshot.state = State::Closed;
				revise();
				waiters = std::move( closeWaiters );
			}
			for ( auto& resolver : waiters )
				resolver( result );
		}

		Executor& executor;
		const std::thread::id applicationThread{ std::this_thread::get_id() };
		const std::unique_ptr<ShortcutBackend> backend;
		mutable std::mutex mutex;
		ServiceSnapshot snapshot;
		std::vector<LiveBinding> bindings;
		Activations activations;
		std::shared_ptr<StateChangedCallback> stateCallback;
		std::shared_ptr<ActivationCallback> activationCallback;
		SmallVector<TaskResolver<CloseResult>, 1> closeWaiters;
		std::optional<Request> pending;
		std::optional<Configuration> pendingConfiguration;
		Uint64 nextRequest{ 0 };
		Uint64 latest{ 0 };
		Uint64 nextDelivery{ 0 };
		bool statePending{ false };
		bool scheduled{ false };
		bool nativeScheduled{ false };

		// Accessed only by the thread that owns native registrations.
		bool opened{ false };
		bool nativePending{ false };
		bool pumpingNative{ false };
		std::optional<Request> active;
		CancellationRegistration cancellation;
	};

	Impl( Executor& executor, std::unique_ptr<ShortcutBackend> backend ) :
		shared( std::make_shared<Shared>( executor, std::move( backend ) ) ) {}

	~Impl() {
		requestClose( {} );
		if ( worker.joinable() )
			worker.join();
		else if ( applicationBackendStarted )
			shared->pumpNative();
	}

	void assertApplicationThread() const {
		eeASSERT( std::this_thread::get_id() == shared->applicationThread );
	}

	RegistrationTask submit( std::vector<Shortcut> shortcuts, CancellationToken cancel,
							 bool retry ) {
		assertApplicationThread();
		std::optional<Request> replaced;
		RegistrationTask task;
		{
			std::lock_guard lock( shared->mutex );
			auto& snapshot = shared->snapshot;
			if ( shared->closing() )
				return RegistrationTask::ready( Err( shuttingDown() ) );
			if ( cancel.isCancellationRequested() )
				return RegistrationTask::ready( Err( cancelled() ) );
			const bool changed = snapshot.desired != shortcuts;
			if ( !changed && !retry ) {
				if ( shared->latest ) {
					return RegistrationTask::ready(
						Err( Error{ ErrorCode::Busy, "This declaration is already pending" } ) );
				}
				if ( snapshot.error )
					return RegistrationTask::ready( Err( *snapshot.error ) );
				return RegistrationTask::ready( Ok( snapshot ) );
			}
			if ( changed ) {
				snapshot.desired = shortcuts;
				++snapshot.generation;
				for ( auto& binding : shared->bindings ) {
					if ( !shared->desires( binding.binding.shortcut.id ) )
						binding.delivery = 0;
				}
				shared->discardStaleActivations();
			}
			if ( !worker.joinable() && !applicationBackendStarted && shortcuts.empty() ) {
				snapshot.state = State::Idle;
				snapshot.error.reset();
				shared->refreshEffective();
				shared->revise();
				task = RegistrationTask::ready( Ok( snapshot ) );
			} else if ( !shared->backend ) {
				snapshot.state = State::Unavailable;
				snapshot.error = Error{ ErrorCode::Unsupported, "No native shortcut backend" };
				shared->refreshEffective();
				shared->revise();
				task = RegistrationTask::ready( Err( *snapshot.error ) );
			} else {
				replaced.swap( shared->pending );
				shared->latest = ++shared->nextRequest;
				Request request{
					std::move( shortcuts ), {}, std::move( cancel ), shared->latest, retry };
				task = RegistrationTask::create( request.resolve );
				shared->pending = std::move( request );
				snapshot.state = State::Registering;
				snapshot.error.reset();
				shared->refreshEffective();
				shared->revise();
			}
		}
		if ( shared->backend && !task.isReady() ) {
			if ( shared->backend->usesApplicationThread() ) {
				applicationBackendStarted = true;
				shared->scheduleNativeWork();
			} else if ( !worker.joinable() ) {
				try {
					worker = std::thread( [state = shared] { state->run(); } );
				} catch ( const std::system_error& error ) {
					std::optional<Request> failed;
					{
						std::lock_guard lock( shared->mutex );
						failed.swap( shared->pending );
					}
					shared->resolveRegistration(
						std::move( *failed ),
						Error{ ErrorCode::BackendUnavailable, error.what(), error.code().value() },
						true );
				}
			} else {
				shared->backend->wake();
			}
		}
		shared->dispatch();
		if ( replaced )
			replaced->resolve( Err( superseded() ) );
		return task;
	}

	void requestClose( std::optional<TaskResolver<CloseResult>> resolver ) {
		assertApplicationThread();
		std::shared_ptr<StateChangedCallback> stateCallback;
		std::shared_ptr<ActivationCallback> activationCallback;
		std::optional<CloseResult> closed;
		bool beginClose = false;
		{
			std::lock_guard lock( shared->mutex );
			auto& snapshot = shared->snapshot;
			if ( snapshot.state == State::Closed ) {
				closed =
					snapshot.error ? CloseResult( Err( *snapshot.error ) ) : CloseResult( Ok() );
			} else {
				if ( resolver )
					shared->closeWaiters.push_back( std::move( *resolver ) );
				if ( snapshot.state != State::Stopping ) {
					snapshot.state = State::Stopping;
					shared->revise();
					stateCallback = std::move( shared->stateCallback );
					activationCallback = std::move( shared->activationCallback );
					shared->activations.clear();
					beginClose = true;
				}
			}
		}
		if ( beginClose ) {
			if ( worker.joinable() || applicationBackendStarted )
				shared->wakeNative();
			else
				shared->finishClose( Ok() );
		}
		if ( resolver && closed )
			( *resolver )( std::move( *closed ) );
	}

	std::shared_ptr<Shared> shared;
	std::thread worker;
	bool applicationBackendStarted{ false };
};

Service::Service( Executor& applicationExecutor ) :
	Service( applicationExecutor, createDesktopBackend( {} ) ) {}

Service::Service( Executor& applicationExecutor, Options options ) :
	Service( applicationExecutor, createDesktopBackend( std::move( options ) ) ) {}

Service::Service( Executor& executor, std::unique_ptr<ShortcutBackend> backend ) :
	mImpl( std::make_unique<Impl>( executor, std::move( backend ) ) ) {}

Service::~Service() = default;

void Service::setActivationCallback( ActivationCallback callback ) {
	mImpl->assertApplicationThread();
	auto next = callback ? std::make_shared<ActivationCallback>( std::move( callback ) ) : nullptr;
	std::lock_guard lock( mImpl->shared->mutex );
	if ( !mImpl->shared->closing() )
		mImpl->shared->activationCallback.swap( next );
}

void Service::setStateChangedCallback( StateChangedCallback callback ) {
	mImpl->assertApplicationThread();
	auto next =
		callback ? std::make_shared<StateChangedCallback>( std::move( callback ) ) : nullptr;
	{
		std::lock_guard lock( mImpl->shared->mutex );
		if ( !mImpl->shared->closing() )
			mImpl->shared->stateCallback.swap( next );
	}
	mImpl->shared->dispatch();
}

RegistrationTask Service::setShortcuts( std::vector<Shortcut> shortcuts,
										CancellationToken cancel ) {
	mImpl->assertApplicationThread();
	{
		std::lock_guard lock( mImpl->shared->mutex );
		if ( mImpl->shared->closing() )
			return RegistrationTask::ready( Err( shuttingDown() ) );
	}
	if ( cancel.isCancellationRequested() )
		return RegistrationTask::ready( Err( cancelled() ) );
	if ( auto error = validate( shortcuts ) )
		return RegistrationTask::ready( Err( std::move( *error ) ) );
	return mImpl->submit( std::move( shortcuts ), std::move( cancel ), false );
}

RegistrationTask Service::clear() {
	return mImpl->submit( {}, {}, false );
}

RegistrationTask Service::retry( CancellationToken cancel ) {
	mImpl->assertApplicationThread();
	std::vector<Shortcut> desired;
	{
		std::lock_guard lock( mImpl->shared->mutex );
		desired = mImpl->shared->snapshot.desired;
	}
	return mImpl->submit( std::move( desired ), std::move( cancel ), true );
}

OperationTask Service::configureShortcuts( std::string activationToken ) {
	mImpl->assertApplicationThread();
	OperationTask task;
	{
		std::lock_guard lock( mImpl->shared->mutex );
		auto& state = *mImpl->shared;
		if ( state.closing() )
			return OperationTask::ready( Err( shuttingDown() ) );
		if ( !state.snapshot.capabilities.canConfigureShortcuts )
			return OperationTask::ready(
				Err( Error{ ErrorCode::Unsupported, "Shortcut settings are unavailable" } ) );
		if ( state.latest || state.pendingConfiguration )
			return OperationTask::ready(
				Err( Error{ ErrorCode::Busy, "Shortcut service is busy" } ) );
		Impl::Configuration config{ std::move( activationToken ), {} };
		task = OperationTask::create( config.resolve );
		state.pendingConfiguration = std::move( config );
	}
	mImpl->shared->wakeNative();
	return task;
}

OperationTask Service::closeAsync() {
	TaskResolver<CloseResult> resolver;
	auto task = OperationTask::create( resolver );
	mImpl->requestClose( std::move( resolver ) );
	return task;
}

ServiceSnapshot Service::snapshot() const {
	mImpl->assertApplicationThread();
	std::lock_guard lock( mImpl->shared->mutex );
	return mImpl->shared->snapshot;
}

} // namespace EE::System::GlobalShortcuts
