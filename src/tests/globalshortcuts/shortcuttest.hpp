#ifndef EE_GLOBALSHORTCUTS_TEST_HPP
#define EE_GLOBALSHORTCUTS_TEST_HPP

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <eepp/globalshortcuts.hpp>
#include <mutex>
#include <stdexcept>
#include <string_view>

namespace EE::System::GlobalShortcuts::Test {

inline constexpr auto TestDeadline = std::chrono::seconds( 5 );

class DeferredExecutor final : public Executor {
  public:
	void execute( Function function ) override {
		std::unique_lock lock( mMutex );
		mQueue.emplace_back( std::move( function ) );
		mChanged.notify_all();
		if ( mHold )
			mChanged.wait_for( lock, TestDeadline, [this] { return !mHold; } );
	}

	bool wait( std::chrono::milliseconds timeout = TestDeadline ) {
		std::unique_lock lock( mMutex );
		return mChanged.wait_for( lock, timeout, [this] { return !mQueue.empty(); } );
	}

	void hold() {
		std::lock_guard lock( mMutex );
		mHold = true;
	}

	void resume() {
		std::lock_guard lock( mMutex );
		mHold = false;
		mChanged.notify_all();
	}

	void drain() {
		std::vector<Function> batch;
		{
			std::lock_guard lock( mMutex );
			batch.swap( mQueue );
		}
		for ( auto& function : batch )
			function();
	}

  private:
	std::mutex mMutex;
	std::condition_variable mChanged;
	std::vector<Function> mQueue;
	bool mHold{ false };
};

inline void expect( bool condition, const char* failure ) {
	if ( !condition )
		throw std::runtime_error( failure );
}

template <typename T> T await( Task<T> task ) {
	expect( task.waitFor( TestDeadline ) == TaskState::Ready,
			"Timed out waiting for shortcut task" );
	return std::move( task ).takeValue();
}

inline ServiceSnapshot registration( RegistrationTask task ) {
	auto result = await( std::move( task ) );
	if ( !result )
		throw std::runtime_error( result.error().message );
	return std::move( result ).value();
}

inline const BoundShortcut& binding( const ServiceSnapshot& snapshot, std::string_view id ) {
	const auto it = std::find_if( snapshot.effective.begin(), snapshot.effective.end(),
								  [&]( const auto& entry ) { return entry.id == id; } );
	expect( it != snapshot.effective.end(), "Expected a binding report for the shortcut" );
	return *it;
}

} // namespace EE::System::GlobalShortcuts::Test

#endif
