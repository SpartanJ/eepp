#include <condition_variable>
#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/scene/node.hpp>
#include <mutex>
#include <vector>

namespace EE { namespace Scene {

struct MainThreadExecutor::State {
	std::mutex mutex;
	std::condition_variable scheduledOnNode;
	std::vector<Function> queue;
	Function wake;
	Node* dispatcher{ nullptr };
	size_t scheduling{ 0 };
	bool scheduled{ false };

	struct SchedulingLease {
		State& state;

		~SchedulingLease() {
			std::lock_guard<std::mutex> lock( state.mutex );
			if ( --state.scheduling == 0 )
				state.scheduledOnNode.notify_all();
		}
	};

	static void drain( const std::shared_ptr<State>& state ) {
		std::vector<Function> functions;
		{
			std::lock_guard<std::mutex> lock( state->mutex );
			functions.swap( state->queue );
			state->scheduled = false;
		}
		for ( auto& function : functions )
			function();
	}
};

MainThreadExecutor::MainThreadExecutor( Node* dispatcher ) : MainThreadExecutor( dispatcher, {} ) {}

MainThreadExecutor::MainThreadExecutor( Node* dispatcher, Function wake ) :
	mState( std::make_shared<State>() ) {
	mState->dispatcher = dispatcher;
	mState->wake = std::move( wake );
}

MainThreadExecutor::~MainThreadExecutor() {
	shutdown();
}

void MainThreadExecutor::shutdown() {
	std::vector<Function> dropped;
	{
		std::unique_lock<std::mutex> lock( mState->mutex );
		mState->dispatcher = nullptr;
		dropped.swap( mState->queue );
		mState->scheduledOnNode.wait( lock, [this] { return mState->scheduling == 0; } );
	}
	// `dropped` is destroyed outside the lock: functions may own arbitrary state.
}

void MainThreadExecutor::execute( Function function ) {
	{
		Node* dispatcher;
		{
			std::lock_guard<std::mutex> lock( mState->mutex );
			if ( !mState->dispatcher )
				return; // Executor shutting down: the function (and what it owns) is dropped.
			mState->queue.emplace_back( std::move( function ) );
			if ( mState->scheduled )
				return;
			mState->scheduled = true;
			dispatcher = mState->dispatcher;
			++mState->scheduling;
		}
		// Node scheduling takes the action manager's lock. Pin the dispatcher until then
		// without taking that lock while holding the executor's mutex.
		const State::SchedulingLease lease{ *mState };
		std::weak_ptr<State> weak = mState;
		dispatcher->runOnMainThread( [weak] {
			if ( auto state = weak.lock() )
				State::drain( state );
		} );
	}
	if ( mState->wake )
		mState->wake();
}

size_t MainThreadExecutor::pending() const {
	std::lock_guard<std::mutex> lock( mState->mutex );
	return mState->queue.size();
}

}} // namespace EE::Scene
