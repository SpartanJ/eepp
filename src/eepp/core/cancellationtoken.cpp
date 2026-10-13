#include <algorithm>
#include <eepp/core/cancellationtoken.hpp>
#include <eepp/core/executor.hpp>

namespace EE {

InlineExecutor& InlineExecutor::instance() {
	static InlineExecutor executor;
	return executor;
}

namespace detail {

bool CancellationState::cancel() {
	std::unique_lock<std::mutex> lock( mMutex );
	if ( mCancelled.load( std::memory_order_relaxed ) )
		return false;
	mCancelled.store( true, std::memory_order_release );
	mExecutingThread = std::this_thread::get_id();

	// Callbacks run outside the lock so they can register, unregister or cancel other sources.
	while ( !mEntries.empty() ) {
		Entry entry( std::move( mEntries.back() ) );
		mEntries.pop_back();
		mExecutingId = entry.id;
		lock.unlock();
		entry.callback();
		// Release captured state before signaling waiters, so a registration destructor that
		// returns can rely on the callback being fully gone.
		entry.callback.reset();
		lock.lock();
		mExecutingId = 0;
		mCallbackDone.notify_all();
	}

	mExecutingThread = {};
	return true;
}

Uint64 CancellationState::add( Callback& callback ) {
	std::lock_guard<std::mutex> lock( mMutex );
	if ( mCancelled.load( std::memory_order_relaxed ) )
		return 0;
	Uint64 id = mNextId++;
	mEntries.push_back( Entry{ id, std::move( callback ) } );
	return id;
}

void CancellationState::remove( Uint64 id ) {
	Callback removed;
	{
		std::unique_lock<std::mutex> lock( mMutex );
		auto it = std::find_if( mEntries.begin(), mEntries.end(),
								[id]( const Entry& entry ) { return entry.id == id; } );
		if ( it != mEntries.end() ) {
			removed = std::move( it->callback );
			mEntries.erase( it );
		} else if ( mExecutingId == id && mExecutingThread != std::this_thread::get_id() ) {
			mCallbackDone.wait( lock, [this, id] { return mExecutingId != id; } );
		}
	}
	// `removed` is destroyed outside the lock: its captures may run arbitrary destructors.
}

} // namespace detail

CancellationRegistration::CancellationRegistration( CancellationRegistration&& other ) noexcept :
	mState( std::move( other.mState ) ), mId( other.mId ) {
	other.mId = 0;
}

CancellationRegistration&
CancellationRegistration::operator=( CancellationRegistration&& other ) noexcept {
	if ( this != &other ) {
		reset();
		mState = std::move( other.mState );
		mId = other.mId;
		other.mId = 0;
	}
	return *this;
}

CancellationRegistration::~CancellationRegistration() {
	reset();
}

void CancellationRegistration::reset() {
	if ( mState && mId != 0 )
		mState->remove( mId );
	mId = 0;
	mState.reset();
}

CancellationRegistration
CancellationToken::onCancel( detail::CancellationState::Callback callback ) const {
	if ( !mState || !callback )
		return {};
	Uint64 id = mState->add( callback );
	if ( id == 0 ) {
		callback();
		return {};
	}
	return CancellationRegistration( mState, id );
}

CancellationSource::CancellationSource() :
	mState( std::make_shared<detail::CancellationState>() ) {}

bool CancellationSource::cancel() {
	return mState && mState->cancel();
}

} // namespace EE
