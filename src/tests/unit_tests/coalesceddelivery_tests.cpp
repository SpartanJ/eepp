#include "utest.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <eepp/vfs/detail/coalesceddelivery.hpp>
#include <future>
#include <stdexcept>
#include <thread>
#include <utility>

using namespace EE;

namespace {

struct DeliveryPolicy {
	using Payload = int;
	using Delivered = int;

	static bool hasData( const Payload& payload ) { return payload != 0; }

	static Delivered take( Payload& payload ) { return std::exchange( payload, 0 ); }
};

using Delivery = VFS::detail::CoalescedDelivery<DeliveryPolicy>;

void publish( const std::shared_ptr<Delivery>& delivery, int value = 1 ) {
	delivery->update( [value]( int& payload ) {
		payload = value;
		return true;
	} );
}

/** Blocks inside execute(), then queues the function without running it. */
class GatedExecutor : public Executor {
  public:
	void execute( Function function ) override {
		std::unique_lock<std::mutex> lock( mMutex );
		mEntered = true;
		mChanged.notify_all();
		mChanged.wait( lock, [this] { return mReleased; } );
		accessAfterClose = closeReturned.load();
		mPending = std::move( function );
	}

	bool waitUntilEntered() {
		std::unique_lock<std::mutex> lock( mMutex );
		return mChanged.wait_for( lock, std::chrono::seconds( 5 ), [this] { return mEntered; } );
	}

	void release() {
		std::lock_guard<std::mutex> lock( mMutex );
		mReleased = true;
		mChanged.notify_all();
	}

	void drain() {
		Function function;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			function = std::move( mPending );
		}
		if ( function )
			function();
	}

	std::atomic<bool> closeReturned{ false };
	bool accessAfterClose{ false };

  private:
	std::mutex mMutex;
	std::condition_variable mChanged;
	Function mPending;
	bool mEntered{ false };
	bool mReleased{ false };
};

/** A valid synchronous executor whose callback runs on another thread. */
class JoiningExecutor : public Executor {
  public:
	void execute( Function function ) override {
		std::thread worker( std::move( function ) );
		worker.join();
	}
};

struct CallbackLifetime {
	bool& active;
	bool& destroyedDuringCall;

	~CallbackLifetime() { destroyedDuringCall = active; }
};

} // namespace

UTEST( CoalescedDelivery, closeDrainsExecutorSubmissionBeforeTeardown ) {
	auto delivery = std::make_shared<Delivery>();
	GatedExecutor executor;
	int calls = 0;
	delivery->setTarget( &executor, [&]( int&& ) { ++calls; } );
	std::thread producer( [&] { publish( delivery ); } );
	const bool entered = executor.waitUntilEntered();
	std::promise<void> closing;
	auto started = closing.get_future();
	auto closed = std::async( std::launch::async, [&] {
		closing.set_value();
		delivery->close();
		executor.closeReturned = true;
	} );
	started.wait();
	const auto early = closed.wait_for( std::chrono::milliseconds( 50 ) );
	// Always release and join before asserting, including on a regressed implementation.
	executor.release();
	producer.join();
	closed.get();
	executor.drain();
	EXPECT_TRUE( entered );
	EXPECT_EQ( std::future_status::timeout, early );
	EXPECT_FALSE( executor.accessAfterClose );
	EXPECT_EQ( 0, calls );
	EXPECT_TRUE( delivery->isClosed() );
}

UTEST( CoalescedDelivery, targetReplacementDrainsOldSubmissionAndDiscardsOldQueue ) {
	auto delivery = std::make_shared<Delivery>();
	GatedExecutor oldExecutor;
	InlineExecutor newExecutor;
	int oldCalls = 0, newCalls = 0;
	delivery->setTarget( &oldExecutor, [&]( int&& ) { ++oldCalls; } );
	std::thread producer( [&] { publish( delivery ); } );
	const bool entered = oldExecutor.waitUntilEntered();
	std::promise<void> changing;
	auto started = changing.get_future();
	auto changed = std::async( std::launch::async, [&] {
		changing.set_value();
		delivery->setTarget( &newExecutor, [&]( int&& value ) { newCalls += value; } );
	} );
	started.wait();
	const auto early = changed.wait_for( std::chrono::milliseconds( 50 ) );
	oldExecutor.release();
	producer.join();
	changed.get();
	oldExecutor.drain();
	EXPECT_TRUE( entered );
	EXPECT_EQ( std::future_status::timeout, early );
	EXPECT_EQ( 0, oldCalls );
	EXPECT_EQ( 1, newCalls );
	delivery->close();
}

UTEST( CoalescedDelivery, callbackCanReplaceItselfAndPublishAgain ) {
	for ( bool otherThread : { false, true } ) {
		auto delivery = std::make_shared<Delivery>();
		InlineExecutor inlineExecutor;
		JoiningExecutor joiningExecutor;
		Executor* executor = otherThread ? static_cast<Executor*>( &joiningExecutor )
										 : static_cast<Executor*>( &inlineExecutor );
		bool active = false, destroyedDuringCall = false;
		int newCalls = 0;
		auto lifetime = std::make_shared<CallbackLifetime>( active, destroyedDuringCall );
		std::weak_ptr<CallbackLifetime> weak = lifetime;
		delivery->setTarget( executor, [&, lifetime = std::move( lifetime )]( int&& ) {
			// Copy all references needed after re-registration onto this invocation's stack.
			auto* channel = delivery.get();
			auto* executing = &active;
			auto* calls = &newCalls;
			auto* next = &inlineExecutor;
			*executing = true;
			channel->setTarget( next, [calls]( int&& value ) { *calls += value; } );
			channel->update( []( int& payload ) {
				payload = 2;
				return true;
			} );
			*executing = false;
		} );
		publish( delivery );
		EXPECT_FALSE( destroyedDuringCall );
		EXPECT_TRUE( weak.expired() );
		EXPECT_EQ( 2, newCalls );
		delivery->close();
	}
}

UTEST( CoalescedDelivery, replacementReleasesInactiveCallableBeforeInlineDelivery ) {
	auto delivery = std::make_shared<Delivery>();
	GatedExecutor executor;
	InlineExecutor next;
	executor.release();
	auto lifetime = std::make_shared<int>( 1 );
	std::weak_ptr<int> weak = lifetime;
	delivery->setTarget( &executor, [lifetime = std::move( lifetime )]( int&& ) {} );
	publish( delivery );
	bool released = false;
	delivery->setTarget( &next, [&]( int&& ) { released = weak.expired(); } );
	EXPECT_TRUE( released );
	executor.drain();
	delivery->close();
}

UTEST( CoalescedDelivery, callbackCanCloseThroughCrossThreadSynchronousExecutor ) {
	auto delivery = std::make_shared<Delivery>();
	JoiningExecutor executor;
	int calls = 0;
	bool active = false, destroyedDuringCall = false;
	auto lifetime = std::make_shared<CallbackLifetime>( active, destroyedDuringCall );
	std::weak_ptr<CallbackLifetime> weak = lifetime;
	delivery->setTarget( &executor, [&, lifetime = std::move( lifetime )]( int&& ) {
		active = true;
		++calls;
		delivery->close();
		active = false;
	} );
	publish( delivery );
	publish( delivery );
	EXPECT_EQ( 1, calls );
	EXPECT_FALSE( destroyedDuringCall );
	EXPECT_TRUE( weak.expired() );
}

UTEST( CoalescedDelivery, inlineCallbackCanReleaseItsLastOwner ) {
	auto delivery = std::make_shared<Delivery>();
	std::weak_ptr<Delivery> weak = delivery;
	delivery->setTarget( nullptr, [&]( int&& ) {
		delivery->close();
		delivery.reset();
	} );
	// Keep no extra owner in the test helper: the channel itself must retain this dispatch.
	delivery->update( []( int& payload ) {
		payload = 1;
		return true;
	} );
	EXPECT_TRUE( weak.expired() );
}

UTEST( CoalescedDelivery, inlineUpdatesDrainWithoutRecursiveCallbacks ) {
	auto delivery = std::make_shared<Delivery>();
	int calls = 0;
	bool recursive = false, active = false;
	delivery->setTarget( nullptr, [&]( int&& value ) {
		recursive = recursive || active;
		active = true;
		++calls;
		if ( value < 1000 )
			publish( delivery, value + 1 );
		active = false;
	} );
	publish( delivery );
	EXPECT_EQ( 1000, calls );
	EXPECT_FALSE( recursive );
	delivery->close();
}

UTEST( CoalescedDelivery, closeAndReplacementDrainAnAlreadyQueuedCallback ) {
	for ( bool replace : { false, true } ) {
		auto delivery = std::make_shared<Delivery>();
		GatedExecutor executor;
		InlineExecutor next;
		executor.release();
		std::promise<void> entered, release;
		auto running = entered.get_future();
		auto finish = release.get_future();
		delivery->setTarget( &executor, [&]( int&& ) {
			entered.set_value();
			finish.wait();
		} );
		publish( delivery );
		std::thread consumer( [&] { executor.drain(); } );
		running.wait();
		auto changed = std::async( std::launch::async, [&] {
			if ( replace )
				delivery->setTarget( &next, []( int&& ) {} );
			else
				delivery->close();
		} );
		const auto early = changed.wait_for( std::chrono::milliseconds( 50 ) );
		release.set_value();
		consumer.join();
		changed.get();
		EXPECT_EQ( std::future_status::timeout, early );
		delivery->close();
	}
}

UTEST( CoalescedDelivery, waitingRetargetDoesNotBlockReentrantRetarget ) {
	auto delivery = std::make_shared<Delivery>();
	JoiningExecutor executor;
	InlineExecutor next;
	std::promise<void> entered, release;
	auto running = entered.get_future();
	auto finish = release.get_future();
	int innerCalls = 0, finalCalls = 0;
	delivery->setTarget( &executor, [&]( int&& ) {
		entered.set_value();
		finish.wait();
		delivery->setTarget( &next, [&]( int&& ) { ++innerCalls; } );
		publish( delivery, 2 );
	} );
	std::thread producer( [&] { publish( delivery ); } );
	running.wait();
	auto changed = std::async( std::launch::async, [&] {
		delivery->setTarget( &next, [&]( int&& value ) { finalCalls += value; } );
	} );
	const auto early = changed.wait_for( std::chrono::milliseconds( 50 ) );
	release.set_value();
	producer.join();
	changed.get();
	EXPECT_EQ( std::future_status::timeout, early );
	// If the external setter was delayed until after the callback finished, the inner target
	// can consume the update first. In either order it is delivered once and the final target wins.
	EXPECT_EQ( 2, 2 * innerCalls + finalCalls );
	const int before = finalCalls;
	publish( delivery, 3 );
	EXPECT_EQ( before + 3, finalCalls );
	delivery->close();
}

UTEST( CoalescedDelivery, throwingCallbackLeavesCloseDrainable ) {
	auto delivery = std::make_shared<Delivery>();
	delivery->setTarget( nullptr, []( int&& ) { throw std::runtime_error( "callback" ); } );
	bool caught = false;
	try {
		publish( delivery );
	} catch ( const std::runtime_error& ) {
		caught = true;
	}
	EXPECT_TRUE( caught );
	EXPECT_TRUE( delivery->isClosed() );
	delivery->close();
}

UTEST( CoalescedDelivery, concurrentPublicationAndCloseStress ) {
	for ( unsigned round = 0; round < 100; ++round ) {
		auto delivery = std::make_shared<Delivery>();
		InlineExecutor executor;
		std::atomic<int> active{ 0 }, overlap{ 0 }, started{ 0 }, calls{ 0 }, afterClose{ 0 };
		std::atomic<bool> start{ false };
		std::atomic<bool> closed{ false };
		auto callback = [&]( int&& ) {
			if ( closed )
				++afterClose;
			++calls;
			if ( active.fetch_add( 1 ) )
				++overlap;
			std::this_thread::yield();
			--active;
		};
		delivery->setTarget( &executor, callback );
		std::thread retarget( [&] {
			while ( !start )
				std::this_thread::yield();
			for ( unsigned i = 0; i < 20; ++i )
				delivery->setTarget( &executor, callback );
		} );
		std::thread producers[3];
		for ( auto& producer : producers ) {
			producer = std::thread( [&] {
				while ( !start )
					std::this_thread::yield();
				publish( delivery );
				++started;
				for ( unsigned i = 0; i < 100; ++i )
					publish( delivery );
			} );
		}
		start = true;
		// Retargeting can legitimately keep every update pending. Ensure at least one callback
		// ran before testing teardown, rather than assuming publish() always invokes it inline.
		while ( started != 3 || calls == 0 )
			std::this_thread::yield();
		delivery->close();
		closed = true;
		for ( auto& producer : producers )
			producer.join();
		retarget.join();
		EXPECT_EQ( 0, active.load() );
		EXPECT_EQ( 0, overlap.load() );
		EXPECT_GT( calls.load(), 0 );
		EXPECT_EQ( 0, afterClose.load() );
	}
	printf( "  CoalescedDelivery<int>: %zu bytes\n", sizeof( Delivery ) );
	// Standard-library synchronization types have different sizes across ABIs. Budget those
	// separately, leaving 64 bytes for the payload, pointers, counters, flags and padding.
	constexpr size_t budget = sizeof( std::mutex ) + sizeof( std::condition_variable ) +
							  sizeof( Delivery::Callback ) +
							  sizeof( std::enable_shared_from_this<Delivery> ) + 64;
	EXPECT_LE( sizeof( Delivery ), budget );
}
