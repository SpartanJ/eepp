#ifndef EE_VFS_SFTP_SSHOPERATION_HPP
#define EE_VFS_SFTP_SSHOPERATION_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/error.hpp>
#include <functional>
#include <optional>

namespace EE { namespace VFS { namespace sftp {

class SSHConnection;

enum class Step : Uint8 {
	Done,
	/** Waiting for socket readiness (libssh2 returned EAGAIN) or an external event. */
	Again,
};

/** Unit of work queued on a connection and executed on the I/O thread.
 *
 * Operations of one connection run strictly one at a time, in submission order: libssh2 keeps
 * per-session and per-SFTP-session request state, so interleaving non-blocking calls of
 * different operations is not safe. Cancellation is honored before an operation starts and at
 * the safe points an operation checks itself; a libssh2 call is never abandoned midway.
 */
class SSHOperation {
  public:
	virtual ~SSHOperation() = default;

	virtual Step step( SSHConnection& connection ) = 0;

	virtual void fail( const VFSError& error ) = 0;
};

template <typename T> class SSHTaskOperation : public SSHOperation {
  public:
	using Function = std::function<std::optional<VFSResult<T>>( SSHConnection& )>;

	SSHTaskOperation( TaskResolver<VFSResult<T>> resolve, CancellationToken token, URI uri,
					  Function function ) :
		mResolve( std::move( resolve ) ),
		mToken( std::move( token ) ),
		mUri( std::move( uri ) ),
		mFunction( std::move( function ) ) {}

	Step step( SSHConnection& connection ) override {
		if ( !mStarted && mToken.isCancellationRequested() ) {
			mResolve( Err( cancelledError( mUri ) ) );
			return Step::Done;
		}
		mStarted = true;
		auto result = mFunction( connection );
		if ( !result )
			return Step::Again;
		mResolve( std::move( *result ) );
		return Step::Done;
	}

	void fail( const VFSError& error ) override {
		VFSError copy( error );
		if ( copy.uri.empty() )
			copy.uri = mUri;
		mResolve( Err( std::move( copy ) ) );
	}

  private:
	TaskResolver<VFSResult<T>> mResolve;
	CancellationToken mToken;
	URI mUri;
	Function mFunction;
	bool mStarted{ false };
};

}}} // namespace EE::VFS::sftp

#endif
