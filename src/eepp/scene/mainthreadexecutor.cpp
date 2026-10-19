#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/scene/node.hpp>
#include <mutex>
#include <vector>

namespace EE { namespace Scene {

struct MainThreadExecutor::State {
	std::mutex mutex;
	std::vector<Function> queue;
	Node* dispatcher{ nullptr };
	bool scheduled{ false };

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

MainThreadExecutor::MainThreadExecutor( Node* dispatcher ) : mState( std::make_shared<State>() ) {
	mState->dispatcher = dispatcher;
}

MainThreadExecutor::~MainThreadExecutor() {
	shutdown();
}

void MainThreadExecutor::shutdown() {
	std::vector<Function> dropped;
	{
		std::lock_guard<std::mutex> lock( mState->mutex );
		mState->dispatcher = nullptr;
		dropped.swap( mState->queue );
	}
	// `dropped` is destroyed outside the lock: functions may own arbitrary state.
}

void MainThreadExecutor::execute( Function function ) {
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
	}
	std::weak_ptr<State> weak = mState;
	dispatcher->runOnMainThread( [weak] {
		if ( auto state = weak.lock() )
			State::drain( state );
	} );
}

size_t MainThreadExecutor::pending() const {
	std::lock_guard<std::mutex> lock( mState->mutex );
	return mState->queue.size();
}

}} // namespace EE::Scene
