#include <eepp/vfs/detail/transferstate.hpp>
#include <eepp/vfs/transfer.hpp>

namespace EE { namespace VFS {

TransferProgressSubscription&
TransferProgressSubscription::operator=( TransferProgressSubscription&& other ) noexcept {
	if ( this != &other ) {
		reset();
		mState = std::move( other.mState );
		mId = other.mId;
		other.mId = 0;
	}
	return *this;
}

TransferProgressSubscription::~TransferProgressSubscription() {
	reset();
}

void TransferProgressSubscription::reset() {
	if ( mId != 0 ) {
		if ( auto state = mState.lock() )
			state->unsubscribe( mId );
	}
	mId = 0;
	mState.reset();
}

Transfer::Transfer( std::shared_ptr<detail::TransferState> state ) : mState( std::move( state ) ) {}

VFSTask<TransferResult> Transfer::completion() {
	if ( !mState ) {
		return readyError<TransferResult>( VFSErrorCode::InvalidOperation, URI(),
										   "Invalid transfer" );
	}
	return mState->takeCompletion();
}

TransferProgressSubscription Transfer::onProgress( TransferProgressCallback callback,
												   Executor* deliveryExecutor ) {
	if ( !mState || !callback )
		return {};
	Uint64 id = mState->subscribe( std::move( callback ), deliveryExecutor );
	return TransferProgressSubscription( mState, id );
}

void Transfer::cancel() {
	if ( mState )
		mState->cancel();
}

CancellationToken Transfer::token() const {
	return mState ? mState->token() : CancellationToken();
}

}} // namespace EE::VFS
