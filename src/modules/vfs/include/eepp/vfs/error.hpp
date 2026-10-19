#ifndef EE_VFS_ERROR_HPP
#define EE_VFS_ERROR_HPP

#include <eepp/core/result.hpp>
#include <eepp/core/task.hpp>
#include <eepp/core/uri.hpp>
#include <eepp/vfs/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace VFS {

/** Portable VFS error categories. High-level code branches on these, never on provider-native
 * codes (errno, libssh2, Win32). */
enum class VFSErrorCode : Uint8 {
	None,
	NotFound,
	AlreadyExists,
	NotDirectory,
	IsDirectory,
	PermissionDenied,
	ReadOnly,
	InvalidURI,
	Unsupported,
	InvalidOperation,
	NotEmpty,
	TooManySymlinks,
	ChangedExternally,
	ConnectionLost,
	AuthenticationFailed,
	HostKeyRejected,
	TimedOut,
	Cancelled,
	ProtocolError,
	NoSpace,
	IO,
	Unknown,
};

/** @return A stable, human readable identifier for code (for example "NotFound"). */
EE_VFS_API const char* toString( VFSErrorCode code );

/** @brief Typed VFS failure.
 *
 * nativeCode keeps the provider error (errno, Win32 error, SFTP status) for diagnostics only.
 */
struct EE_VFS_API VFSError {
	URI uri;
	std::string message;
	Int64 nativeCode{ 0 };
	VFSErrorCode code{ VFSErrorCode::None };

	VFSError() = default;

	explicit VFSError( VFSErrorCode code ) : code( code ) {}

	VFSError( VFSErrorCode code, URI uri, std::string message = {}, Int64 nativeCode = 0 ) :
		uri( std::move( uri ) ),
		message( std::move( message ) ),
		nativeCode( nativeCode ),
		code( code ) {}

	bool is( VFSErrorCode other ) const noexcept { return code == other; }

	/** @return "Code: message (uri)" for logs. */
	std::string toString() const;
};

template <typename T> using VFSResult = Result<T, VFSError>;

template <typename T> using VFSTask = Task<Result<T, VFSError>>;

/** @return A ready task holding an error. */
template <typename T> VFSTask<T> readyError( VFSError error ) {
	return makeReadyTask<VFSResult<T>>( Err( std::move( error ) ) );
}

template <typename T>
VFSTask<T> readyError( VFSErrorCode code, URI uri = {}, std::string message = {} ) {
	return readyError<T>( VFSError( code, std::move( uri ), std::move( message ) ) );
}

/** @return A ready task holding a success value. */
template <typename T, typename... Args> VFSTask<T> readyValue( Args&&... args ) {
	if constexpr ( std::is_void_v<T> )
		return makeReadyTask<VFSResult<void>>( Ok() );
	else
		return makeReadyTask<VFSResult<T>>( Ok( T( std::forward<Args>( args )... ) ) );
}

/** @return VFSError for a cancelled operation on uri. */
inline VFSError cancelledError( const URI& uri ) {
	return VFSError( VFSErrorCode::Cancelled, uri, "Operation cancelled" );
}

}} // namespace EE::VFS

#endif
