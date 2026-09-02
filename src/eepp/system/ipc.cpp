#include <eepp/system/ipc.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <eepp/system/md5.hpp>
#include <eepp/system/sys.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <aclapi.h>
#include <sddl.h>
#include <windows.h>
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace EE { namespace System {

namespace {

constexpr Uint32 IPCMagic = 0x45454950; // "EEIP"
constexpr Uint16 IPCVersion = 1;
constexpr Uint16 IPCFlags = 0;
constexpr std::size_t IPCHeaderSize = 12;
constexpr std::size_t IPCInlinePayloadSize = 512;
constexpr Uint32 IPCPipeBufferSize = 64 * 1024;
using EndpointHash = std::array<char, 33>;
using NativeName = std::array<wchar_t, 64>;

class MessageBuffer {
  public:
	explicit MessageBuffer( std::size_t size ) :
		mHeap( size > IPCInlinePayloadSize ? new Uint8[size] : nullptr ) {}

	Uint8* data() { return mHeap ? mHeap.get() : mInline.data(); }

  private:
	std::array<Uint8, IPCInlinePayloadSize> mInline;
	std::unique_ptr<Uint8[]> mHeap;
};

static_assert( sizeof( MessageBuffer ) <= IPCInlinePayloadSize + sizeof( void* ) );

std::array<Uint8, IPCHeaderSize> makeHeader( std::size_t payloadSize ) {
	std::array<Uint8, IPCHeaderSize> header{};
	const Uint32 size = static_cast<Uint32>( payloadSize );
	header[0] = static_cast<Uint8>( IPCMagic >> 24 );
	header[1] = static_cast<Uint8>( IPCMagic >> 16 );
	header[2] = static_cast<Uint8>( IPCMagic >> 8 );
	header[3] = static_cast<Uint8>( IPCMagic );
	header[4] = static_cast<Uint8>( IPCVersion >> 8 );
	header[5] = static_cast<Uint8>( IPCVersion );
	header[6] = static_cast<Uint8>( IPCFlags >> 8 );
	header[7] = static_cast<Uint8>( IPCFlags );
	header[8] = static_cast<Uint8>( size >> 24 );
	header[9] = static_cast<Uint8>( size >> 16 );
	header[10] = static_cast<Uint8>( size >> 8 );
	header[11] = static_cast<Uint8>( size );
	return header;
}

bool decodeHeader( const std::array<Uint8, IPCHeaderSize>& wire, Uint32& payloadSize ) {
	const Uint32 magic = static_cast<Uint32>( wire[0] ) << 24 |
						 static_cast<Uint32>( wire[1] ) << 16 |
						 static_cast<Uint32>( wire[2] ) << 8 | wire[3];
	const Uint16 version = static_cast<Uint16>( wire[4] ) << 8 | wire[5];
	const Uint16 flags = static_cast<Uint16>( wire[6] ) << 8 | wire[7];
	payloadSize = static_cast<Uint32>( wire[8] ) << 24 | static_cast<Uint32>( wire[9] ) << 16 |
				  static_cast<Uint32>( wire[10] ) << 8 | wire[11];
	return magic == IPCMagic && version == IPCVersion && flags == IPCFlags &&
		   payloadSize <= IPC::MaxMessageSize;
}

bool validEndpoint( std::string_view endpoint ) {
	return !endpoint.empty() && endpoint.size() <= 4096;
}

EndpointHash endpointHash( std::string_view endpoint ) {
	MD5::Context context;
	MD5::init( context );
	MD5::update( context, endpoint.data(), endpoint.size() );
	const MD5::Digest digest = MD5::result( context ).digest;
	constexpr char HexDigits[] = "0123456789abcdef";
	EndpointHash hash{};
	for ( std::size_t i = 0; i < digest.size(); ++i ) {
		hash[i * 2] = HexDigits[digest[i] >> 4];
		hash[i * 2 + 1] = HexDigits[digest[i] & 0x0F];
	}
	return hash;
}

} // namespace

class IPC::Impl {
  public:
	std::atomic<bool> running{ false };
	MessageFn callback;
	std::thread worker;
	std::mutex handlesMutex;

#if EE_PLATFORM == EE_PLATFORM_WIN
	HANDLE listener{ INVALID_HANDLE_VALUE };
	HANDLE ownership{ nullptr };
	NativeName nativeName{};
	PSECURITY_DESCRIPTOR securityDescriptor{ nullptr };
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	int listener{ -1 };
	int client{ -1 };
	sockaddr_un nativeAddress{};
#endif
};

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace {

NativeName nativeName( std::wstring_view prefix, const EndpointHash& hash ) {
	NativeName name{};
	if ( prefix.size() + hash.size() > name.size() )
		return name;
	std::copy( prefix.begin(), prefix.end(), name.begin() );
	for ( std::size_t i = 0; i < hash.size() - 1; ++i )
		name[prefix.size() + i] = static_cast<wchar_t>( hash[i] );
	return name;
}

HANDLE createPipe( const wchar_t* name, PSECURITY_DESCRIPTOR descriptor, bool first ) {
	SECURITY_ATTRIBUTES attributes{ sizeof( SECURITY_ATTRIBUTES ), descriptor, FALSE };
	return CreateNamedPipeW(
		name, PIPE_ACCESS_INBOUND | ( first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0 ),
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES, IPCPipeBufferSize, IPCPipeBufferSize, 0, &attributes );
}

bool readExact( HANDLE pipe, void* data, std::size_t size ) {
	auto* bytes = static_cast<Uint8*>( data );
	while ( size > 0 ) {
		DWORD read = 0;
		const DWORD chunk = static_cast<DWORD>( std::min<std::size_t>( size, MAXDWORD ) );
		if ( !ReadFile( pipe, bytes, chunk, &read, nullptr ) || read == 0 )
			return false;
		bytes += read;
		size -= read;
	}
	return true;
}

bool writeExact( HANDLE pipe, const void* data, std::size_t size ) {
	const auto* bytes = static_cast<const Uint8*>( data );
	while ( size > 0 ) {
		DWORD written = 0;
		const DWORD chunk = static_cast<DWORD>( std::min<std::size_t>( size, MAXDWORD ) );
		if ( !WriteFile( pipe, bytes, chunk, &written, nullptr ) || written == 0 )
			return false;
		bytes += written;
		size -= written;
	}
	return true;
}

bool endpointOwned( const wchar_t* ownershipName ) {
	HANDLE ownership = OpenMutexW( SYNCHRONIZE, FALSE, ownershipName );
	if ( ownership == nullptr )
		return false;
	CloseHandle( ownership );
	return true;
}

void cancelSynchronousIO( HANDLE thread ) {
	using CancelSynchronousIOFn = BOOL( WINAPI* )( HANDLE );
	static const auto cancel = reinterpret_cast<CancelSynchronousIOFn>(
		GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "CancelSynchronousIo" ) );
	if ( cancel != nullptr )
		cancel( thread );
}

IPC::Status windowsErrorStatus( DWORD error ) {
	if ( error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND )
		return IPC::Status::NotFound;
	if ( error == ERROR_PIPE_BUSY || error == ERROR_SEM_TIMEOUT )
		return IPC::Status::Timeout;
	if ( error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA )
		return IPC::Status::Disconnected;
	return IPC::Status::Error;
}

} // namespace

#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN

namespace {

const std::string& runtimeDirectory() {
	static const std::string directory = [] {
		std::string path = Sys::getTempPath();
		if ( path.empty() )
			return std::string{};
		if ( path.back() != '/' )
			path += '/';
		path += "eepp-ipc-" + std::to_string( static_cast<unsigned long long>( getuid() ) );
		if ( mkdir( path.c_str(), 0700 ) != 0 && errno != EEXIST )
			return std::string{};
		struct stat info;
		if ( lstat( path.c_str(), &info ) != 0 || !S_ISDIR( info.st_mode ) ||
			 info.st_uid != getuid() || ( info.st_mode & 077 ) != 0 )
			return std::string{};
		return path;
	}();
	return directory;
}

bool makeAddress( std::string_view endpoint, sockaddr_un& address ) {
	const std::string& directory = runtimeDirectory();
	if ( directory.empty() )
		return false;
	const EndpointHash hash = endpointHash( endpoint );
	const std::size_t pathSize = directory.size() + 1 + hash.size() - 1;
	if ( pathSize >= sizeof( address.sun_path ) )
		return false;
	std::memset( &address, 0, sizeof( address ) );
	address.sun_family = AF_UNIX;
	std::memcpy( address.sun_path, directory.data(), directory.size() );
	address.sun_path[directory.size()] = '/';
	std::memcpy( address.sun_path + directory.size() + 1, hash.data(), hash.size() - 1 );
	return true;
}

bool readExact( int fd, void* data, std::size_t size ) {
	auto* bytes = static_cast<Uint8*>( data );
	while ( size > 0 ) {
		const ssize_t readSize = recv( fd, bytes, size, 0 );
		if ( readSize == 0 )
			return false;
		if ( readSize < 0 ) {
			if ( errno == EINTR )
				continue;
			return false;
		}
		bytes += readSize;
		size -= static_cast<std::size_t>( readSize );
	}
	return true;
}

bool writeExact( int fd, const void* data, std::size_t size ) {
	const auto* bytes = static_cast<const Uint8*>( data );
	while ( size > 0 ) {
#ifdef MSG_NOSIGNAL
		const ssize_t written = send( fd, bytes, size, MSG_NOSIGNAL );
#else
		const ssize_t written = send( fd, bytes, size, 0 );
#endif
		if ( written < 0 ) {
			if ( errno == EINTR )
				continue;
			return false;
		}
		if ( written == 0 )
			return false;
		bytes += written;
		size -= static_cast<std::size_t>( written );
	}
	return true;
}

IPC::Status connectSocket( const sockaddr_un& address, Time timeout, int& fd ) {
	fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( fd < 0 )
		return IPC::Status::Error;

	const int oldFlags = fcntl( fd, F_GETFL, 0 );
	if ( oldFlags >= 0 )
		fcntl( fd, F_SETFL, oldFlags | O_NONBLOCK );
	if ( connect( fd, reinterpret_cast<const sockaddr*>( &address ), sizeof( address ) ) != 0 ) {
		if ( errno != EINPROGRESS ) {
			const int error = errno;
			::close( fd );
			fd = -1;
			return error == ENOENT || error == ECONNREFUSED ? IPC::Status::NotFound
															: IPC::Status::Error;
		}
		pollfd wait{ fd, POLLOUT, 0 };
		const int timeoutMs = static_cast<int>( eemax<double>( 0, timeout.asMilliseconds() ) );
		int result;
		do {
			result = poll( &wait, 1, timeoutMs );
		} while ( result < 0 && errno == EINTR );
		if ( result <= 0 ) {
			::close( fd );
			fd = -1;
			return result == 0 ? IPC::Status::Timeout : IPC::Status::Error;
		}
		int error = 0;
		socklen_t errorSize = sizeof( error );
		if ( getsockopt( fd, SOL_SOCKET, SO_ERROR, &error, &errorSize ) != 0 || error != 0 ) {
			::close( fd );
			fd = -1;
			return error == ENOENT || error == ECONNREFUSED ? IPC::Status::NotFound
															: IPC::Status::Error;
		}
	}
	if ( oldFlags >= 0 )
		fcntl( fd, F_SETFL, oldFlags );
	return IPC::Status::Done;
}

} // namespace

#endif

IPC::IPC() : mImpl( std::make_unique<Impl>() ) {}

IPC::~IPC() {
	close();
}

IPC::Status IPC::listen( std::string_view endpoint, MessageFn callback ) {
	if ( !validEndpoint( endpoint ) || !callback )
		return Status::InvalidEndpoint;
	close();

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return Status::Error;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	const EndpointHash hash = endpointHash( endpoint );
	const NativeName pipe = nativeName( L"\\\\.\\pipe\\eepp-", hash );
	const NativeName ownership = nativeName( L"Local\\eepp-ipc-owner-", hash );
	mImpl->nativeName = pipe;
	if ( !ConvertStringSecurityDescriptorToSecurityDescriptorW(
			 L"D:P(A;;GA;;;OW)", SDDL_REVISION_1, &mImpl->securityDescriptor, nullptr ) )
		return Status::Error;
	SECURITY_ATTRIBUTES attributes{ sizeof( SECURITY_ATTRIBUTES ), mImpl->securityDescriptor,
									FALSE };
	mImpl->ownership = CreateMutexW( &attributes, FALSE, ownership.data() );
	if ( mImpl->ownership == nullptr || GetLastError() == ERROR_ALREADY_EXISTS ) {
		if ( mImpl->ownership != nullptr )
			CloseHandle( mImpl->ownership );
		mImpl->ownership = nullptr;
		LocalFree( mImpl->securityDescriptor );
		mImpl->securityDescriptor = nullptr;
		return Status::AlreadyExists;
	}
	mImpl->listener = createPipe( mImpl->nativeName.data(), mImpl->securityDescriptor, true );
	if ( mImpl->listener == INVALID_HANDLE_VALUE ) {
		const DWORD error = GetLastError();
		CloseHandle( mImpl->ownership );
		mImpl->ownership = nullptr;
		LocalFree( mImpl->securityDescriptor );
		mImpl->securityDescriptor = nullptr;
		return error == ERROR_ACCESS_DENIED || error == ERROR_PIPE_BUSY ||
					   error == ERROR_ALREADY_EXISTS
				   ? Status::AlreadyExists
				   : Status::Error;
	}
	mImpl->callback = std::move( callback );
	mImpl->running = true;
	mImpl->worker = std::thread( [impl = mImpl.get()] {
		while ( impl->running ) {
			HANDLE current;
			{
				std::lock_guard<std::mutex> lock( impl->handlesMutex );
				current = impl->listener;
			}
			const BOOL connected = ConnectNamedPipe( current, nullptr )
									   ? TRUE
									   : GetLastError() == ERROR_PIPE_CONNECTED;
			if ( !connected ) {
				if ( !impl->running )
					break;
				continue;
			}
			if ( !impl->running )
				break;
			std::array<Uint8, IPCHeaderSize> header;
			Uint32 payloadSize = 0;
			if ( readExact( current, header.data(), header.size() ) &&
				 decodeHeader( header, payloadSize ) ) {
				MessageBuffer payload( payloadSize );
				if ( readExact( current, payload.data(), payloadSize ) && impl->running ) {
					const Uint8 empty = 0;
					impl->callback( payloadSize == 0 ? &empty : payload.data(), payloadSize );
				}
			}
			CloseHandle( current );
			HANDLE replacement = INVALID_HANDLE_VALUE;
			if ( impl->running )
				replacement =
					createPipe( impl->nativeName.data(), impl->securityDescriptor, false );
			{
				std::lock_guard<std::mutex> lock( impl->handlesMutex );
				impl->listener = replacement;
			}
			if ( replacement == INVALID_HANDLE_VALUE )
				break;
		}
	} );
	return Status::Done;
#else
	if ( !makeAddress( endpoint, mImpl->nativeAddress ) )
		return Status::Error;
	mImpl->listener = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( mImpl->listener < 0 )
		return Status::Error;
	if ( bind( mImpl->listener, reinterpret_cast<sockaddr*>( &mImpl->nativeAddress ),
			   sizeof( mImpl->nativeAddress ) ) != 0 ) {
		if ( errno != EADDRINUSE ) {
			::close( mImpl->listener );
			mImpl->listener = -1;
			return Status::Error;
		}
		int probe = -1;
		const Status probeStatus =
			connectSocket( mImpl->nativeAddress, Milliseconds( 100 ), probe );
		if ( probeStatus == Status::Done ) {
			::close( probe );
			::close( mImpl->listener );
			mImpl->listener = -1;
			return Status::AlreadyExists;
		}
		if ( probeStatus != Status::NotFound || unlink( mImpl->nativeAddress.sun_path ) != 0 ||
			 bind( mImpl->listener, reinterpret_cast<sockaddr*>( &mImpl->nativeAddress ),
				   sizeof( mImpl->nativeAddress ) ) != 0 ) {
			::close( mImpl->listener );
			mImpl->listener = -1;
			return Status::Error;
		}
	}
	chmod( mImpl->nativeAddress.sun_path, 0600 );
	if ( ::listen( mImpl->listener, SOMAXCONN ) != 0 ) {
		::close( mImpl->listener );
		mImpl->listener = -1;
		unlink( mImpl->nativeAddress.sun_path );
		return Status::Error;
	}
	mImpl->callback = std::move( callback );
	mImpl->running = true;
	mImpl->worker = std::thread( [impl = mImpl.get()] {
		while ( impl->running ) {
			const int client = accept( impl->listener, nullptr, nullptr );
			if ( client < 0 ) {
				if ( !impl->running )
					break;
				if ( errno == EINTR )
					continue;
				break;
			}
			{
				std::lock_guard<std::mutex> lock( impl->handlesMutex );
				impl->client = client;
			}
			if ( !impl->running ) {
				{
					std::lock_guard<std::mutex> lock( impl->handlesMutex );
					impl->client = -1;
				}
				::close( client );
				break;
			}
			std::array<Uint8, IPCHeaderSize> header;
			Uint32 payloadSize = 0;
			if ( readExact( client, header.data(), header.size() ) &&
				 decodeHeader( header, payloadSize ) ) {
				MessageBuffer payload( payloadSize );
				if ( readExact( client, payload.data(), payloadSize ) && impl->running ) {
					const Uint8 empty = 0;
					impl->callback( payloadSize == 0 ? &empty : payload.data(), payloadSize );
				}
			}
			{
				std::lock_guard<std::mutex> lock( impl->handlesMutex );
				if ( impl->client == client )
					impl->client = -1;
			}
			::close( client );
		}
	} );
	return Status::Done;
#endif
}

void IPC::close() {
	if ( !mImpl->running.exchange( false ) ) {
		if ( mImpl->worker.joinable() )
			mImpl->worker.join();
		return;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	{
		std::lock_guard<std::mutex> lock( mImpl->handlesMutex );
		cancelSynchronousIO( reinterpret_cast<HANDLE>( mImpl->worker.native_handle() ) );
		HANDLE wake = CreateFileW( mImpl->nativeName.data(), GENERIC_WRITE, 0, nullptr,
								   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
		if ( wake != INVALID_HANDLE_VALUE )
			CloseHandle( wake );
	}
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	{
		std::lock_guard<std::mutex> lock( mImpl->handlesMutex );
		if ( mImpl->client >= 0 )
			shutdown( mImpl->client, SHUT_RDWR );
	}
	if ( mImpl->listener >= 0 ) {
		shutdown( mImpl->listener, SHUT_RDWR );
		::close( mImpl->listener );
		mImpl->listener = -1;
	}
#endif
	if ( mImpl->worker.joinable() )
		mImpl->worker.join();
#if EE_PLATFORM == EE_PLATFORM_WIN
	{
		std::lock_guard<std::mutex> lock( mImpl->handlesMutex );
		if ( mImpl->listener != INVALID_HANDLE_VALUE ) {
			DisconnectNamedPipe( mImpl->listener );
			CloseHandle( mImpl->listener );
			mImpl->listener = INVALID_HANDLE_VALUE;
		}
	}
	if ( mImpl->securityDescriptor != nullptr ) {
		LocalFree( mImpl->securityDescriptor );
		mImpl->securityDescriptor = nullptr;
	}
	if ( mImpl->ownership != nullptr ) {
		CloseHandle( mImpl->ownership );
		mImpl->ownership = nullptr;
	}
#endif
#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( mImpl->nativeAddress.sun_path[0] != '\0' )
		unlink( mImpl->nativeAddress.sun_path );
#endif
	mImpl->callback = {};
}

bool IPC::isListening() const {
	return mImpl->running;
}

IPC::Status IPC::send( std::string_view endpoint, const void* data, std::size_t size,
					   Time timeout ) {
	if ( !validEndpoint( endpoint ) || ( size > 0 && data == nullptr ) )
		return Status::InvalidEndpoint;
	if ( size > MaxMessageSize )
		return Status::MessageTooLarge;
	const auto header = makeHeader( size );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return Status::Error;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	const EndpointHash hash = endpointHash( endpoint );
	const NativeName name = nativeName( L"\\\\.\\pipe\\eepp-", hash );
	const NativeName ownership = nativeName( L"Local\\eepp-ipc-owner-", hash );
	const Uint64 timeoutMs = static_cast<Uint64>( eemax<double>( 0, timeout.asMilliseconds() ) );
	const Uint64 startTime = Sys::getTicks();
	HANDLE pipe;
	while ( true ) {
		pipe = CreateFileW( name.data(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL, nullptr );
		if ( pipe != INVALID_HANDLE_VALUE )
			break;
		const DWORD error = GetLastError();
		const bool transitioning =
			error == ERROR_FILE_NOT_FOUND && endpointOwned( ownership.data() );
		if ( error != ERROR_PIPE_BUSY && error != ERROR_SEM_TIMEOUT && !transitioning )
			return windowsErrorStatus( error );
		if ( Sys::getTicks() - startTime >= timeoutMs )
			return Status::Timeout;
		Sleep( 1 );
	}
	const bool written = writeExact( pipe, header.data(), header.size() ) &&
						 ( size == 0 || writeExact( pipe, data, size ) );
	if ( written )
		FlushFileBuffers( pipe );
	CloseHandle( pipe );
	return written ? Status::Done : Status::Disconnected;
#else
	sockaddr_un address;
	if ( !makeAddress( endpoint, address ) )
		return Status::Error;
	int fd = -1;
	const Status connected = connectSocket( address, timeout, fd );
	if ( connected != Status::Done )
		return connected;
	const bool written = writeExact( fd, header.data(), header.size() ) &&
						 ( size == 0 || writeExact( fd, data, size ) );
	::close( fd );
	return written ? Status::Done : Status::Disconnected;
#endif
}

IPC::Status IPC::send( std::string_view endpoint, std::string_view message, Time timeout ) {
	return send( endpoint, message.data(), message.size(), timeout );
}

}} // namespace EE::System
