#ifdef EE_DBUS_TRANSPORT

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <eepp/system/dbus/dbus.hpp>
#include <eepp/system/dbus/dbuslibrary.hpp>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <utility>

namespace EE::System::Dbus {

static_assert( sizeof( Native::DBusError ) == sizeof( ::DBusError ) &&
			   alignof( Native::DBusError ) == alignof( ::DBusError ) );
static_assert( sizeof( Native::DBusMessageIter ) == sizeof( ::DBusMessageIter ) &&
			   alignof( Native::DBusMessageIter ) == alignof( ::DBusMessageIter ) );
static_assert( sizeof( Native::DBusObjectPathVTable ) == sizeof( ::DBusObjectPathVTable ) &&
			   alignof( Native::DBusObjectPathVTable ) == alignof( ::DBusObjectPathVTable ) );

class DbusTransportLibrary {
  public:
	static const DbusTransportLibrary* load() {
		static const DbusTransportLibrary library;
		return library.mAvailable ? &library : nullptr;
	}

#define DBUS_SYMBOLS( X )                       \
	X( bus_get_private )                        \
	X( bus_get_unique_name )                    \
	X( connection_set_exit_on_disconnect )      \
	X( connection_set_watch_functions )         \
	X( connection_set_timeout_functions )       \
	X( connection_set_max_message_size )        \
	X( connection_set_max_received_size )       \
	X( connection_add_filter )                  \
	X( connection_close )                       \
	X( connection_unref )                       \
	X( connection_get_is_connected )            \
	X( connection_send_with_reply )             \
	X( connection_get_dispatch_status )         \
	X( connection_dispatch )                    \
	X( watch_get_enabled )                      \
	X( watch_get_unix_fd )                      \
	X( watch_get_flags )                        \
	X( watch_handle )                           \
	X( timeout_get_enabled )                    \
	X( timeout_get_interval )                   \
	X( timeout_handle )                         \
	X( pending_call_get_completed )             \
	X( pending_call_steal_reply )               \
	X( pending_call_cancel )                    \
	X( pending_call_unref )                     \
	X( message_new_method_call )                \
	X( message_ref )                            \
	X( message_unref )                          \
	X( message_get_type )                       \
	X( message_get_sender )                     \
	X( message_get_path )                       \
	X( message_get_error_name )                 \
	X( message_has_signature )                  \
	X( message_is_signal )                      \
	X( message_iter_init )                      \
	X( message_iter_init_append )               \
	X( message_iter_get_arg_type )              \
	X( message_iter_get_element_type )          \
	X( message_iter_recurse )                   \
	X( message_iter_next )                      \
	X( message_iter_get_basic )                 \
	X( message_iter_append_basic )              \
	X( message_iter_open_container )            \
	X( message_iter_close_container )           \
	X( message_iter_abandon_container_if_open ) \
	X( validate_path )                          \
	X( validate_utf8 )
#define DECLARE( name ) decltype( &dbus_##name ) name{ nullptr };
	DBUS_SYMBOLS( DECLARE )
#undef DECLARE

  private:
	DbusTransportLibrary() : mAvailable( open() ) {}

	bool open() {
		void* handle = DbusLibrary::loadHandle();
		if ( !handle )
			return false;
#define LOAD( name )                                               \
	if ( !DbusLibrary::loadSymbol( handle, name, "dbus_" #name ) ) \
		return false;
		DBUS_SYMBOLS( LOAD )
#undef LOAD
		return true;
	}

	bool mAvailable;
};

#undef DBUS_SYMBOLS

DbusReader DbusReader::child( int expected ) const {
	DbusReader child( *mLibrary );
	if ( type() == expected ) {
		mLibrary->message_iter_recurse( &mIter, &child.mIter );
		child.mValid = true;
	}
	return child;
}

bool DbusReader::next() {
	return mValid && mLibrary->message_iter_next( &mIter );
}

int DbusReader::type() const {
	return mValid ? mLibrary->message_iter_get_arg_type( &mIter ) : DBUS_TYPE_INVALID;
}

int DbusReader::elementType() const {
	return type() == DBUS_TYPE_ARRAY ? mLibrary->message_iter_get_element_type( &mIter )
									 : DBUS_TYPE_INVALID;
}

const char* DbusReader::string( int expected ) const {
	const char* value = nullptr;
	if ( type() == expected )
		mLibrary->message_iter_get_basic( &mIter, &value );
	return value;
}

bool DbusReader::read( Uint32& value ) const {
	if ( type() != DBUS_TYPE_UINT32 )
		return false;
	mLibrary->message_iter_get_basic( &mIter, &value );
	return true;
}

bool DbusWriter::string( const char* value, int type ) {
	if ( type != DBUS_TYPE_STRING && type != DBUS_TYPE_OBJECT_PATH )
		return false;
	if ( !value || ( type == DBUS_TYPE_OBJECT_PATH ? !mLibrary->validate_path( value, nullptr )
												   : !mLibrary->validate_utf8( value, nullptr ) ) )
		return false;
	return mLibrary->message_iter_append_basic( &mIter, type, &value );
}

bool DbusWriter::write( Uint32 value ) {
	return mLibrary->message_iter_append_basic( &mIter, DBUS_TYPE_UINT32, &value );
}

bool DbusWriter::open( DbusWriter& child, int type, const char* signature ) {
	return mLibrary->message_iter_open_container( &mIter, type, signature, &child.mIter );
}

bool DbusWriter::close( DbusWriter& child ) {
	return mLibrary->message_iter_close_container( &mIter, &child.mIter );
}

void DbusWriter::abandon( DbusWriter& child ) {
	mLibrary->message_iter_abandon_container_if_open( &mIter, &child.mIter );
}

DbusMessage::DbusMessage( const DbusTransportLibrary* library, DBusMessage* message ) :
	mLibrary( library ), mMessage( message ) {}

DbusMessage::~DbusMessage() {
	if ( mMessage )
		mLibrary->message_unref( mMessage );
}

DbusMessage::DbusMessage( DbusMessage&& other ) noexcept :
	mLibrary( other.mLibrary ), mMessage( std::exchange( other.mMessage, nullptr ) ) {}

DbusMessage& DbusMessage::operator=( DbusMessage&& other ) noexcept {
	DbusMessage old( std::move( other ) );
	std::swap( mLibrary, old.mLibrary );
	std::swap( mMessage, old.mMessage );
	return *this;
}

DbusReader DbusMessage::read() const {
	DbusReader reader( *mLibrary );
	reader.mValid = mLibrary->message_iter_init( mMessage, &reader.mIter );
	return reader;
}

DbusWriter DbusMessage::write() {
	DbusWriter writer( *mLibrary );
	mLibrary->message_iter_init_append( mMessage, &writer.mIter );
	return writer;
}

std::string_view DbusMessage::sender() const {
	const auto* value = mLibrary->message_get_sender( mMessage );
	return value ? value : "";
}

std::string_view DbusMessage::path() const {
	const auto* value = mLibrary->message_get_path( mMessage );
	return value ? value : "";
}

std::string_view DbusMessage::error() const {
	const auto* value = mLibrary->message_get_error_name( mMessage );
	return value ? value : "";
}

bool DbusMessage::signature( const char* value ) const {
	return mLibrary->message_has_signature( mMessage, value );
}

bool DbusMessage::signal( const char* interface, const char* member ) const {
	return mLibrary->message_is_signal( mMessage, interface, member );
}

struct DbusConnection::Impl {
	using Clock = std::chrono::steady_clock;

	struct Watch {
		DBusWatch* value{ nullptr };
		Uint64 generation{ 0 };
	};

	struct Timeout {
		DBusTimeout* value{ nullptr };
		Clock::time_point due{};
	};

	static dbus_bool_t addWatch( DBusWatch* watch, void* data ) {
		auto& self = *static_cast<Impl*>( data );
		for ( auto& entry : self.watches ) {
			if ( !entry.value ) {
				entry = { watch, ++self.generation };
				return true;
			}
		}
		return false;
	}

	static void removeWatch( DBusWatch* watch, void* data ) {
		for ( auto& entry : static_cast<Impl*>( data )->watches ) {
			if ( entry.value == watch )
				entry = {};
		}
	}

	static void toggleWatch( DBusWatch* watch, void* data ) {
		auto& self = *static_cast<Impl*>( data );
		for ( auto& entry : self.watches ) {
			if ( entry.value == watch ) {
				entry.generation = ++self.generation;
				return;
			}
		}
	}

	static dbus_bool_t addTimeout( DBusTimeout* timeout, void* data ) {
		auto& self = *static_cast<Impl*>( data );
		for ( auto& entry : self.timeouts ) {
			if ( !entry.value ) {
				entry.value = timeout;
				toggleTimeout( timeout, data );
				return true;
			}
		}
		return false;
	}

	static void removeTimeout( DBusTimeout* timeout, void* data ) {
		for ( auto& entry : static_cast<Impl*>( data )->timeouts ) {
			if ( entry.value == timeout )
				entry = {};
		}
	}

	static void toggleTimeout( DBusTimeout* timeout, void* data ) {
		auto& self = *static_cast<Impl*>( data );
		for ( auto& entry : self.timeouts ) {
			if ( entry.value == timeout ) {
				entry.due = self.library->timeout_get_enabled( timeout )
								? Clock::now() + std::chrono::milliseconds(
													 self.library->timeout_get_interval( timeout ) )
								: Clock::time_point::max();
				return;
			}
		}
	}

	static DBusHandlerResult filter( DBusConnection*, DBusMessage* message, void* data ) {
		auto& self = *static_cast<Impl*>( data );
		if ( self.library->message_get_type( message ) != DBUS_MESSAGE_TYPE_SIGNAL )
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		self.signal = DbusMessage( self.library, self.library->message_ref( message ) );
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	bool openPipe();

	void drainPipe();

	bool handleTimeouts();

	const DbusTransportLibrary* library{ nullptr };
	DBusConnection* connection{ nullptr };
	DBusPendingCall* pending{ nullptr };
	DbusMessage signal;
	std::array<Watch, 8> watches{};
	std::array<Timeout, 8> timeouts{};
	Uint64 generation{ 0 };
	std::atomic<int> writeFd{ -1 };
	std::atomic<bool> woken{ false };
	int readFd{ -1 };
};

bool DbusConnection::Impl::openPipe() {
	if ( readFd >= 0 )
		return true;
	int fds[2];
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( pipe2( fds, O_NONBLOCK | O_CLOEXEC ) < 0 )
		return false;
#else
	if ( pipe( fds ) < 0 )
		return false;
	for ( int fd : fds ) {
		if ( fcntl( fd, F_SETFD, FD_CLOEXEC ) < 0 || fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 ) {
			::close( fds[0] );
			::close( fds[1] );
			return false;
		}
	}
#endif
	readFd = fds[0];
	writeFd.store( fds[1] );
	return true;
}

void DbusConnection::Impl::drainPipe() {
	if ( readFd < 0 )
		return;
	char bytes[128];
	ssize_t count;
	do {
		count = read( readFd, bytes, sizeof( bytes ) );
	} while ( count > 0 || ( count < 0 && errno == EINTR ) );
}

bool DbusConnection::Impl::handleTimeouts() {
	const auto now = Clock::now();
	for ( auto& timeout : timeouts ) {
		if ( timeout.value && timeout.due <= now ) {
			auto* value = timeout.value;
			toggleTimeout( value, this );
			if ( !library->timeout_handle( value ) )
				return false;
		}
	}
	return true;
}

DbusConnection::DbusConnection() : mImpl( std::make_unique<Impl>() ) {}

DbusConnection::~DbusConnection() {
	close();
	if ( mImpl->readFd >= 0 ) {
		::close( mImpl->readFd );
		::close( mImpl->writeFd.load() );
	}
}

bool DbusConnection::open() {
	auto& self = *mImpl;
	if ( self.connection )
		return connected();
	if ( !self.openPipe() )
		return false;
	self.library = DbusTransportLibrary::load();
	if ( !self.library )
		return false;
	auto& dbus = *self.library;
	self.connection = dbus.bus_get_private( DBUS_BUS_SESSION, nullptr );
	if ( !self.connection )
		return false;
	dbus.connection_set_exit_on_disconnect( self.connection, false );
	dbus.connection_set_max_message_size( self.connection, 1024 * 1024 );
	dbus.connection_set_max_received_size( self.connection, 4 * 1024 * 1024 );
	if ( dbus.connection_set_watch_functions( self.connection, Impl::addWatch, Impl::removeWatch,
											  Impl::toggleWatch, &self, nullptr ) &&
		 dbus.connection_set_timeout_functions( self.connection, Impl::addTimeout,
												Impl::removeTimeout, Impl::toggleTimeout, &self,
												nullptr ) &&
		 dbus.connection_add_filter( self.connection, Impl::filter, &self, nullptr ) )
		return true;
	close();
	return false;
}

void DbusConnection::close() {
	auto& self = *mImpl;
	cancel();
	self.signal = {};
	if ( auto* connection = std::exchange( self.connection, nullptr ) ) {
		auto& dbus = *self.library;
		dbus.connection_close( connection );
		dbus.connection_unref( connection );
		self.watches = {};
		self.timeouts = {};
	}
}

bool DbusConnection::connected() const {
	return mImpl->connection && mImpl->library->connection_get_is_connected( mImpl->connection );
}

std::string_view DbusConnection::uniqueName() const {
	const auto* name = mImpl->library->bus_get_unique_name( mImpl->connection );
	return name ? name : "";
}

DbusMessage DbusConnection::method( const char* destination, const char* path,
									const char* interface, const char* member ) const {
	return DbusMessage( mImpl->library, mImpl->library->message_new_method_call(
											destination, path, interface, member ) );
}

bool DbusConnection::call( DbusMessage message, int milliseconds ) {
	return message && !mImpl->pending && connected() &&
		   mImpl->library->connection_send_with_reply( mImpl->connection, message.mMessage,
													   &mImpl->pending, milliseconds ) &&
		   mImpl->pending;
}

void DbusConnection::cancel() {
	auto& self = *mImpl;
	if ( auto* pending = std::exchange( self.pending, nullptr ) ) {
		self.library->pending_call_cancel( pending );
		self.library->pending_call_unref( pending );
	}
}

DbusMessage DbusConnection::reply() {
	auto& self = *mImpl;
	if ( !self.pending || !self.library->pending_call_get_completed( self.pending ) )
		return {};
	auto* pending = std::exchange( self.pending, nullptr );
	DbusMessage result( self.library, self.library->pending_call_steal_reply( pending ) );
	self.library->pending_call_unref( pending );
	return result;
}

DbusMessage DbusConnection::signal() {
	return std::move( mImpl->signal );
}

void DbusConnection::wake() noexcept {
	mImpl->woken.store( true );
	mImpl->woken.notify_one();
	const int fd = mImpl->writeFd.load();
	if ( fd >= 0 ) {
		const char byte = 1;
		while ( ::write( fd, &byte, 1 ) < 0 && errno == EINTR ) {
		}
	}
}

DbusConnection::Step DbusConnection::wait() {
	auto& self = *mImpl;
	if ( self.woken.exchange( false ) ) {
		self.drainPipe();
		return Step::Idle;
	}
	if ( !self.connection ) {
		self.woken.wait( false );
		return Step::Idle;
	}
	auto& dbus = *self.library;
	if ( !self.handleTimeouts() )
		return Step::Failed;
	const auto dispatch = dbus.connection_get_dispatch_status( self.connection );
	if ( dispatch == DBUS_DISPATCH_NEED_MEMORY )
		return Step::Failed;
	if ( dispatch == DBUS_DISPATCH_DATA_REMAINS ) {
		return dbus.connection_dispatch( self.connection ) == DBUS_DISPATCH_NEED_MEMORY
				   ? Step::Failed
				   : Step::Ready;
	}
	if ( !connected() )
		return Step::Failed;

	std::array<pollfd, 9> fds{};
	std::array<Uint64, 8> generations{};
	fds[0] = { self.readFd, POLLIN, 0 };
	for ( size_t i = 0; i < self.watches.size(); ++i ) {
		const auto& watch = self.watches[i];
		fds[i + 1].fd = -1;
		if ( watch.value && dbus.watch_get_enabled( watch.value ) ) {
			const auto flags = dbus.watch_get_flags( watch.value );
			fds[i + 1] = { dbus.watch_get_unix_fd( watch.value ),
						   static_cast<short>( ( flags & DBUS_WATCH_READABLE ? POLLIN : 0 ) |
											   ( flags & DBUS_WATCH_WRITABLE ? POLLOUT : 0 ) ),
						   0 };
			generations[i] = watch.generation;
		}
	}
	const auto now = Impl::Clock::now();
	auto due = Impl::Clock::time_point::max();
	for ( const auto& timeout : self.timeouts ) {
		if ( timeout.value )
			due = std::min( due, timeout.due );
	}
	const auto delay =
		due == Impl::Clock::time_point::max()
			? -1
			: static_cast<int>( std::max<int64_t>(
				  0, std::chrono::ceil<std::chrono::milliseconds>( due - now ).count() ) );
	if ( poll( fds.data(), fds.size(), delay ) < 0 )
		return errno == EINTR ? Step::Idle : Step::Failed;
	if ( fds.front().revents & ( POLLERR | POLLHUP | POLLNVAL ) )
		return Step::Failed;
	if ( fds.front().revents & POLLIN ) {
		self.drainPipe();
		return Step::Idle;
	}
	for ( size_t i = 0; i < self.watches.size(); ++i ) {
		const auto events = fds[i + 1].revents;
		const auto& watch = self.watches[i];
		if ( !events || !watch.value || generations[i] != watch.generation ||
			 !dbus.watch_get_enabled( watch.value ) )
			continue;
		const auto flags = ( events & POLLIN ? DBUS_WATCH_READABLE : 0 ) |
						   ( events & POLLOUT ? DBUS_WATCH_WRITABLE : 0 ) |
						   ( events & ( POLLERR | POLLNVAL ) ? DBUS_WATCH_ERROR : 0 ) |
						   ( events & POLLHUP ? DBUS_WATCH_HANGUP : 0 );
		if ( !dbus.watch_handle( watch.value, flags ) )
			return Step::Failed;
	}
	return self.handleTimeouts() ? Step::Ready : Step::Failed;
}

} // namespace EE::System::Dbus

#endif
