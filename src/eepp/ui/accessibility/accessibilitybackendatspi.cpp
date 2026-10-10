#include "accessibilitybackendatspi.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace EE { namespace UI { namespace AtSpi {

AtSpiApplication::AtSpiApplication( AccessibilityManager& manager ) :
	mManager( &manager ), mPrimaryManager( &manager ) {
	registerManager( manager );
	if ( manager.getSceneNode() && manager.getSceneNode()->getWindow() )
		mName = String::fromUtf8( manager.getSceneNode()->getWindow()->getTitle() );
}

void AtSpiApplication::initialize() {
	if ( !mDBus.load() )
		return initializationFinished();
	DBusConnection* sessionBus = mDBus.busGet( 0, nullptr );
	if ( !sessionBus )
		return initializationFinished();
	DBusMessage* request =
		mDBus.messageNewMethodCall( "org.a11y.Bus", "/org/a11y/bus", "org.a11y.Bus", "GetAddress" );
	if ( !request )
		return initializationFinished();
	DBusMessage* reply =
		mDBus.connectionSendWithReplyAndBlock( sessionBus, request, 1000, nullptr );
	mDBus.messageUnref( request );
	if ( !reply )
		return initializationFinished();
	const char* address = nullptr;
	const bool gotAddress = mDBus.messageGetArgs( reply, nullptr, 's', &address, 0 ) != 0;
	if ( gotAddress && address ) {
		mConnection = mDBus.connectionOpenPrivate( address, nullptr );
		if ( mConnection && !mDBus.busRegister( mConnection, nullptr ) ) {
			mDBus.connectionClose( mConnection );
			mDBus.connectionUnref( mConnection );
			mConnection = nullptr;
		}
	}
	mDBus.messageUnref( reply );
	if ( mConnection ) {
		// Client disconnections are watched per client (watchClient()), not with a bus-wide
		// NameOwnerChanged match that would wake us for every application on the bus.
		mDBus.connectionAddFilter( mConnection, &AtSpiApplication::clientDisconnected, this,
								   nullptr );
		registerApplication();
		if ( mConnection && mDBus.connectionGetUnixFd( mConnection, &mConnectionFd ) &&
			 pipe( mWakePipe ) == 0 ) {
			for ( int fd : mWakePipe )
				fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK );
			mRunning.store( true, std::memory_order_release );
			mIOThread = std::thread( &AtSpiApplication::waitForMessages, this );
		}
		// Socket.Embed emits the registry's application-add event before replying. A client can
		// query us immediately, and the blocking Embed call may already have moved that query
		// into libdbus's internal dispatch queue before the I/O thread starts polling the
		// drained socket.
		if ( mConnection )
			mDispatchRequested.store( true, std::memory_order_release );
	}
	initializationFinished();
}

AtSpiApplication::~AtSpiApplication() {
	if ( mConnection ) {
		mRunning.store( false, std::memory_order_release );
		if ( mIOThread.joinable() ) {
			const char stop = 1;
			const ssize_t written = write( mWakePipe[1], &stop, sizeof( stop ) );
			(void)written;
			mIOThread.join();
			close( mWakePipe[0] );
			close( mWakePipe[1] );
		}
		mDBus.connectionClose( mConnection );
		mDBus.connectionUnref( mConnection );
	}
}

Uint64 AtSpiApplication::registerManager( AccessibilityManager& manager ) {
	for ( const auto& entry : mManagers ) {
		if ( entry.second == &manager )
			return entry.first;
	}
	const Uint64 id = mNextManagerId++;
	mManagers.emplace_back( id, &manager );
	// initialize() may still be constructing the connection on the scene's ThreadPool.
	// Acquire its completion flag before reading any of the published D-Bus state.
	if ( isAvailable() && hasActiveClients() )
		mPendingWindowAdds.push_back( &manager );
	return id;
}

void AtSpiApplication::unregisterManager( AccessibilityManager& manager ) {
	mPendingWindowAdds.erase(
		std::remove( mPendingWindowAdds.begin(), mPendingWindowAdds.end(), &manager ),
		mPendingWindowAdds.end() );
	mPendingTexts.erase( std::remove_if( mPendingTexts.begin(), mPendingTexts.end(),
										 [&manager]( const PendingText& text ) {
											 return text.manager == &manager;
										 } ),
						 mPendingTexts.end() );
	for ( auto it = mManagers.begin(); it != mManagers.end(); ++it ) {
		if ( it->second != &manager )
			continue;
		const Int32 index = static_cast<Int32>( it - mManagers.begin() );
		if ( !mTextStates.empty() ) {
			const auto prefix = std::string( NodePathPrefix ) + String::toString( it->first ) + "/";
			for ( auto text = mTextStates.begin(); text != mTextStates.end(); ) {
				if ( text->first.compare( 0, prefix.size(), prefix ) == 0 )
					text = mTextStates.erase( text );
				else
					++text;
			}
		}
		if ( isAvailable() && hasActiveClients() )
			sendWindowChanged( manager, false, index );
		mManagers.erase( it );
		break;
	}
	if ( mManager == &manager )
		mManager = mManagers.empty() ? nullptr : mManagers.begin()->second;
	if ( mPrimaryManager.load( std::memory_order_acquire ) == &manager ) {
		auto* primaryManager = mManagers.empty() ? nullptr : mManagers.begin()->second;
		mPrimaryManager.store( primaryManager, std::memory_order_release );
	}
}

void AtSpiApplication::update() {
	if ( !mInitialized.load( std::memory_order_acquire ) || !mConnection )
		return;
	flushPendingTexts();
	// Signals queued by widget updates must leave the connection even when no client sends
	// another query. Otherwise typing notifications wait indefinitely in libdbus's queue.
	if ( mOutgoingPending ) {
		mDBus.connectionReadWrite( mConnection, 0 );
		mOutgoingPending = mDBus.connectionGetOutgoingSize( mConnection ) > 0;
		if ( mDBus.connectionGetDispatchStatus( mConnection ) == DBusDispatchDataRemains )
			mDispatchRequested.store( true, std::memory_order_release );
	}
	if ( !mPendingWindowAdds.empty() ) {
		for ( auto* manager : mPendingWindowAdds ) {
			for ( size_t i = 0; i < mManagers.size(); ++i ) {
				if ( mManagers[i].second == manager ) {
					sendWindowChanged( *manager, true, static_cast<Int32>( i ) );
					break;
				}
			}
		}
		mPendingWindowAdds.clear();
	}
	if ( !mDispatchRequested.exchange( false, std::memory_order_acq_rel ) )
		return;
	mDBus.connectionReadWrite( mConnection, 0 );
	constexpr size_t MaxMessagesPerUpdate = 256;
	for ( size_t i = 0; i < MaxMessagesPerUpdate &&
						mDBus.connectionGetDispatchStatus( mConnection ) == DBusDispatchDataRemains;
		  ++i )
		mDBus.connectionDispatch( mConnection );
	mDBus.connectionReadWrite( mConnection, 0 );
	// connectionReadWrite() can move multiple requests from the socket into libdbus. Once the
	// socket is drained, poll() will not wake us for requests that remain in libdbus's internal
	// dispatch queue. Preserve the bounded per-update work without stranding that backlog.
	if ( mDBus.connectionGetDispatchStatus( mConnection ) == DBusDispatchDataRemains ) {
		mDispatchRequested.store( true, std::memory_order_release );
		Window::Input::wakeUpEventLoop();
	} else {
		const char drained = 1;
		const ssize_t written = write( mWakePipe[1], &drained, sizeof( drained ) );
		(void)written;
	}
}

void AtSpiApplication::waitForMessages() {
	bool dispatchPending = false;
	while ( mRunning.load( std::memory_order_acquire ) ) {
		// A readable socket stays readable until the UI thread drains it. Do not poll it
		// again while dispatch is pending: only the drain/stop pipe can re-arm this watcher.
		pollfd descriptors[2]{ { dispatchPending ? -1 : mConnectionFd, POLLIN, 0 },
							   { mWakePipe[0], POLLIN, 0 } };
		const int ready = poll( descriptors, 2, -1 );
		if ( ready < 0 && errno == EINTR )
			continue;
		if ( ready <= 0 || descriptors[0].revents & ( POLLERR | POLLHUP | POLLNVAL ) )
			break;
		if ( descriptors[1].revents & POLLIN ) {
			char wake[64];
			const ssize_t consumed = read( mWakePipe[0], wake, sizeof( wake ) );
			(void)consumed;
			dispatchPending = mDispatchRequested.load( std::memory_order_acquire );
			continue;
		}
		if ( !( descriptors[0].revents & POLLIN ) )
			continue;
		dispatchPending = true;
		if ( mDispatchRequested.exchange( true, std::memory_order_acq_rel ) )
			continue;
		// UIApplication waits on its first live window, not necessarily the queried scene.
		// SDL windows share the event loop, so wake it without retaining an Input pointer that
		// the main thread could delete when that window closes.
		Window::Input::wakeUpEventLoop();
	}
}

DBusHandlerResult AtSpiApplication::handleMessage( DBusConnection*, DBusMessage* message,
												   void* userData ) {
	auto backend = static_cast<AtSpiApplication*>( userData );
	const char* interface = backend->mDBus.messageGetInterface( message );
	const char* member = backend->mDBus.messageGetMember( message );
	const bool registryPropertySet =
		interface && member && std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
		std::strcmp( member, "Set" ) == 0 &&
		std::strcmp( backend->mDBus.messageGetPath( message ), RootPath ) == 0;
	if ( !registryPropertySet )
		backend->activate( message );
	if ( backend->handleMessage( message ) != 0 ) {
		backend->sendError( message, "org.freedesktop.DBus.Error.UnknownMethod",
							"Unsupported accessibility method or property" );
	}
	return 0;
}

void AtSpiApplication::activate( DBusMessage* message ) {
	const char* sender = mDBus.messageGetSender( message );
	// Registry bookkeeping is not evidence of an assistive client. Its bus connection
	// normally outlives every screen reader and would otherwise pin the active flag forever.
	if ( !sender || mRegistryBusName == sender || !mClients.emplace( sender ).second )
		return;
	mHasActiveClients.store( true, std::memory_order_release );
	if ( isAccessibilityTraceEnabled() ) {
		const char* member = mDBus.messageGetMember( message );
		std::fprintf( stderr, "eepp accessibility: AT-SPI client %s connected (first call %s)\n",
					  sender, member ? member : "?" );
	}
	watchClient( sender );
}

namespace {

std::string clientMatchRule( const char* name ) {
	return std::string( "type='signal',sender='org.freedesktop.DBus',"
						"interface='org.freedesktop.DBus',member='NameOwnerChanged',arg0='" ) +
		   name + "'";
}

} // namespace

void AtSpiApplication::watchClient( const char* name ) {
	// Neither request blocks: without an error argument libdbus queues the AddMatch instead of
	// waiting for its reply, and the NameHasOwner reply arrives through clientDisconnected().
	// The bus daemon handles one connection's requests in order, so the check runs after the
	// match is installed: a client that already left is reported by the check, and one that
	// leaves later by the match.
	mDBus.busAddMatch( mConnection, clientMatchRule( name ).c_str(), nullptr );
	DBusMessage* check = mDBus.messageNewMethodCall(
		"org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameHasOwner" );
	if ( !check )
		return;
	DBusMessageIter iter;
	mDBus.messageIterInitAppend( check, &iter );
	appendBasic( iter, 's', &name );
	Uint32 serial = 0;
	if ( mDBus.connectionSend( mConnection, check, &serial ) )
		mClientChecks.emplace( serial, name );
	mDBus.messageUnref( check );
	if ( !mOutgoingPending ) {
		mOutgoingPending = true;
		Window::Input::wakeUpEventLoop();
	}
}

void AtSpiApplication::clientGone( const char* name ) {
	auto found = mClients.find( name );
	if ( found == mClients.end() )
		return;
	if ( isAccessibilityTraceEnabled() )
		std::fprintf( stderr, "eepp accessibility: AT-SPI client %s disconnected\n", name );
	mDBus.busRemoveMatch( mConnection, clientMatchRule( name ).c_str(), nullptr );
	mClients.erase( found );
	const bool active = !mClients.empty();
	if ( !active ) {
		mTextStates.clear();
		mPendingTexts.clear();
	}
	mHasActiveClients.store( active, std::memory_order_release );
}

DBusHandlerResult AtSpiApplication::clientDisconnected( DBusConnection*, DBusMessage* message,
														void* userData ) {
	auto* application = static_cast<AtSpiApplication*>( userData );
	auto& dbus = application->mDBus;
	const int type = dbus.messageGetType( message );
	if ( type == DBusMessageTypeMethodReturn || type == DBusMessageTypeError ) {
		// The reply to a watchClient() NameHasOwner check.
		auto check = application->mClientChecks.find( dbus.messageGetReplySerial( message ) );
		if ( check == application->mClientChecks.end() )
			return 1;
		Uint32 hasOwner = 1;
		if ( type == DBusMessageTypeMethodReturn &&
			 dbus.messageGetArgs( message, nullptr, 'b', &hasOwner, 0 ) && !hasOwner )
			application->clientGone( check->second.c_str() );
		application->mClientChecks.erase( check );
		return 0;
	}
	const char* interface = dbus.messageGetInterface( message );
	const char* member = dbus.messageGetMember( message );
	const char* sender = dbus.messageGetSender( message );
	if ( !interface || !member || std::strcmp( interface, "org.freedesktop.DBus" ) != 0 ||
		 std::strcmp( member, "NameOwnerChanged" ) != 0 || !sender ||
		 std::strcmp( sender, "org.freedesktop.DBus" ) != 0 )
		return 1;
	const char* name = nullptr;
	const char* oldOwner = nullptr;
	const char* newOwner = nullptr;
	if ( dbus.messageGetArgs( message, nullptr, 's', &name, 's', &oldOwner, 's', &newOwner, 0 ) &&
		 name && newOwner && !*newOwner )
		application->clientGone( name );
	return 1;
}

void AtSpiApplication::send( DBusMessage* reply ) {
	if ( !reply )
		return;
	mDBus.connectionSend( mConnection, reply, nullptr );
	mDBus.messageUnref( reply );
	if ( !mOutgoingPending ) {
		mOutgoingPending = true;
		Window::Input::wakeUpEventLoop();
	}
}

void AtSpiApplication::sendBasic( DBusMessage* request, int type, const void* value ) {
	DBusMessage* reply = mDBus.messageNewMethodReturn( request );
	DBusMessageIter iter;
	mDBus.messageIterInitAppend( reply, &iter );
	appendBasic( iter, type, value );
	send( reply );
}

void AtSpiApplication::registerApplication() {
	mBusName = mDBus.busGetUniqueName( mConnection );
	static const DBusObjectPathVTable vtable{
		nullptr, &AtSpiApplication::handleMessage, nullptr, nullptr, nullptr, nullptr };
	if ( mBusName.empty() ||
		 !mDBus.connectionRegisterObjectPath( mConnection, RootPath, &vtable, this ) ||
		 !mDBus.connectionRegisterObjectPath( mConnection, CachePath, &vtable, this ) ||
		 !mDBus.connectionRegisterFallback( mConnection, "/org/eepp/a11y", &vtable, this ) ) {
		mDBus.connectionClose( mConnection );
		mDBus.connectionUnref( mConnection );
		mConnection = nullptr;
		return;
	}

	DBusMessage* embed =
		mDBus.messageNewMethodCall( "org.a11y.atspi.Registry", "/org/a11y/atspi/accessible/root",
									"org.a11y.atspi.Socket", "Embed" );
	if ( !embed )
		return;
	DBusMessageIter iter;
	mDBus.messageIterInitAppend( embed, &iter );
	appendApplicationRef( iter );
	DBusMessage* reply = mDBus.connectionSendWithReplyAndBlock( mConnection, embed, 1000, nullptr );
	mDBus.messageUnref( embed );
	if ( reply && mDBus.messageGetType( reply ) != DBusMessageTypeError ) {
		DBusMessageIter iter;
		DBusMessageIter structure;
		if ( mDBus.messageIterInit( reply, &iter ) ) {
			mDBus.messageIterRecurse( &iter, &structure );
			const char* bus = nullptr;
			const char* path = nullptr;
			mDBus.messageIterGetBasic( &structure, &bus );
			if ( mDBus.messageIterNext( &structure ) )
				mDBus.messageIterGetBasic( &structure, &path );
			if ( bus && path ) {
				mRegistryBusName = bus;
				mRegistryPath = path;
			}
		}
		mDBus.messageUnref( reply );
		return;
	}
	if ( reply )
		mDBus.messageUnref( reply );
	mDBus.connectionClose( mConnection );
	mDBus.connectionUnref( mConnection );
	mConnection = nullptr;
}

}}} // namespace EE::UI::AtSpi

namespace EE { namespace UI {

namespace {

class AtSpiAccessibilityBackend final : public AccessibilityBackend {
  public:
	explicit AtSpiAccessibilityBackend( AccessibilityManager& manager ) : mManager( manager ) {
		auto scene = manager.getSceneNode();
		if ( !scene || !scene->getWindow() || scene->getParent() )
			return;
		mApplication = application().lock();
		if ( !mApplication ) {
			mApplication = std::make_shared<AtSpi::AtSpiApplication>( manager );
			application() = mApplication;
			if ( scene->hasThreadPool() ) {
				scene->getThreadPool()->run(
					[application = mApplication] { application->initialize(); } );
			} else {
				// Without a pool, run the D-Bus round trips on a detached thread that shares
				// ownership: tearing the backend down never waits for their timeouts, and when
				// the thread holds the last reference the application is destroyed there, which
				// joins only the I/O thread.
				std::thread( [application = mApplication] { application->initialize(); } ).detach();
			}
		} else {
			mApplication->registerManager( manager );
		}
	}

	~AtSpiAccessibilityBackend() {
		if ( mApplication )
			mApplication->unregisterManager( mManager );
	}

	bool isAvailable() const { return mApplication && mApplication->isAvailable(); }

	bool isInitializationComplete() const override {
		return mApplication && mApplication->isInitialized();
	}

	bool hasActiveClients() const { return mApplication && mApplication->hasActiveClients(); }

	void update() {
		if ( mApplication )
			mApplication->update();
	}

	void onEvent( const AccessibilityPendingEvent& event ) {
		if ( mApplication )
			mApplication->onEvent( mManager, event );
	}

	bool supportsTextChanges() const override { return true; }

	bool onTextChanged( AccessibilityNodeRef ref, const AccessibilityTextChange& change ) override {
		return mApplication && mApplication->onTextChanged( mManager, ref, change );
	}

	void announce( const String& message, AccessibilityLive priority ) override {
		if ( mApplication )
			mApplication->announce( mManager, message, priority );
	}

  private:
	static std::weak_ptr<AtSpi::AtSpiApplication>& application() {
		static std::weak_ptr<AtSpi::AtSpiApplication> instance;
		return instance;
	}

	AccessibilityManager& mManager;
	std::shared_ptr<AtSpi::AtSpiApplication> mApplication;
};

} // namespace

std::unique_ptr<AccessibilityBackend> createAccessibilityBackend( AccessibilityManager& manager ) {
	return std::make_unique<AtSpiAccessibilityBackend>( manager );
}

}} // namespace EE::UI

#endif
