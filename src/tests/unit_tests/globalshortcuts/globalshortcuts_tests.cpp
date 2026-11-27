#include "../utest.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <eepp/globalshortcuts.hpp>
#include <eepp/globalshortcuts/shortcutbackend.hpp>
#include <future>
#include <mutex>

namespace EE::System::GlobalShortcuts::Private {

class ServiceTestAccess {
  public:
	static std::unique_ptr<Service> create( Executor& executor,
											std::unique_ptr<ShortcutBackend> backend ) {
		return std::unique_ptr<Service>( new Service( executor, std::move( backend ) ) );
	}
};

} // namespace EE::System::GlobalShortcuts::Private

using namespace EE;
using namespace EE::System::GlobalShortcuts;
using namespace EE::Window;
using EE::System::GlobalShortcuts::Private::ServiceTestAccess;
using EE::System::GlobalShortcuts::Private::ShortcutBackend;

namespace {

constexpr auto TestDeadline = std::chrono::seconds( 3 );

class DeferredExecutor : public Executor {
  public:
	void execute( Function function ) override {
		std::unique_lock lock( mutex );
		if ( hold ) {
			posting = true;
			changed.notify_all();
			changed.wait_for( lock, TestDeadline, [&] { return !hold; } );
		}
		queue.emplace_back( std::move( function ) );
	}

	void holdPost() {
		std::lock_guard lock( mutex );
		hold = true;
	}

	bool waitForPost() {
		std::unique_lock lock( mutex );
		return changed.wait_for( lock, TestDeadline, [&] { return posting; } );
	}

	void releasePost() {
		std::lock_guard lock( mutex );
		hold = false;
		changed.notify_all();
	}

	void drain() {
		std::vector<Function> batch;
		{
			std::lock_guard lock( mutex );
			batch.swap( queue );
		}
		for ( auto& function : batch )
			function();
	}

  private:
	std::mutex mutex;
	std::condition_variable changed;
	std::vector<Function> queue;
	bool hold{ false };
	bool posting{ false };
};

class ControlledBackend : public ShortcutBackend {
  public:
	using Event = SmallUniqueFunction<void( Client& ), 64>;

	EE::System::GlobalShortcuts::Backend type() const override {
		return EE::System::GlobalShortcuts::Backend::X11;
	}

	Result<Capabilities, Error> open() override {
		++opened;
		return Ok( Capabilities{ true, false, true } );
	}

	void reconcile( Client&, Uint64 request, std::vector<Shortcut>, bool retry ) override {
		std::lock_guard lock( mutex );
		requests.push_back( { request, retry } );
		changed.notify_all();
	}

	void cancel( Client&, Uint64 ) override { ++cancelled; }

	void wait( Client& client ) override {
		Event event;
		{
			std::unique_lock lock( mutex );
			changed.wait( lock, [&] { return notified || !events.empty(); } );
			if ( notified ) {
				notified = false;
				return;
			}
			event = std::move( events.front() );
			events.pop_front();
		}
		event( client );
	}

	void wake() noexcept override {
		std::lock_guard lock( mutex );
		notified = true;
		changed.notify_all();
	}

	Result<void, Error> close() override {
		++closed;
		return Ok();
	}

	void enqueue( Event event ) {
		std::lock_guard lock( mutex );
		events.push_back( std::move( event ) );
		changed.notify_all();
	}

	struct Request {
		Uint64 id;
		bool retry;
	};

	Request takeRequest() {
		std::unique_lock lock( mutex );
		if ( !changed.wait_for( lock, TestDeadline, [&] { return !requests.empty(); } ) )
			return {};
		auto request = requests.front();
		requests.pop_front();
		return request;
	}

	void finish( Uint64 request, std::vector<Binding> bindings ) {
		enqueue( [request, bindings = std::move( bindings )]( Client& client ) mutable {
			client.complete( request, Ok( std::move( bindings ) ) );
		} );
	}

	bool flush() {
		std::promise<void> flushed;
		auto completion = flushed.get_future();
		enqueue( [flushed = std::move( flushed )]( Client& ) mutable { flushed.set_value(); } );
		return completion.wait_for( TestDeadline ) == std::future_status::ready;
	}

	std::atomic<unsigned> opened{ 0 };
	std::atomic<unsigned> cancelled{ 0 };
	std::atomic<unsigned> closed{ 0 };

  private:
	std::mutex mutex;
	std::condition_variable changed;
	std::deque<Event> events;
	std::deque<Request> requests;
	bool notified{ false };
};

Shortcut shortcut( std::string id, Keycode key, Uint32 modifiers = KEYMOD_LCTRL ) {
	return { std::move( id ), "Test shortcut", Trigger{ key, modifiers } };
}

ShortcutBackend::Binding bound( const Shortcut& shortcut, Uint64 incarnation ) {
	return { BoundShortcut{ shortcut.id, {}, shortcut.preferred, {}, BindingStatus::Bound },
			 incarnation };
}

} // namespace

UTEST( GlobalShortcuts, stubAndValidation ) {
	DeferredExecutor executor;
	auto stub = ServiceTestAccess::create( executor, nullptr );
	auto& service = *stub;
	EXPECT_TRUE( service.snapshot().state == State::Idle );
	EXPECT_TRUE( *service.clear().get() );
	auto duplicate =
		service.setShortcuts( { shortcut( "one", KEY_A ), shortcut( "two", KEY_A, KEYMOD_RCTRL ) } )
			.get();
	ASSERT_TRUE( duplicate.has_value() && !( *duplicate ) );
	EXPECT_TRUE( duplicate->error().code == ErrorCode::InvalidShortcut );
	EXPECT_EQ( 0u, service.snapshot().generation );
	auto unsupported = service.setShortcuts( { shortcut( "one", KEY_A ) } ).get();
	ASSERT_TRUE( unsupported.has_value() && !( *unsupported ) );
	EXPECT_TRUE( unsupported->error().code == ErrorCode::Unsupported );
	auto snapshot = service.snapshot();
	EXPECT_TRUE( snapshot.state == State::Unavailable );
	ASSERT_EQ( 1u, snapshot.desired.size() );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_CTRL ), snapshot.desired[0].preferred->modifiers );
	EXPECT_TRUE( *service.closeAsync().get() );
	EXPECT_TRUE( *service.closeAsync().get() );
	EXPECT_TRUE( service.snapshot().state == State::Closed );
}

UTEST( GlobalShortcuts, latestDeclarationAndExplicitRetry ) {
	DeferredExecutor executor;
	auto backend = std::make_unique<ControlledBackend>();
	auto* native = backend.get();
	auto service = ServiceTestAccess::create( executor, std::move( backend ) );
	auto first = shortcut( "first", KEY_A );
	auto second = shortcut( "second", KEY_B );
	EXPECT_EQ( 0u, native->opened.load() );
	EXPECT_TRUE( *service->clear().get() );
	EXPECT_EQ( 0u, native->opened.load() );

	auto older = service->setShortcuts( { first } );
	auto firstRequest = native->takeRequest();
	ASSERT_NE( 0u, firstRequest.id );
	CancellationSource cancel;
	auto newest = service->setShortcuts( { second }, cancel.token() );
	auto secondRequest = native->takeRequest();
	ASSERT_NE( 0u, secondRequest.id );
	ASSERT_TRUE( older.waitFor( TestDeadline ) == TaskState::Ready );
	auto superseded = std::move( older ).takeValue();
	ASSERT_FALSE( superseded );
	EXPECT_TRUE( superseded.error().code == ErrorCode::Superseded );
	cancel.cancel();
	ASSERT_TRUE( newest.waitFor( TestDeadline ) == TaskState::Ready );
	auto cancelled = std::move( newest ).takeValue();
	ASSERT_FALSE( cancelled );
	EXPECT_TRUE( cancelled.error().code == ErrorCode::Cancelled );
	EXPECT_EQ( 2u, native->cancelled.load() );
	ASSERT_EQ( 1u, service->snapshot().desired.size() );
	EXPECT_TRUE( second.id == service->snapshot().desired[0].id );
	auto unchanged = service->setShortcuts( { second } ).get();
	ASSERT_TRUE( unchanged.has_value() && !( *unchanged ) );
	EXPECT_TRUE( unchanged->error().code == ErrorCode::Cancelled );

	auto retry = service->retry();
	auto retryRequest = native->takeRequest();
	ASSERT_NE( 0u, retryRequest.id );
	EXPECT_TRUE( retryRequest.retry );
	native->finish( retryRequest.id, { bound( second, 1 ) } );
	ASSERT_TRUE( retry.waitFor( TestDeadline ) == TaskState::Ready );
	auto succeeded = std::move( retry ).takeValue();
	ASSERT_TRUE( succeeded );
	EXPECT_EQ( 2u, succeeded->generation );
	EXPECT_TRUE( succeeded->state == State::Ready );
	auto identical = service->setShortcuts( { second } ).get();
	ASSERT_TRUE( identical.has_value() && *identical );
	EXPECT_EQ( succeeded->revision, identical->value().revision );
}

UTEST( GlobalShortcuts, removalInvalidatesQueuedActivationAcrossRestore ) {
	DeferredExecutor executor;
	auto backend = std::make_unique<ControlledBackend>();
	auto* native = backend.get();
	auto service = ServiceTestAccess::create( executor, std::move( backend ) );
	auto first = shortcut( "first", KEY_A );
	auto second = shortcut( "second", KEY_B );
	std::vector<std::string> activations;
	service->setActivationCallback(
		[&]( const Activation& event ) { activations.push_back( event.id ); } );

	auto initial = service->setShortcuts( { first, second } );
	auto initialRequest = native->takeRequest();
	ASSERT_NE( 0u, initialRequest.id );
	native->finish( initialRequest.id, { bound( first, 1 ), bound( second, 2 ) } );
	ASSERT_TRUE( initial.waitFor( TestDeadline ) == TaskState::Ready );
	native->enqueue( []( ShortcutBackend::Client& client ) {
		client.activate( 1, {} );
		client.activate( 2, {} );
	} );
	ASSERT_TRUE( native->flush() );

	auto remove = service->setShortcuts( { second } );
	auto removeRequest = native->takeRequest();
	ASSERT_NE( 0u, removeRequest.id );
	auto restore = service->setShortcuts( { first, second } );
	auto restoreRequest = native->takeRequest();
	ASSERT_NE( 0u, restoreRequest.id );
	native->finish( restoreRequest.id, { bound( first, 1 ), bound( second, 2 ) } );
	ASSERT_TRUE( restore.waitFor( TestDeadline ) == TaskState::Ready );
	executor.drain();
	ASSERT_EQ( 1u, activations.size() );
	EXPECT_TRUE( second.id == activations.front() );
}

UTEST( GlobalShortcuts, partialReportsAndCoalescedStatePreserveIssues ) {
	DeferredExecutor executor;
	auto backend = std::make_unique<ControlledBackend>();
	auto* native = backend.get();
	auto service = ServiceTestAccess::create( executor, std::move( backend ) );
	auto first = shortcut( "first", KEY_A );
	auto second = shortcut( "second", KEY_B );
	std::optional<ServiceSnapshot> delivered;
	std::vector<std::string> activations;
	service->setStateChangedCallback( [&]( const ServiceSnapshot& state ) { delivered = state; } );
	service->setActivationCallback(
		[&]( const Activation& event ) { activations.push_back( event.id ); } );

	auto registration = service->setShortcuts( { first, second } );
	auto request = native->takeRequest();
	ASSERT_NE( 0u, request.id );
	native->finish( request.id, { bound( first, 1 ) } );
	ASSERT_TRUE( registration.waitFor( TestDeadline ) == TaskState::Ready );
	auto partial = std::move( registration ).takeValue();
	ASSERT_TRUE( partial );
	ASSERT_EQ( 2u, partial->effective.size() );
	EXPECT_TRUE( partial->state == State::Degraded );
	EXPECT_TRUE( partial->effective[1].status == BindingStatus::Unbound );

	native->enqueue( [first, second]( ShortcutBackend::Client& client ) {
		client.bindingsChanged( {}, Error{ ErrorCode::Disconnected, "Test connection lost" } );
		client.bindingsChanged( { bound( first, 2 ), bound( second, 3 ) }, {} );
		for ( unsigned i = 0; i < 65; ++i )
			client.activate( 2, {} );
		client.activate( 3, {} );
	} );
	ASSERT_TRUE( native->flush() );
	executor.drain();
	ASSERT_TRUE( delivered.has_value() );
	EXPECT_TRUE( delivered->state == State::Ready );
	ASSERT_TRUE( delivered->lastBackendIssue.has_value() );
	EXPECT_TRUE( delivered->lastBackendIssue->code == ErrorCode::Disconnected );
	EXPECT_EQ( 2u, delivered->droppedActivations );
	ASSERT_EQ( 64u, activations.size() );
	EXPECT_TRUE( first.id == activations.front() );
	EXPECT_TRUE( second.id == activations.back() );
}

UTEST( GlobalShortcuts, closeQuiescesAnInFlightDispatch ) {
	DeferredExecutor executor;
	auto backend = std::make_unique<ControlledBackend>();
	auto* native = backend.get();
	auto service = ServiceTestAccess::create( executor, std::move( backend ) );
	bool ran = false;
	service->setActivationCallback( [&]( const Activation& ) { ran = true; } );
	auto first = shortcut( "first", KEY_A );
	auto registration = service->setShortcuts( { first } );
	auto request = native->takeRequest();
	ASSERT_NE( 0u, request.id );
	native->finish( request.id, { bound( first, 1 ) } );
	ASSERT_TRUE( registration.waitFor( TestDeadline ) == TaskState::Ready );
	executor.holdPost();
	native->enqueue( []( ShortcutBackend::Client& client ) { client.activate( 1, {} ); } );
	const bool inFlight = executor.waitForPost();
	auto close = service->closeAsync();
	auto again = service->closeAsync();
	EXPECT_FALSE( close.isReady() );
	executor.releasePost();
	EXPECT_TRUE( inFlight );
	ASSERT_TRUE( close.waitFor( TestDeadline ) == TaskState::Ready );
	ASSERT_TRUE( again.waitFor( TestDeadline ) == TaskState::Ready );
	EXPECT_TRUE( std::move( close ).takeValue() );
	EXPECT_TRUE( std::move( again ).takeValue() );
	EXPECT_EQ( 1u, native->closed.load() );
	service.reset();
	executor.drain();
	EXPECT_FALSE( ran );
}

UTEST( GlobalShortcuts, applicationBackendDefersNativeWorkAndCallbackClose ) {
	DeferredExecutor executor;
	struct ApplicationBackend : ShortcutBackend {
		EE::System::GlobalShortcuts::Backend type() const override {
			return EE::System::GlobalShortcuts::Backend::MacOS;
		}
		bool usesApplicationThread() const override { return true; }
		Result<Capabilities, Error> open() override {
			openThread = std::this_thread::get_id();
			return Ok( Capabilities{ true, false, true } );
		}
		void reconcile( Client& receiver, Uint64 request, std::vector<Shortcut> shortcuts,
						bool ) override {
			client = &receiver;
			receiver.complete( request,
							   Ok( std::vector<Binding>{ bound( shortcuts.front(), 1 ) } ) );
		}
		void processEvents( Client& receiver ) override {
			if ( activatePending ) {
				activatePending = false;
				receiver.activate( 1, {} );
			}
		}
		void cancel( Client&, Uint64 ) override {}
		void wait( Client& ) override {}
		void wake() noexcept override {}
		Result<void, Error> close() override {
			closeThread = std::this_thread::get_id();
			return Ok();
		}

		Client* client{ nullptr };
		std::thread::id openThread;
		std::thread::id closeThread;
		bool activatePending{ false };
	};

	auto backend = std::make_unique<ApplicationBackend>();
	auto* native = backend.get();
	auto service = ServiceTestAccess::create( executor, std::move( backend ) );
	OperationTask close;
	bool activated = false;
	service->setActivationCallback( [&]( const Activation& ) {
		activated = true;
		close = service->closeAsync();
	} );

	auto registration = service->setShortcuts( { shortcut( "first", KEY_A ) } );
	EXPECT_FALSE( registration.isReady() );
	executor.drain();
	ASSERT_TRUE( registration.isReady() );
	EXPECT_TRUE( std::move( registration ).takeValue() );
	EXPECT_TRUE( native->openThread == std::this_thread::get_id() );

	native->activatePending = true;
	native->client->scheduleNativeWork();
	executor.drain();
	EXPECT_FALSE( activated );
	executor.drain();
	EXPECT_TRUE( activated );
	EXPECT_FALSE( close.isReady() );
	executor.drain();
	ASSERT_TRUE( close.isReady() );
	EXPECT_TRUE( std::move( close ).takeValue() );
	EXPECT_TRUE( native->closeThread == std::this_thread::get_id() );
}
